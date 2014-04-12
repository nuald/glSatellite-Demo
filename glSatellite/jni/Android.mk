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

ifneq ($(filter %armeabi-v7a,$(TARGET_ARCH_ABI)),)
LOCAL_CFLAGS += -mhard-float -D_NDK_MATH_NO_SOFTFP=1
LOCAL_LDLIBS += -lm_hard
ifeq (,$(filter -fuse-ld=mcld,$(APP_LDFLAGS) $(LOCAL_LDFLAGS)))
LOCAL_LDFLAGS += -Wl,--no-warn-mismatch
endif
endif

include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/ndk_helper)
$(call import-module,android/native_app_glue)
