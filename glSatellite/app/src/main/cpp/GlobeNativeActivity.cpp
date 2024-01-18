#include <cstdlib>
#include "Engine.h"
#include "MessageQueue.h"

const char *HELPER_CLASS_NAME = "ca/raido/helper/NDKHelper";

Engine g_engine;
android_poll_source g_poll_src;
int g_msgread;
bool g_developer_mode = false;

/* JNI helpers:
   - message queue helpers
   - additional NDK helpers
*/
void HandleMessageWrapper([[maybe_unused]] android_app* app, [[maybe_unused]] android_poll_source* source) {
    Message msg = ReadMessageQueue(g_msgread);
    g_engine.HandleMessage(msg);
}

extern "C" JNIEXPORT void JNICALL
Java_ca_raido_glSatelliteDemo_GlobeNativeActivity_useTle(
        JNIEnv *env, [[maybe_unused]] jobject thiz, jstring javaString) {
    const char *nativeString = env->GetStringUTFChars(javaString, nullptr);
    size_t len = strlen(nativeString);
    char *path = reinterpret_cast<char*>(calloc(len + 1, 1));
    strncpy(path, nativeString, len);
    env->ReleaseStringUTFChars(javaString, nativeString);
    Message msg = {USE_TLE, path};
    PostMessage(msg);
}

jclass RetrieveClass(JNIEnv *jni, ANativeActivity* activity,
        const char* class_name) {
    auto activity_class = jni->GetObjectClass(activity->clazz);
    jmethodID get_class_loader = jni->GetMethodID(activity_class,
        "getClassLoader", "()Ljava/lang/ClassLoader;");
    jobject cls = jni->CallObjectMethod(activity->clazz, get_class_loader);
    jclass class_loader = jni->FindClass("java/lang/ClassLoader");
    jmethodID find_class = jni->GetMethodID(class_loader, "loadClass",
        "(Ljava/lang/String;)Ljava/lang/Class;");

    jstring str_class_name = jni->NewStringUTF(class_name);
    auto class_retrieved = (jclass)jni->CallObjectMethod(cls, find_class,
        str_class_name);
    jni->DeleteLocalRef(str_class_name);
    return class_retrieved;
}

// void ReadDeveloperMode(ANativeActivity* activity) {
//     JNIEnv *jni;
//     activity->vm->AttachCurrentThread(&jni, nullptr);

//     auto clazz = RetrieveClass(jni, activity, HELPER_CLASS_NAME);
//     auto methodID = jni->GetStaticMethodID(clazz, "isDeveloperMode",
//         "()Z");
//     jboolean jMode = jni->CallStaticBooleanMethod(clazz, methodID);
//     g_developer_mode = (bool)jMode;
//     activity->vm->DetachCurrentThread();
// }

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(android_app *state) {
    g_engine.SetState(state);

    //Init helper functions
    ndk_helper::JNIHelper::Init(state->activity, HELPER_CLASS_NAME);
    // ReadDeveloperMode(state->activity);

    state->userData = &g_engine;
    state->onAppCmd = Engine::HandleCmd;
    state->onInputEvent = Engine::HandleInput;

    InitMessageQueue(state->looper);
    g_poll_src.app = state;
    g_poll_src.process = HandleMessageWrapper;
    g_msgread = AddMessageQueue(&g_poll_src);

#ifdef USE_NDK_PROFILER
    monstartup("libGlobeNativeActivity.so");
#endif

    // Prepare to monitor accelerometer
    g_engine.InitSensors();

    // loop waiting for stuff to do.
    while (true) {
        // Read all pending events.
        int id;
        int events;
        android_poll_source *source;

        // If not animating, we will block forever waiting for events.
        // If animating, we loop until all events are read, then continue
        // to draw the next frame of animation.
        while ((id = ALooper_pollAll(g_engine.IsReady() ? 0 : -1, nullptr,
            &events, (void **)&source)) >= 0) {
            // Process this event.
            if (source) {
                source->process(state, source);
            }

            g_engine.ProcessSensors(id);

            // Check if we are exiting.
            if (state->destroyRequested != 0) {
                g_engine.TermDisplay();
                return;
            }
        }

        if (g_engine.IsReady()) {
            // Drawing is throttled to the screen update rate, so there
            // is no need to do timing here.
            g_engine.DrawFrame();
        }
    }
}
