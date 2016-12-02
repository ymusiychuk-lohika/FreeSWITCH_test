LOCAL_PATH := $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := libbjntm

LOCAL_CPPFLAGS := -DPOSIX -DANDROID -Wno-psabi -fPIC 


LOCAL_C_INCLUDES := \
../../../../bjnbuild/android/include/\
../../../../webrtc/webrtc/system_wrappers/source/android

LOCAL_CPP_EXTENSION := .cc
  
#including source files
LOCAL_SRC_FILES := \
    talk/base/asyncfile.cc \
    talk/base/asyncsocket.cc \
    talk/base/common.cc \
    talk/base/cpuid.cc \
    talk/base/cpumonitor.cc \
    talk/base/fileutils.cc \
    talk/base/ipaddress.cc \
    talk/base/logging.cc \
    talk/base/messagehandler.cc \
    talk/base/messagequeue.cc \
    talk/base/nethelpers.cc \
    talk/base/pathutils.cc \
    talk/base/physicalsocketserver.cc \
    talk/base/signalthread.cc \
    talk/base/socketaddress.cc \
    talk/base/stream.cc \
    talk/base/linux.cc \
    talk/base/stringencode.cc \
    talk/base/stringutils.cc \
    talk/base/systeminfo.cc \
    talk/base/thread.cc \
    talk/base/timeutils.cc \
    talk/base/timing.cc \
    talk/base/unixfilesystem.cc \
    talk/base/urlencode.cc \
    talk/base/worker.cc \
    talk/session/phone/devicemanager.cc \
    talk/session/phone/androiddevicemanager.cc

include $(BUILD_STATIC_LIBRARY)

LOCAL_STATIC_LIBRARIES := \
   libbjntm 
