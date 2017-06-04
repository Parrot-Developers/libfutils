LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libfutils
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := c utility functions
LOCAL_LIBRARIES := libulog

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/hash.c \
	src/timetools.c \

ifneq ("$(TARGET_OS)","windows")
LOCAL_SRC_FILES += \
	src/fdutils.c \
	src/synctools.c \
	src/systimetools.c \
	src/mbox.c

ifneq ("$(TARGET_OS)","darwin")
ifneq ("$(TARGET_OS)-$(TARGET_OS_FLAVOUR)","linux-android")
LOCAL_SRC_FILES += \
	src/dynmbox.c
endif
endif

endif

include $(BUILD_LIBRARY)

# Unit testing
ifdef TARGET_TEST

include $(CLEAR_VARS)
LOCAL_MODULE := tst-libfutils
LOCAL_SRC_FILES := \
	tests/futils_test.c \
	tests/futils_test_dynmbox.c

LOCAL_LIBRARIES := libfutils libcunit

include $(BUILD_EXECUTABLE)

endif
