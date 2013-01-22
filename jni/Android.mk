LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_CFLAGS	:= -DHAVE_INTTYPES_H
LOCAL_MODULE	:= store
LOCAL_SRC_FILES	:= StoreWatcher.c za_co_technodev_javajni_Store.c Store.c

include $(BUILD_SHARED_LIBRARY)