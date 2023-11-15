LOCAL_PATH := $(call my-dir)

# Add prebuilt Oboe library
include $(CLEAR_VARS)
LOCAL_MODULE := Oboe
LOCAL_SRC_FILES := $(OBOE_SDK_ROOT)/prefab/modules/oboe/libs/android.$(TARGET_ARCH_ABI)/liboboe.so
include $(PREBUILT_SHARED_LIBRARY)

# Add prebuilt CloudXR client library
include $(CLEAR_VARS)
LOCAL_MODULE := CloudXRClient
LOCAL_SRC_FILES := $(CLOUDXR_SDK_ROOT)/jni/$(TARGET_ARCH_ABI)/libCloudXRClient.so
include $(PREBUILT_SHARED_LIBRARY)

include $(CLEAR_VARS)

LOCAL_MODULE := CloudXRClientOVR

LOCAL_CPP_FEATURES := exceptions

LOCAL_C_INCLUDES := $(OVR_SDK_ROOT)/1stParty/OVR/Include \
                    $(OVR_SDK_ROOT)/1stParty/utilities/include \
                    $(OVR_SDK_ROOT)/3rdParty/stb/src \
                    $(OBOE_SDK_ROOT)/prefab/modules/oboe/include \
                    $(C_SHARED_INCLUDE) \
                    $(CLOUDXR_SDK_ROOT)/include \
                    ../src

LOCAL_SRC_FILES := ../src/main.cpp \
                   ../src/EGLHelper.cpp \
                   $(CLOUDXR_SDK_ROOT)/include/CloudXRFileLogger.cpp

LOCAL_LDLIBS := -llog -landroid -lGLESv3 -lEGL
LOCAL_STATIC_LIBRARIES	:= android_native_app_glue
LOCAL_SHARED_LIBRARIES := vrapi Oboe CloudXRClient

include $(BUILD_SHARED_LIBRARY)

# Add OVR VrApi module
$(call import-module,android/native_app_glue)
$(call import-add-path, $(OVR_SDK_ROOT))
$(call import-module,VrApi/Projects/AndroidPrebuilt/jni)
