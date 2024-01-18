/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <fstream>
#include <iostream>

#include "JNIHelper.h"

namespace ndk_helper {

#define NATIVEACTIVITY_CLASS_NAME "android/app/NativeActivity"

//---------------------------------------------------------------------------
// JNI Helper functions
//---------------------------------------------------------------------------

//---------------------------------------------------------------------------
// Singleton
//---------------------------------------------------------------------------
JNIHelper* JNIHelper::GetInstance() {
  static JNIHelper helper;
  return &helper;
}

//---------------------------------------------------------------------------
// Ctor
//---------------------------------------------------------------------------
JNIHelper::JNIHelper() : activity_(nullptr) {}

//---------------------------------------------------------------------------
// Dtor
//---------------------------------------------------------------------------
JNIHelper::~JNIHelper() {
  // Lock mutex
  std::lock_guard<std::mutex> lock(mutex_);

  JNIEnv* env = AttachCurrentThread();
  env->DeleteGlobalRef(jni_helper_java_ref_);
  env->DeleteGlobalRef(jni_helper_java_class_);

  DetachCurrentThread();
}

//---------------------------------------------------------------------------
// Init
//---------------------------------------------------------------------------
void JNIHelper::Init(ANativeActivity* activity, const char* helper_class_name) {
  JNIHelper& helper = *GetInstance();

  helper.activity_ = activity;

  // Lock mutex
  std::lock_guard<std::mutex> lock(helper.mutex_);

  JNIEnv* env = helper.AttachCurrentThread();

  // Retrieve app bundle id
  jclass android_content_Context = env->GetObjectClass(helper.activity_->clazz);
  jmethodID midGetPackageName = env->GetMethodID(
      android_content_Context, "getPackageName", "()Ljava/lang/String;");

  jstring packageName = (jstring)env->CallObjectMethod(helper.activity_->clazz,
                                                       midGetPackageName);
  const char* appname = env->GetStringUTFChars(packageName, nullptr);
  helper.app_name_ = std::string(appname);

  jclass cls = helper.RetrieveClass(env, helper_class_name);
  helper.jni_helper_java_class_ = (jclass)env->NewGlobalRef(cls);

  jmethodID constructor =
      env->GetMethodID(helper.jni_helper_java_class_, "<init>",
                       "(Landroid/app/NativeActivity;)V");

  helper.jni_helper_java_ref_ = env->NewObject(helper.jni_helper_java_class_,
                                               constructor, activity->clazz);
  helper.jni_helper_java_ref_ = env->NewGlobalRef(helper.jni_helper_java_ref_);

  // Get app label
  auto labelName = (jstring)helper.CallObjectMethod("getApplicationName",
                                                       "()Ljava/lang/String;");
  const char* label = env->GetStringUTFChars(labelName, nullptr);

  env->ReleaseStringUTFChars(packageName, appname);
  env->ReleaseStringUTFChars(labelName, label);
  env->DeleteLocalRef(packageName);
  env->DeleteLocalRef(labelName);
  env->DeleteLocalRef(cls);
}

void JNIHelper::Init(ANativeActivity* activity, const char* helper_class_name,
                     const char* native_soname) {
  Init(activity, helper_class_name);
  if (native_soname) {
    JNIHelper& helper = *GetInstance();
    // Lock mutex
    std::lock_guard<std::mutex> lock(helper.mutex_);

    JNIEnv* env = helper.AttachCurrentThread();

    // Setup soname
    jstring soname = env->NewStringUTF(native_soname);

    jmethodID mid = env->GetMethodID(helper.jni_helper_java_class_,
                                     "loadLibrary", "(Ljava/lang/String;)V");
    env->CallVoidMethod(helper.jni_helper_java_ref_, mid, soname);

    env->DeleteLocalRef(soname);
  }
}

//---------------------------------------------------------------------------
// readFile
//---------------------------------------------------------------------------
bool JNIHelper::ReadFile(const char* fileName,
                         std::vector<uint8_t>* buffer_ref) {
  if (activity_ == nullptr) {
    LOGI(
        "JNIHelper has not been initialized.Call init() to initialize the "
        "helper");
    return false;
  }

  // Lock mutex
  std::lock_guard<std::mutex> lock(mutex_);

  // First, try reading from externalFileDir;
  JNIEnv* env = AttachCurrentThread();

  jstring str_path = GetExternalFilesDirJString(env);
  const char* path = env->GetStringUTFChars(str_path, nullptr);
  std::string s(path);

  if (fileName[0] != '/') {
    s.append("/");
  }
  s.append(fileName);
  std::ifstream f(s.c_str(), std::ios::binary);

  env->ReleaseStringUTFChars(str_path, path);
  env->DeleteLocalRef(str_path);
  activity_->vm->DetachCurrentThread();

  if (f) {
    LOGI("reading:%s", s.c_str());
    f.seekg(0, std::ifstream::end);
    int32_t fileSize = f.tellg();
    f.seekg(0, std::ifstream::beg);
    buffer_ref->reserve(fileSize);
    buffer_ref->assign(std::istreambuf_iterator<char>(f),
                       std::istreambuf_iterator<char>());
    f.close();
    return true;
  } else {
    // Fallback to assetManager
    AAssetManager* assetManager = activity_->assetManager;
    AAsset* assetFile =
        AAssetManager_open(assetManager, fileName, AASSET_MODE_BUFFER);
    if (!assetFile) {
      return false;
    }
    uint8_t* data = (uint8_t*)AAsset_getBuffer(assetFile);
    int32_t size = AAsset_getLength(assetFile);
    if (data == nullptr) {
      AAsset_close(assetFile);

      LOGI("Failed to load:%s", fileName);
      return false;
    }

    buffer_ref->reserve(size);
    buffer_ref->assign(data, data + size);

    AAsset_close(assetFile);
    return true;
  }
}

uint32_t JNIHelper::LoadTexture(const char* file_name, int32_t* outWidth,
                                int32_t* outHeight, bool* hasAlpha) {
  if (activity_ == nullptr) {
    LOGI(
        "JNIHelper has not been initialized. Call init() to initialize the "
        "helper");
    return 0;
  }

  // Lock mutex
  std::lock_guard<std::mutex> lock(mutex_);

  JNIEnv* env = AttachCurrentThread();
  jstring name = env->NewStringUTF(file_name);

  GLuint tex;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);

  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_NEAREST);
  glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  jmethodID mid = env->GetMethodID(jni_helper_java_class_, "loadTexture",
                                   "(Ljava/lang/String;)Ljava/lang/Object;");

  jobject out = env->CallObjectMethod(jni_helper_java_ref_, mid, name);

  jclass javaCls =
      RetrieveClass(env, "ca/raido/helper/NDKHelper$TextureInformation");
  jfieldID fidRet = env->GetFieldID(javaCls, "ret", "Z");
  jfieldID fidHasAlpha = env->GetFieldID(javaCls, "alphaChannel", "Z");
  jfieldID fidWidth = env->GetFieldID(javaCls, "originalWidth", "I");
  jfieldID fidHeight = env->GetFieldID(javaCls, "originalHeight", "I");
  bool ret = env->GetBooleanField(out, fidRet);
  bool alpha = env->GetBooleanField(out, fidHasAlpha);
  int32_t width = env->GetIntField(out, fidWidth);
  int32_t height = env->GetIntField(out, fidHeight);
  if (!ret) {
    glDeleteTextures(1, &tex);
    tex = -1;
    LOGI("Texture load failed %s", file_name);
  }
  LOGI("Loaded texture original size:%dx%d alpha:%d", width, height,
       (int32_t)alpha);
  if (outWidth != nullptr) {
    *outWidth = width;
  }
  if (outHeight != nullptr) {
    *outHeight = height;
  }
  if (hasAlpha != nullptr) {
    *hasAlpha = alpha;
  }

  // Generate mipmap
  glGenerateMipmap(GL_TEXTURE_2D);

  env->DeleteLocalRef(name);
  DetachCurrentThread();

  return tex;
}

//---------------------------------------------------------------------------
// Misc implementations
//---------------------------------------------------------------------------
jclass JNIHelper::RetrieveClass(JNIEnv* jni, const char* class_name) {
  jclass activity_class = jni->FindClass(NATIVEACTIVITY_CLASS_NAME);
  jmethodID get_class_loader = jni->GetMethodID(
      activity_class, "getClassLoader", "()Ljava/lang/ClassLoader;");
  jobject cls = jni->CallObjectMethod(activity_->clazz, get_class_loader);
  jclass class_loader = jni->FindClass("java/lang/ClassLoader");
  jmethodID find_class = jni->GetMethodID(
      class_loader, "loadClass", "(Ljava/lang/String;)Ljava/lang/Class;");

  jstring str_class_name = jni->NewStringUTF(class_name);
  jclass class_retrieved =
      (jclass)jni->CallObjectMethod(cls, find_class, str_class_name);
  jni->DeleteLocalRef(str_class_name);
  jni->DeleteLocalRef(activity_class);
  jni->DeleteLocalRef(class_loader);
  return class_retrieved;
}

jstring JNIHelper::GetExternalFilesDirJString(JNIEnv* env) {
  if (activity_ == nullptr) {
    LOGI(
        "JNIHelper has not been initialized. Call init() to initialize the "
        "helper");
    return nullptr;
  }

  // Invoking getExternalFilesDir() java API
  jclass cls_Env = env->FindClass(NATIVEACTIVITY_CLASS_NAME);
  jmethodID mid = env->GetMethodID(cls_Env, "getExternalFilesDir",
                                   "(Ljava/lang/String;)Ljava/io/File;");
  jobject obj_File = env->CallObjectMethod(activity_->clazz, mid, nullptr);
  jclass cls_File = env->FindClass("java/io/File");
  jmethodID mid_getPath =
      env->GetMethodID(cls_File, "getPath", "()Ljava/lang/String;");
  jstring obj_Path = (jstring)env->CallObjectMethod(obj_File, mid_getPath);

  return obj_Path;
}

jobject JNIHelper::CallObjectMethod(const char* strMethodName,
                                    const char* strSignature, ...) {
  if (activity_ == nullptr) {
    LOGI(
        "JNIHelper has not been initialized. Call init() to initialize the "
        "helper");
    return nullptr;
  }

  JNIEnv* env = AttachCurrentThread();
  jmethodID mid =
      env->GetMethodID(jni_helper_java_class_, strMethodName, strSignature);
  if (mid == nullptr) {
    LOGI("method ID %s, '%s' not found", strMethodName, strSignature);
    return nullptr;
  }

  va_list args;
  va_start(args, strSignature);
  jobject obj = env->CallObjectMethodV(jni_helper_java_ref_, mid, args);
  va_end(args);

  return obj;
}

// This JNI function is invoked from UIThread asynchronously
extern "C" {
JNIEXPORT void Java_com_sample_helper_NDKHelper_RunOnUiThreadHandler(
        [[maybe_unused]] JNIEnv* env, [[maybe_unused]] jobject thiz, int64_t pointer) {
  auto pCallback = reinterpret_cast<std::function<void()>*>(pointer);
  (*pCallback)();

  // Deleting temporary object
  delete pCallback;
}
}

}  // namespace ndkHelper
