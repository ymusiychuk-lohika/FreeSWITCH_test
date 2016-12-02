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

#ifndef TALK_SESSION_PHONE_WIN32DEVICEMANAGER_H_
#define TALK_SESSION_PHONE_WIN32DEVICEMANAGER_H_

#include <string>
#include <vector>

#include <atlbase.h>
#include <dbt.h>
#include <strmif.h>  // must come before ks.h
#include <ks.h>
#include <ksmedia.h>
//#define INITGUID  // For PKEY_AudioEndpoint_GUID
#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <FunctionDiscoveryKeys_devpkey.h>
#include <uuids.h>
#include <endpointvolume.h>

#include "talk/base/sigslot.h"
#include "talk/base/stringencode.h"
#include "talk/session/phone/devicemanager.h"
#include "talk/base/win32window.h"
#include "audiodevlistener.h"

#define WIN7_MAJOR_VERSION      6
#define WIN7_MINOR_VERSION      1

namespace cricket {
    class Win32DeviceManager;
    class Win32DeviceWatcher
        : public DeviceWatcher
        , public talk_base::Win32Window {
    public:
        explicit Win32DeviceWatcher(Win32DeviceManager* dm);
        virtual ~Win32DeviceWatcher();
        virtual bool Start();
        virtual void Stop();
        void notify(bool arrival);
        void OnVolumeChanged(const VolumeStats& vol);
        HRESULT GetCurrentVolStats(VolumeStats &volStats);
        bool PostSetSessionVolume(float spkVol);
        bool GetAudioHardwareDetailsFromDeviceId(EDataFlow typeDev,
            const std::string& deviceId,
            std::string& vendorId,
            std::string& productId,
            std::string& classId,
            std::string& deviceIdKey,
            bool idKeyWithType);
        bool DisableDucking();
        bool PostSetSpeakerGUID(const std::string& guid);
        bool PostSetMicGUID(const std::string& guid);
        void EnableAudioNotifyMechanism(bool enable);
        bool PostSetSpeakerEndpointVolume(float spkVol); // Expects value between 0.0 and 1.0
        bool PostSetSpeakerSessionMute(bool bMute);
        bool PostSetSpeakerEndpointMute(bool bMute);
        bool SetDeviceGUIDTimeout(bool isMic, const std::string& guid, unsigned long msecTimeout);
        HRESULT PostGetCurrentVolStats(VolumeStats &volStats, unsigned long msecTimeout);
        bool PostSetMicVolume(float micVol); // Expects value between 0.0 and 1.0
        bool PostSetMicMute(bool bMute);

    private:
        HDEVNOTIFY Register(REFGUID guid);
        void Unregister(HDEVNOTIFY notify);
        virtual bool OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT& result);

        Win32DeviceManager*                 manager_;
        HDEVNOTIFY                          audio_notify_;
        HDEVNOTIFY                          video_notify_;
        AudioDeviceListenerWin*             audio_listener_;
        bool                                audio_listener_enabled_;
        bool                                device_enumerator_supported_;
    };

class Win32DeviceManager : public DeviceManager {
 public:
  Win32DeviceManager();
  virtual ~Win32DeviceManager();

  // Initialization
  virtual bool Init();
  virtual void Terminate();

  virtual bool GetVideoCaptureDevices(std::vector<Device>* devs);
  bool GetCurrentVolStats(VolumeStats &volStats);
  bool PostSetSessionVolume(float spkVol); // Expects value between 0.0 and 1.0
  bool GetAudioHardwareDetailsFromDeviceId(EDataFlow typeDev,
      const std::string& deviceId,
      std::string& vendorId,
      std::string& productId,
      std::string& classId,
      std::string& deviceIdKey,
      bool idKeyWithType);
  bool DisableDucking();
  bool PostSetSpeakerGUID(const std::string& guid);
  bool PostSetMicGUID(const std::string& guid);
  void EnableAudioNotifyMechanism(bool enable);
  bool PostSetSpeakerEndpointVolume(float spkVol); // Expects value between 0.0 and 1.0
  bool PostSetSpeakerSessionMute(bool bMute); // Expects value between 0.0 and 1.0
  bool PostSetSpeakerEndpointMute(bool bMute); // Expects value between 0.0 and 1.0
  bool SetDeviceGUIDTimeout(bool isMic, const std::string& guid, unsigned long msecTimeout);
  bool PostGetCurrentVolStats(VolumeStats &volStats, unsigned long msecTimeout);
  bool PostSetMicVolume(float micVol); // Expects value between 0.0 and 1.0
  bool PostSetMicMute(bool bMute);

 private:
  virtual bool GetAudioDevices(bool input, std::vector<Device>* devs);
  virtual bool GetDefaultVideoCaptureDevice(Device* device);
  bool GetCoreAudioDevices(bool input, std::vector<Device>* devs);
  bool GetDefaultDevice(IMMDeviceEnumerator *enumerator, EDataFlow dataFlow, ERole eRole, Device* out);
  bool GetWaveDevices(bool input, std::vector<Device>* devs);

  virtual bool GetDefaultAudioDevices(bool input);
  bool GetCoreAudioDefaultDevices(bool input);
  bool GetWaveDefaultDevices(bool input);

  bool need_couninitialize_;
  CRITICAL_SECTION CriticalSection;
};

}  // namespace cricket

#endif  // TALK_SESSION_PHONE_WIN32DEVICEMANAGER_H_
