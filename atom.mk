LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libfutils
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := c utility functions
LOCAL_LIBRARIES := libulog
LOCAL_CONDITIONAL_LIBRARIES := OPTIONAL:libputils

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_SRC_FILES := \
	src/hash.c \
	src/systimetools.c \
	src/timetools.c \

ifeq ("$(TARGET_OS)", "linux")
  LOCAL_SRC_FILES += src/inotify.c
  ifneq ("$(TARGET_OS)-$(TARGET_OS_FLAVOUR)","linux-android")
    LOCAL_SRC_FILES += src/dynmbox.c
  endif
endif

ifneq ("$(TARGET_OS)","windows")
LOCAL_SRC_FILES += \
	src/fdutils.c \
	src/synctools.c \
	src/mbox.c
endif

include $(BUILD_LIBRARY)

# Unit testing
ifdef TARGET_TEST

include $(CLEAR_VARS)
LOCAL_MODULE := tst-libfutils
LOCAL_SRC_FILES := \
	tests/futils_test.c \
	tests/futils_test_dynmbox.c \
	tests/futils_test_systimetools.c

LOCAL_LIBRARIES := libfutils libcunit

include $(BUILD_EXECUTABLE)

endif
