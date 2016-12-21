LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libfutils
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := c utility functions
LOCAL_LIBRARIES := libulog

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/fdutils.c \
	src/hash.c \
	src/timetools.c \
	src/synctools.c \
	src/systimetools.c \
	src/mbox.c

include $(BUILD_LIBRARY)

