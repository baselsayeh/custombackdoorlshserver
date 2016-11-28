LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
  lsh.c

LOCAL_MODULE := lsh
LOCAL_LDFLAGS   += -llog -lselinux
LOCAL_CFLAGS    += -Os -s -DNDEBUG
LOCAL_CFLAGS    += -fPIE
LOCAL_LDFLAGS   += -fPIE -pie

include $(BUILD_EXECUTABLE)

