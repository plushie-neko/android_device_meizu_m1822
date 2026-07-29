LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := camera.$(TARGET_BOARD_PLATFORM)
LOCAL_MODULE_RELATIVE_PATH := hw
LOCAL_MODULE_TAGS := optional
LOCAL_VENDOR_MODULE := true

LOCAL_SRC_FILES := \
    camera_shim.cpp

LOCAL_SHARED_LIBRARIES := \
    liblog \
    libhardware \
    libutils \
    libcutils \
    libdl

LOCAL_CFLAGS := -Wall -Wextra -Werror -Wno-unused-parameter

LOCAL_C_INCLUDES += system/media/camera/include

include $(BUILD_SHARED_LIBRARY)
