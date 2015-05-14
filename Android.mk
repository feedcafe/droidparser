# Copyright 2013 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LOCAL_SRC_FILES:= src/droidparser.c

LOCAL_C_INCLUDES += external/libxml2/include

LOCAL_SHARED_LIBRARIES:= libxml2

LOCAL_MODULE:= droidparser

include $(BUILD_EXECUTABLE)
