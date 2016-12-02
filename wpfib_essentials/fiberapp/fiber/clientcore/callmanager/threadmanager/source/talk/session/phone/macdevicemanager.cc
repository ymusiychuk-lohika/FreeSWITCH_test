/*
 * libjingle
 * Copyright 2004 Google Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice,
 *     this list of conditions and the following disclaimer.
 *  2. Redistributions in binary form must reproduce the above copyright notice,
 *     this list of conditions and the following disclaimer in the documentation
 *     and/or other materials provided with the distribution.
 *  3. The name of the author may not be used to endorse or promote products
 *     derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO
 * EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "talk/session/phone/macdevicemanager.h"

#include <CoreAudio/CoreAudio.h>
#include <QuickTime/QuickTime.h>

#include "talk/base/logging_libjingle.h"
#include "talk/base/stringutils.h"
#include "talk/base/thread.h"
#include "talk/session/phone/mediacommon.h"

class DeviceWatcherImpl;

namespace cricket {

DeviceManagerInterface* DeviceManagerFactory::Create() {
  return new MacDeviceManager();
}

class MacDeviceWatcher : public DeviceWatcher {
 public:
  explicit MacDeviceWatcher(DeviceManagerInterface* dm);
  virtual ~MacDeviceWatcher();
  virtual bool Start();
  virtual void Stop();

 private:
  DeviceManagerInterface* manager_;
  DeviceWatcherImpl* impl_;
};

static const char* kFilteredAudioDevicesName[] = {
    NULL,
};
// TODO: Try to get hold of a copy of Final Cut to understand why we
//               crash while scanning their components on OS X.
static const char* const kFilteredVideoDevicesName[] =  {
    "Google Camera Adapter",   // Our own magiccams
    "DVCPRO HD",               // Final cut
    "Sonix SN9C201p",          // Crashes in OpenAComponent and CloseComponent
    NULL,
};
static const int kVideoDeviceOpenAttempts = 3;
static const UInt32 kAudioDeviceNameLength = 64;
// Obj-C functions defined in macdevicemanagermm.mm
// TODO: have a shared header for these function defines.
extern DeviceWatcherImpl* CreateDeviceWatcherCallback(
    DeviceManagerInterface* dm);
extern void ReleaseDeviceWatcherCallback(DeviceWatcherImpl* impl);
extern bool GetQTKitVideoDevices(std::vector<Device>* out);
static bool GetAudioDeviceIDs(bool inputs, std::vector<AudioDeviceID>* out);
static bool GetAudioDeviceName(AudioDeviceID id, bool input, std::string* out, std::string* out_guid);
static bool isAudioDeviceInternal(AudioDeviceID id, bool input);

MacDeviceManager::MacDeviceManager() {
  set_watcher(new MacDeviceWatcher(this));
}

MacDeviceManager::~MacDeviceManager() {
}

bool MacDeviceManager::GetVideoCaptureDevices(std::vector<Device>* devices) {
  devices->clear();
  if (!GetQTKitVideoDevices(devices)) {
    return false;
  }
#if ENABLE_SCREEN_SHARE
  devices->insert(devices->begin(), Device(SCREEN_SHARING_FRIENDLY_NAME, SCREEN_SHARING_UNIQUE_ID, true));
#endif    
  return FilterDevices(devices, kFilteredVideoDevicesName);
}

bool MacDeviceManager::GetAudioDevices(bool input,
                                       std::vector<Device>* devs) {
  devs->clear();
  std::vector<AudioDeviceID> dev_ids;
  bool ret = GetAudioDeviceIDs(input, &dev_ids);
  if (!ret) {
    return false;
  }
  for (size_t i = 0; i < dev_ids.size(); ++i) {
    std::string name, guid;
    bool builtin = isAudioDeviceInternal(dev_ids[i], input);
    if (GetAudioDeviceName(dev_ids[i], input, &name, &guid)) {
      devs->push_back(Device(name, guid, builtin));
    }
  }
  return FilterDevices(devs, kFilteredAudioDevicesName);
}

static bool GetAudioDeviceIDs(bool input,
                              std::vector<AudioDeviceID>* out_dev_ids) {
  UInt32 propsize;
  OSErr err = AudioHardwareGetPropertyInfo(kAudioHardwarePropertyDevices,
                                           &propsize, NULL);
  if (0 != err) {
    LOG(LS_ERROR) << "Couldn't get information about property, "
                  << "so no device list acquired.";
    return false;
  }

  size_t num_devices = propsize / sizeof(AudioDeviceID);
  talk_base::scoped_array<AudioDeviceID> device_ids(
      new AudioDeviceID[num_devices]);

  err = AudioHardwareGetProperty(kAudioHardwarePropertyDevices,
                                 &propsize, device_ids.get());
  if (0 != err) {
    LOG(LS_ERROR) << "Failed to get device ids, "
                  << "so no device listing acquired.";
    return false;
  }

  for (size_t i = 0; i < num_devices; ++i) {
    AudioDeviceID an_id = device_ids[i];
    // find out the number of channels for this direction
    // (input/output) on this device -
    // we'll ignore anything with no channels.
    err = AudioDeviceGetPropertyInfo(an_id, 0, input,
                                     kAudioDevicePropertyStreams,
                                     &propsize, NULL);
    if (0 == err) {
      unsigned num_channels = propsize / sizeof(AudioStreamID);
      if (0 < num_channels) {
        out_dev_ids->push_back(an_id);
      }
    } else {
      LOG(LS_ERROR) << "No property info for stream property for device id "
                    << an_id << "(is_input == " << input
                    << "), so not including it in the list.";
    }
  }

  return true;
}

static bool isAudioDeviceInternal(AudioDeviceID id, bool input)
{
  UInt32 type=0;
  UInt32 length = sizeof(type); 
  AudioDeviceGetProperty(id, 0, input,
                         kAudioDevicePropertyTransportType,
                         &length, &type);
  return (type == kAudioDeviceTransportTypeBuiltIn);
}

static char * MYCFStringCopyUTF8String(CFStringRef aString) {
    if (aString == NULL) {
        return NULL;
    }
    CFIndex length = CFStringGetLength(aString);
    CFIndex maxSize =
    CFStringGetMaximumSizeForEncoding(length,
                                        kCFStringEncodingUTF8);
    char *buffer = (char *)malloc(maxSize);
    if (CFStringGetCString(aString, buffer, maxSize,
                           kCFStringEncodingUTF8)) {
        return buffer;
    }
    return NULL;
}
    
static bool GetAudioDeviceName(AudioDeviceID id,
                               bool input,
                               std::string* out_name,
                               std::string* out_guid) {
  UInt32 nameLength = kAudioDeviceNameLength;
  char name[kAudioDeviceNameLength + 1];
  OSErr err = AudioDeviceGetProperty(id, 0, input,
                                     kAudioDevicePropertyDeviceName,
                                     &nameLength, name);
  if (0 != err) {
    LOG(LS_ERROR) << "No name acquired for device id " << id;
    return false;
  }
  *out_guid = name;
  *out_name = name;
  return true;
}

MacDeviceWatcher::MacDeviceWatcher(DeviceManagerInterface* manager)
    : DeviceWatcher(manager),
      manager_(manager),
      impl_(NULL) {
}

MacDeviceWatcher::~MacDeviceWatcher() {
}

bool MacDeviceWatcher::Start() {
  if (!impl_) {
    impl_ = CreateDeviceWatcherCallback(manager_);
  }
  return impl_ != NULL;
}

void MacDeviceWatcher::Stop() {
  if (impl_) {
    ReleaseDeviceWatcherCallback(impl_);
    impl_ = NULL;
  }
}

};  // namespace cricket
