LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := libfutils
LOCAL_CATEGORY_PATH := libs
LOCAL_DESCRIPTION := c/c++ utility functions
LOCAL_PRIVATE_LIBRARIES := libulog
LOCAL_CONDITIONAL_PRIVATE_LIBRARIES := OPTIONAL:libputils

LOCAL_EXPORT_C_INCLUDES := \
	$(LOCAL_PATH)/include

LOCAL_CFLAGS := -D_FILE_OFFSET_BITS=64

LOCAL_SRC_FILES := \
	src/hash.c \
	src/mbox.c \
	src/systimetools.c \
	src/timetools.c \
	src/random.c \
	src/varint.c

ifeq ("$(TARGET_OS)", "linux")
  LOCAL_SRC_FILES += src/inotify.c
  ifneq ("$(TARGET_OS_FLAVOUR)", "android")
    LOCAL_SRC_FILES += src/string.c
  endif
endif
ifneq ("$(TARGET_OS)-$(TARGET_OS_FLAVOUR)","linux-android")
  LOCAL_SRC_FILES += src/dynmbox.c
endif

ifeq ("$(TARGET_OS)", "hexagon")
LOCAL_SRC_FILES += src/safew.c
endif

ifeq ($(filter "hexagon" "windows", "$(TARGET_OS)"),)
LOCAL_SRC_FILES += \
	src/fdutils.c \
	src/fs.c \
	src/safew.c \
	src/synctools.c
else ifeq ("$(TARGET_OS)", "windows")
LOCAL_LDLIBS += -lws2_32
endif

ifeq (,$(filter $(TARGET_OS)-$(TARGET_OS_FLAVOUR), baremetal-liteos))
LOCAL_SRC_FILES += \
	src/fs.cpp \
	src/string.cpp
endif

include $(BUILD_LIBRARY)

# Unit testing
ifdef TARGET_TEST

include $(CLEAR_VARS)
LOCAL_MODULE := tst-libfutils
LOCAL_SRC_FILES := \
	tests/futils_test.c \
	tests/futils_test_dynmbox.c \
	tests/futils_test_list.c \
	tests/futils_test_mbox.c \
	tests/futils_test_random.c \
	tests/futils_test_systimetools.c \
	tests/futils_test_timetools.c \
	tests/futils_test_varint.c

ifeq ("$(TARGET_OS)", "linux")
  ifneq ("$(TARGET_OS_FLAVOUR)", "android")
    LOCAL_SRC_FILES += tests/futils_test_string.c
  endif
endif


ifneq ("$(TARGET_OS)","windows")
LOCAL_SRC_FILES += \
	tests/futils_test_safew.c
endif

LOCAL_LIBRARIES := libfutils libcunit

ifeq ("$(TARGET_OS)","windows")
  LOCAL_LDLIBS += -lws2_32
endif

ifeq (,$(filter $(TARGET_OS)-$(TARGET_OS_FLAVOUR), baremetal-liteos))
LOCAL_SRC_FILES += \
	tests/futils_test_fs.cpp \
	tests/futils_test_string.cpp
endif

include $(BUILD_EXECUTABLE)

endif

include $(CLEAR_VARS)
LOCAL_MODULE := futils-random
LOCAL_CATEGORY_PATH := test
LOCAL_DESCRIPTION := futils random benchmark
LOCAL_SRC_FILES := \
	tests/futils_random.c
LOCAL_LIBRARIES := libfutils
include $(BUILD_EXECUTABLE)
