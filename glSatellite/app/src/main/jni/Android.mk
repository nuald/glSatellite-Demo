LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := GlobeNativeActivity
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.cpp))
LOCAL_SRC_FILES += ndk_helper/gestureDetector.cpp ndk_helper/gl3stub.cpp ndk_helper/tapCamera.cpp ndk_helper/GLContext.cpp ndk_helper/JNIHelper.cpp ndk_helper/perfMonitor.cpp ndk_helper/vecmath.cpp ndk_helper/shader.cpp

LOCAL_C_INCLUDES :=

LOCAL_CFLAGS := -Wall
LOCAL_CPP_FEATURES += exceptions

LOCAL_LDLIBS    := -llog -landroid -latomic -lEGL -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
