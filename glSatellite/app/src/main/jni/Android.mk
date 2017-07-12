LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE    := GlobeNativeActivity
LOCAL_SRC_FILES := $(notdir $(wildcard $(LOCAL_PATH)/*.cpp))
LOCAL_SRC_FILES += helper/gestureDetector.cpp helper/tapCamera.cpp

LOCAL_C_INCLUDES :=

LOCAL_CFLAGS := -Wall
LOCAL_CPP_FEATURES += exceptions

LOCAL_LDLIBS    := -llog -landroid -latomic -lEGL -lGLESv2
LOCAL_STATIC_LIBRARIES := android_native_app_glue ndk_helper

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/ndk_helper)
$(call import-module,android/native_app_glue)
