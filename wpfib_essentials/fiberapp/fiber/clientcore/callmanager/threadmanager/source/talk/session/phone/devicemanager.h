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

#ifndef TALK_SESSION_PHONE_DEVICEMANAGER_H_
#define TALK_SESSION_PHONE_DEVICEMANAGER_H_

#include <string>
#include <vector>

#include "talk/base/scoped_ptr.h"
#include "talk/base/sigslot.h"
#include "talk/base/stringencode.h"

const std::string AutomaticDeviceName = "Automatic Device";
const std::string AutomaticDeviceGuid = "-1";
#define ENABLE_SCREEN_SHARE 1
#define SCREEN_SHARING_FRIENDLY_NAME "Screen Share"
#define SCREEN_SHARING_UNIQUE_ID "{7306149c-b8c7-4227-9946-6d6316edc64f}"
#define MAX_DEVICE_BUF (128 - 1) // 128 including NULL char

namespace cricket {
enum local_devices_status{
  LOCAL_DEVICES_OK,
  LOCAL_CAM_BUSY,
  LOCAL_NO_CAM,
};
// Used to represent an audio or video capture or render device.
struct Device {
  Device() {}
  Device(const std::string& first, int second)
      : name(first, 0, MAX_DEVICE_BUF),
        id(talk_base::ToString(second), 0, MAX_DEVICE_BUF) 
  {
  }
    
#if defined(ANDROID) || defined(IOS)
  Device(const std::string& first, const std::string& second)
         : name(first), id(second) {}
#else
  Device(const std::string& first, const std::string& second, bool builtin)
      : name(first, 0, MAX_DEVICE_BUF),
        id(second, 0, MAX_DEVICE_BUF),
        builtIn(builtin) {}
#endif

  std::string name;
  std::string id;
  bool builtIn;
  bool isJackAvailable;
};

#ifdef WIN32
struct VolumeStats{
    bool m_sessionMuteState;
    bool m_systemMuteState;
    bool m_micMuteState;
    float m_sessionVolume;
    float m_systemVolume;
    float m_micVolume;
    VolumeStats() :
          m_sessionMuteState(false)
        , m_systemMuteState(false)
        , m_micMuteState(false)
        , m_sessionVolume(-1)
        , m_systemVolume(-1)
        , m_micVolume(-1) {}
    void setToDefault() {
        m_sessionMuteState = false;
        m_systemMuteState = false;
        m_micMuteState = false;
        m_sessionVolume = -1;
        m_systemVolume = -1;
        m_micVolume = -1;
    }
};
#endif
// DeviceManagerInterface - interface to manage the audio and
// video devices on the system.
class DeviceManagerInterface {
 public:
  virtual ~DeviceManagerInterface() { }

  // Initialization
  virtual bool Init() = 0;
  virtual void Terminate() = 0;

  // Capabilities
  virtual int GetCapabilities() = 0;

  //Default Device
  virtual bool GetDefaultAudioInputDevices() = 0;
  virtual bool GetDefaultAudioOutputDevices() = 0;

  // Device enumeration
  virtual bool GetAudioInputDevices(std::vector<Device>* devices) = 0;
  virtual bool GetAudioOutputDevices(std::vector<Device>* devices) = 0;

  virtual bool GetAudioInputDevice(const std::string& name, Device* out) = 0;
  virtual bool GetAudioOutputDevice(const std::string& name, Device* out) = 0;

  virtual bool GetVideoCaptureDevices(std::vector<Device>* devs) = 0;
  virtual bool GetVideoCaptureDevice(const std::string& name, Device* out) = 0;

  sigslot::signal1<bool> SignalDevicesChange;
#ifdef WIN32
  sigslot::signal1<const VolumeStats&> SignalVolumeChanges;
#endif

  static const char kDefaultDeviceName[];
};

class DeviceWatcher {
 public:
  explicit DeviceWatcher(DeviceManagerInterface* dm) {}
  virtual ~DeviceWatcher() {}
  virtual bool Start() { return true; }
  virtual void Stop() {}
};

class DeviceManagerFactory {
 public:
  static DeviceManagerInterface* Create();
 private:
  DeviceManagerFactory();
};

class DeviceManager : public DeviceManagerInterface {
 public:
  DeviceManager();
  virtual ~DeviceManager();

  // Initialization
  virtual bool Init();
  virtual void Terminate();

  // Capabilities
  virtual int GetCapabilities();

  // Default Device
  virtual bool GetDefaultAudioInputDevices();
  virtual bool GetDefaultAudioOutputDevices();

  // Device enumeration
  virtual bool GetAudioInputDevices(std::vector<Device>* devices);
  virtual bool GetAudioOutputDevices(std::vector<Device>* devices);

  virtual bool GetAudioInputDevice(const std::string& name, Device* out);
  virtual bool GetAudioOutputDevice(const std::string& name, Device* out);

  virtual bool GetVideoCaptureDevices(std::vector<Device>* devs);
  virtual bool GetVideoCaptureDevice(const std::string& name, Device* out);

  // The exclusion_list MUST be a NULL terminated list.
  static bool FilterDevices(std::vector<Device>* devices,
      const char* const exclusion_list[]);
  bool initialized() const { return initialized_; }
  Device getDefaultCapDevice() { return _defaultCapDevice; }
  Device getDefaultCapCommDevice() { return _defaultCapCommDevice; }
  Device getDefaultPlayDevice() { return _defaultPlayDevice; }
  Device getDefaultPlayCommDevice() { return _defaultPlayCommDevice; }

 protected:
  virtual bool GetDefaultAudioDevices(bool input);
  virtual bool GetAudioDevices(bool input, std::vector<Device>* devs);
  virtual bool GetAudioDevice(bool is_input, const std::string& name,
                              Device* out);
  virtual bool GetDefaultVideoCaptureDevice(Device* device);

  void set_initialized(bool initialized) { initialized_ = initialized; }

  void set_watcher(DeviceWatcher* watcher) { watcher_.reset(watcher); }
  DeviceWatcher* watcher() { return watcher_.get(); }
  Device _defaultCapDevice;
  Device _defaultCapCommDevice;
  Device _defaultPlayDevice;
  Device _defaultPlayCommDevice;
 private:
  // The exclusion_list MUST be a NULL terminated list.
  static bool ShouldDeviceBeIgnored(const std::string& device_name,
      const char* const exclusion_list[]);
  bool initialized_;
  talk_base::scoped_ptr<DeviceWatcher> watcher_;
};

}  // namespace cricket

#endif  // TALK_SESSION_PHONE_DEVICEMANAGER_H_
