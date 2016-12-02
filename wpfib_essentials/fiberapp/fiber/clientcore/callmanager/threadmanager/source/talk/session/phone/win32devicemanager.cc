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

#include "talk/session/phone/win32devicemanager.h"

#include <atlbase.h>
#include <dbt.h>
#include <Devicetopology.h> // To get audio Jack Info
#include <strmif.h>  // must come before ks.h
#include <ks.h>
#include <ksmedia.h>
//#define INITGUID  // For PKEY_AudioEndpoint_GUID
#include <mmdeviceapi.h>
#include <mmsystem.h>
#include <FunctionDiscoveryKeys_devpkey.h>
#include <uuids.h>
#include <tchar.h>

#include "talk/base/win32.h"  // ToUtf8
#include "talk/base/win32window.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/stringutils.h"
#include "talk/base/thread.h"
#include "talk/session/phone/mediacommon.h"

#ifdef HAVE_LOGITECH_HEADERS
#include "third_party/logitech/files/logitechquickcam.h"
#endif


#define EXIT_ON_ERROR(hres)  \
              if (FAILED(hres)) { goto Exit; }
#define SAFE_RELEASE(punk)  \
              if ((punk) != NULL)  \
                { (punk)->Release(); (punk) = NULL; }

namespace cricket {

DeviceManagerInterface* DeviceManagerFactory::Create() {
  return new Win32DeviceManager();
}


static const char* kFilteredAudioDevicesName[] = {
    NULL,
};
static const char* const kFilteredVideoDevicesName[] =  {
    "Google Camera Adapter",   // Our own magiccams
    "Asus virtual Camera",     // Bad Asus desktop virtual cam
    "Bluetooth Video",         // Bad Sony viao bluetooth sharing driver
    "(VFW)",                   // Ignore Video For Windows camera
    "Panoramic",               // Ignore Panoramic cameras of polycom/microsoft roundtable cameras
    NULL,
};
static const wchar_t kFriendlyName[] = L"FriendlyName";
static const wchar_t kDevicePath[] = L"DevicePath";
static const char kUsbDevicePathPrefix[] = "\\\\?\\usb";
static bool GetDevices(const CLSID& catid, std::vector<Device>* out);

Win32DeviceManager::Win32DeviceManager()
    : need_couninitialize_(false) {
  set_watcher(new Win32DeviceWatcher(this));
  InitializeCriticalSectionAndSpinCount(&CriticalSection, 0x00000400);
}

Win32DeviceManager::~Win32DeviceManager() {
  DeleteCriticalSection(&CriticalSection);
  if (initialized()) {
    Terminate();
  }
}

void Win32DeviceManager::EnableAudioNotifyMechanism(bool enable){
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher*>(watcher());
    if (NULL != deviceWatcher){
        deviceWatcher->EnableAudioNotifyMechanism(enable);
    }
    else{
        LOG(LS_ERROR) << "Device watcher is null";
    }
}
bool Win32DeviceManager::Init() {
  if (!initialized()) {
    HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    need_couninitialize_ = SUCCEEDED(hr);
    if (FAILED(hr)) {
      LOG(LS_ERROR) << "CoInitialize failed, hr=" << hr;
      if (hr != RPC_E_CHANGED_MODE) {
        return false;
      }
    }
    if (!watcher()->Start()) {
      return false;
    }
    set_initialized(true);
  }
  return true;
}

void Win32DeviceManager::Terminate() {
  if (initialized()) {
    watcher()->Stop();
    if (need_couninitialize_) {
      CoUninitialize();
      need_couninitialize_ = false;
    }
    set_initialized(false);
  }
}

bool Win32DeviceManager::GetDefaultVideoCaptureDevice(Device* device) {
  bool ret = false;
  // If there are multiple capture devices, we want the first USB one.
  // This avoids issues with defaulting to virtual cameras or grabber cards.
  std::vector<Device> devices;
  ret = (GetVideoCaptureDevices(&devices) && !devices.empty());
  if (ret) {
    *device = devices[0];
    for (size_t i = 0; i < devices.size(); ++i) {
      if (strnicmp(devices[i].id.c_str(), kUsbDevicePathPrefix,
                   ARRAY_SIZE(kUsbDevicePathPrefix) - 1) == 0) {
        *device = devices[i];
        break;
      }
    }
  }
  return ret;
}

bool Win32DeviceManager::GetAudioDevices(bool input,
                                         std::vector<Device>* devs) {
  devs->clear();

  if (talk_base::IsWindowsVistaOrLater()) {
    if (!GetCoreAudioDevices(input, devs))
      return false;
  } else {
    if (!GetWaveDevices(input, devs))
      return false;
  }
  return FilterDevices(devs, kFilteredAudioDevicesName);
}

bool Win32DeviceManager :: GetDefaultAudioDevices(bool input)
{
  if (talk_base::IsWindowsVistaOrLater()) {
    if (!GetCoreAudioDefaultDevices(input))
      return false;
  } else {
    if (!GetWaveDefaultDevices(input))
      return false;
  }
  return true;
}

bool Win32DeviceManager::GetVideoCaptureDevices(std::vector<Device>* devices) {
  devices->clear();
  bool ret = GetDevices(CLSID_VideoInputDeviceCategory, devices);
#if ENABLE_SCREEN_SHARE
  devices->insert( devices->begin(),Device(SCREEN_SHARING_FRIENDLY_NAME, SCREEN_SHARING_UNIQUE_ID, true));
#else
  if(!ret)
  {
      return false;
  }
#endif  //ENABLE_SCREEN_SHARE
  return FilterDevices(devices, kFilteredVideoDevicesName);
}

bool GetDevices(const CLSID& catid, std::vector<Device>* devices) {
  HRESULT hr;

  // CComPtr is a scoped pointer that will be auto released when going
  // out of scope. CoUninitialize must not be called before the
  // release.
  CComPtr<ICreateDevEnum> sys_dev_enum;
  CComPtr<IEnumMoniker> cam_enum;
  if (FAILED(hr = sys_dev_enum.CoCreateInstance(CLSID_SystemDeviceEnum)) ||
      FAILED(hr = sys_dev_enum->CreateClassEnumerator(catid, &cam_enum, 0))) {
    LOG(LS_ERROR) << "Failed to create device enumerator, hr="  << hr;
    return false;
  }

  // Only enum devices if CreateClassEnumerator returns S_OK. If there are no
  // devices available, S_FALSE will be returned, but enumMk will be NULL.
  if (hr == S_OK) {
    CComPtr<IMoniker> mk;
    while (cam_enum->Next(1, &mk, NULL) == S_OK) {
#ifdef HAVE_LOGITECH_HEADERS
      // Initialize Logitech device if applicable
      MaybeLogitechDeviceReset(mk);
#endif
      CComPtr<IPropertyBag> bag;
      if (SUCCEEDED(mk->BindToStorage(NULL, NULL,
          __uuidof(bag), reinterpret_cast<void**>(&bag)))) {
        CComVariant name, path;
        std::string name_str, path_str;
        if (SUCCEEDED(bag->Read(kFriendlyName, &name, 0)) &&
            name.vt == VT_BSTR) {
          name_str = talk_base::ToUtf8(name.bstrVal);
          // Get the device id if one exists.
          if (SUCCEEDED(bag->Read(kDevicePath, &path, 0)) &&
              path.vt == VT_BSTR) {
            path_str = talk_base::ToUtf8(path.bstrVal);
          } else {
            path_str = talk_base::ToUtf8(name.bstrVal);
          }

          devices->push_back(Device(name_str, path_str, true));
        }
      }
      mk = NULL;
    }
  }

  return true;
}

HRESULT GetStringProp(IPropertyStore* bag, PROPERTYKEY key, std::string* out) {
  out->clear();
  PROPVARIANT var;
  PropVariantInit(&var);

  HRESULT hr = bag->GetValue(key, &var);
  if (SUCCEEDED(hr)) {
    if (var.pwszVal)
      *out = talk_base::ToUtf8(var.pwszVal);
    else
      hr = E_FAIL;
  }

  PropVariantClear(&var);
  return hr;
}

HRESULT GetIntProp(IPropertyStore* bag, PROPERTYKEY key, unsigned int* out) {
  PROPVARIANT var;
  PropVariantInit(&var);

  HRESULT hr = bag->GetValue(key, &var);
  if (SUCCEEDED(hr)) {
      *out = var.uintVal;
  }

  PropVariantClear(&var);
  return hr;
}

HRESULT GetDeviceGuid(IMMDevice* device, std::string &name) {
  CComPtr<IPropertyStore> props;

  HRESULT hr = device->OpenPropertyStore(STGM_READ, &props);
  if (FAILED(hr)) {
    return hr;
  }

  // Get the endpoint's name.
  LPWSTR pwszID = NULL;
  hr = device->GetId(&pwszID);
  name = CW2A (pwszID);
  return hr;
}

bool isBuiltInDevice(IPropertyStore* props)
{
  std::string ifaceName;
  GetStringProp(props, PKEY_Device_EnumeratorName, &ifaceName);
  if(ifaceName == "USB" || ifaceName == "usb") {
      return false;
  } else {
    unsigned int formFactor = 0;
    GetIntProp(props, PKEY_AudioEndpoint_FormFactor , &formFactor);
    //HDMI
    if(formFactor == EndpointFormFactor::DigitalAudioDisplayDevice) {
        return false;
    }
  }
  return true;
}

// Taken from http://msdn.microsoft.com/en-us/library/ms679023%28VS.85%29.aspx
bool GetJackAvailability(IMMDevice* device)
{
    HRESULT hr = S_OK;
    IDeviceTopology *pDeviceTopology = NULL;
    IConnector *pConnFrom = NULL;
    IConnector *pConnTo = NULL;
    IPart *pPart = NULL;
    IKsJackDescription *pJackDesc = NULL;
    
    // Get the endpoint device's IDeviceTopology interface.
    hr = device->Activate(__uuidof(IDeviceTopology), CLSCTX_ALL,
                           NULL, (void**)&pDeviceTopology);
    EXIT_ON_ERROR(hr)

    // The device topology for an endpoint device always
    // contains just one connector (connector number 0).
    hr = pDeviceTopology->GetConnector(0, &pConnFrom);
    EXIT_ON_ERROR(hr)

    // Step across the connection to the jack on the adapter.
    hr = pConnFrom->GetConnectedTo(&pConnTo);
    if (HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND) == hr)
    {
        // The adapter device is not currently active.
        hr = E_NOINTERFACE;
    }
    EXIT_ON_ERROR(hr)
    // Get the connector's IPart interface.
    hr = pConnTo->QueryInterface(__uuidof(IPart), (void**)&pPart);
    EXIT_ON_ERROR(hr)

    // Activate the connector's IKsJackDescription interface.
    hr = pPart->Activate(CLSCTX_INPROC_SERVER,
                         __uuidof(IKsJackDescription), (void**)&pJackDesc);
    EXIT_ON_ERROR(hr)

Exit:
    SAFE_RELEASE(pPart)
    SAFE_RELEASE(pConnTo)
    SAFE_RELEASE(pConnFrom)
    SAFE_RELEASE(pDeviceTopology)
    
    return (hr == S_OK ? true : false);      
}

// Adapted from http://msdn.microsoft.com/en-us/library/dd370812(v=VS.85).aspx
HRESULT CricketDeviceFromImmDevice(IMMDevice* device, Device* out) {
  CComPtr<IPropertyStore> props;

  HRESULT hr = device->OpenPropertyStore(STGM_READ, &props);
  if (FAILED(hr)) {
    LOG(LS_WARNING) << "OpenPropertyStore failed: " << hr;
    return hr;
  }

  // Get the endpoint's name and id.
  std::string name, guid;
  bool isInternal = true;
  hr = GetStringProp(props, PKEY_Device_FriendlyName, &name);
  if (SUCCEEDED(hr)) {
    LPWSTR pwszID = NULL;
    hr = device->GetId(&pwszID);
    guid = CW2A (pwszID);
    if (SUCCEEDED(hr)) {
      out->name = name;
      out->id = guid;
      out->builtIn = isBuiltInDevice(props);
    }
  }
  else {
    LOG(LS_WARNING) << "GetStringProp failed: " << hr;
  }
  out->isJackAvailable = GetJackAvailability(device);
  return hr;
}

bool Win32DeviceManager::GetDefaultDevice(IMMDeviceEnumerator * enumerator, EDataFlow dataFlow,
                                          ERole eRole, Device* out)
{
  HRESULT defHr = S_OK;
  CComPtr<IMMDevice> defaultDevice;
  std::string defDevGuid;
  defHr = enumerator->GetDefaultAudioEndpoint(dataFlow, eRole, &defaultDevice);
  if(SUCCEEDED(defHr)) {
    defHr = CricketDeviceFromImmDevice(defaultDevice, out);
  }
  else {
    LOG(LS_WARNING) << "GetDefaultAudioEndpoint for eDataFlow " << dataFlow << " and eRole: " << eRole << " failed with hr " << defHr;
  }
  return true;
}

bool Win32DeviceManager::GetCoreAudioDefaultDevices(bool input)
{
  HRESULT hr = S_OK;
  CComPtr<IMMDeviceEnumerator> enumerator;

  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
      __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
  if (SUCCEEDED(hr)) {
    CComPtr<IMMDeviceCollection> devices;
    hr = enumerator->EnumAudioEndpoints((input ? eCapture : eRender),
                                        DEVICE_STATE_ACTIVE, &devices);
    if (SUCCEEDED(hr)) {
      GetDefaultDevice(enumerator, (input ? eCapture : eRender), eCommunications, (input ? &_defaultCapCommDevice: &_defaultPlayCommDevice));
      GetDefaultDevice(enumerator, (input ? eCapture : eRender), eConsole, (input ? &_defaultCapDevice: &_defaultPlayDevice));
    }
  }
  if (FAILED(hr)) {
    LOG(LS_WARNING) << "GetCoreAudioDefaultDevices failed with hr " << hr;
    return false;
  }
  return true;
}

bool Win32DeviceManager::GetCoreAudioDevices(
    bool input, std::vector<Device>* devs) {
  HRESULT hr = S_OK;
  CComPtr<IMMDeviceEnumerator> enumerator;

  hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
      __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator));
  if (SUCCEEDED(hr)) {
    CComPtr<IMMDeviceCollection> devices;
    hr = enumerator->EnumAudioEndpoints((input ? eCapture : eRender),
                                        DEVICE_STATE_ACTIVE, &devices);
    if (SUCCEEDED(hr)) {
      unsigned int count;
      hr = devices->GetCount(&count);
      if (SUCCEEDED(hr)) {
        for (unsigned int i = 0; i < count; i++) {
          CComPtr<IMMDevice> device;

          // Get pointer to endpoint number i.
          hr = devices->Item(i, &device);
          if (FAILED(hr)) {
            break;
          }
          Device dev;
          hr = CricketDeviceFromImmDevice(device, &dev);
          if (SUCCEEDED(hr)) {
            devs->push_back(dev);
          } else {
            LOG(LS_WARNING) << "Unable to query IMM Device, skipping.  HR="
                            << hr;
            hr = S_FALSE;
          }
        }
      }
    }
  }

  if (FAILED(hr)) {
    LOG(LS_WARNING) << "GetCoreAudioDevices failed with hr " << hr;
    return false;
  }
  return true;
}

bool Win32DeviceManager::GetWaveDefaultDevices(bool input)
{
  if (input)
  {
    WAVEINCAPS caps;
    if (waveInGetDevCaps(0, &caps, sizeof(caps)) == MMSYSERR_NOERROR &&
        caps.wChannels > 0)
    {
      EnterCriticalSection(&CriticalSection);
      char *t = (char*)malloc(sizeof(caps.szPname));
#ifdef UNICODE
      WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, t, sizeof(caps.szPname), NULL, NULL);
#else
      wcstombs(t, caps.szPname, sizeof(caps.szPname));
#endif
      _defaultCapCommDevice.name.assign(t);
      _defaultCapCommDevice.id.assign(t);
      _defaultCapDevice.name.assign(t);
      _defaultCapDevice.id.assign(t);
      free(t);
      LeaveCriticalSection(&CriticalSection);
    }
  }
  else
  {
    WAVEOUTCAPS caps;
    if (waveOutGetDevCaps(0, &caps, sizeof(caps)) == MMSYSERR_NOERROR &&
        caps.wChannels > 0)
    {
      EnterCriticalSection(&CriticalSection);
      char *t = (char*)malloc(sizeof(caps.szPname));
#ifdef UNICODE
      WideCharToMultiByte(CP_UTF8, 0, caps.szPname, -1, t, sizeof(caps.szPname), NULL, NULL);
#else
      wcstombs(t, caps.szPname, sizeof(caps.szPname));
#endif
      _defaultPlayCommDevice.name.assign(t);
      _defaultPlayCommDevice.id.assign(t);
      _defaultPlayDevice.name.assign(t);
      _defaultPlayDevice.id.assign(t);
      free(t);
      LeaveCriticalSection(&CriticalSection);
    }
  }
  return true;
}

bool Win32DeviceManager::GetWaveDevices(bool input, std::vector<Device>* devs) {
  // Note, we don't use the System Device Enumerator interface here since it
  // adds lots of pseudo-devices to the list, such as DirectSound and Wave
  // variants of the same device.
  if (input) {
    int num_devs = waveInGetNumDevs();
    for (int i = 0; i < num_devs; ++i) {
      WAVEINCAPS caps;
      if (waveInGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR &&
          caps.wChannels > 0) {
        devs->push_back(Device(talk_base::ToUtf8(caps.szPname),
                               talk_base::ToUtf8(caps.szPname), true));
      }
    }
  } else {
    int num_devs = waveOutGetNumDevs();
    for (int i = 0; i < num_devs; ++i) {
      WAVEOUTCAPS caps;
      if (waveOutGetDevCaps(i, &caps, sizeof(caps)) == MMSYSERR_NOERROR &&
          caps.wChannels > 0) {
        devs->push_back(Device(talk_base::ToUtf8(caps.szPname),
                               talk_base::ToUtf8(caps.szPname), true));
      }
    }
  }
  return true;
}

//Get the current volume stats for system and session
bool Win32DeviceManager::GetCurrentVolStats(VolumeStats &volStats){
    HRESULT hr = S_OK;
    bool success = false;
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher*>(watcher());
    if (NULL != deviceWatcher){
        hr = deviceWatcher->GetCurrentVolStats(volStats);
        if (SUCCEEDED(hr)){
            success = true;
        }
        else{
            LOG(LS_ERROR) << "GetCurrentVolStats failed";
        }
    }
    else{
        LOG(LS_ERROR) << "Not able to get the device watcher pointer";
    }
    return success;
}

bool Win32DeviceManager::PostSetSessionVolume(float spkVol){
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->PostSetSessionVolume(spkVol);
    }
    else{
        LOG(LS_ERROR) << "Device watcher pointer is null, not able to set speaker volume";
        return false;
    }
}

bool Win32DeviceManager::PostSetSpeakerEndpointVolume(float spkVol){
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->PostSetSpeakerEndpointVolume(spkVol);
    }
    else{
        LOG(LS_ERROR) << "Device watcher pointer is null, not able to set speaker endpoint volume";
        return false;
    }
}

bool Win32DeviceManager::PostSetSpeakerSessionMute(bool bMute) {
        Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->PostSetSpeakerSessionMute(bMute);
    }
    else{
        LOG(LS_ERROR) << "Device watcher pointer is null, not able to set session mute";
        return false;
    }
}

bool Win32DeviceManager::PostSetSpeakerEndpointMute(bool bMute) {
        Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
        if (NULL != deviceWatcher){
            return deviceWatcher->PostSetSpeakerEndpointMute(bMute);
        }
        else{
            LOG(LS_ERROR) << "Device watcher pointer is null, not able to set speaker endpoint mute";
            return false;
        }
    }

bool Win32DeviceManager::GetAudioHardwareDetailsFromDeviceId(EDataFlow typeDev,
    const std::string& deviceId,
    std::string& vendorId,
    std::string& productId,
    std::string& classId,
    std::string& deviceIdKey,
    bool idKeyWithType){

    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->GetAudioHardwareDetailsFromDeviceId(typeDev,
            deviceId, vendorId, productId, classId, deviceIdKey, idKeyWithType);
    }
    else{
        LOG(LS_ERROR) << "Device watcher pointer is null while getting"
            " audio hardware details";
        return false;
    }
}

bool Win32DeviceManager::DisableDucking(){
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->DisableDucking();
    }
    else{
        LOG(LS_ERROR) << "Device watcher pointer is null, not able to set"
            "ducking preference";
        return false;
    }
}

//Set the speaker guid which is currently in use
bool Win32DeviceManager::PostSetSpeakerGUID(const std::string& guid){
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->PostSetSpeakerGUID(guid);
    }
    else{
        LOG(LS_ERROR) << "Not able to get the device watcher pointer while "
            "setting speaker guid";
        return false;
    }
    return false;
}

//Set the mic guid which is currently in use
bool Win32DeviceManager::PostSetMicGUID(const std::string& guid){
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->PostSetMicGUID(guid);
    }
    else{
        LOG(LS_ERROR) << "Not able to get the device watcher pointer while "
            "setting mic guid";
        return false;
    }
    return false;
}

//Set the speaker guid which is currently in use
bool Win32DeviceManager::SetDeviceGUIDTimeout(bool isMic,
    const std::string& guid,
    unsigned long msecTimeout) {
    Win32DeviceWatcher *deviceWatcher =
        dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->SetDeviceGUIDTimeout(isMic,
            guid, msecTimeout);
    }
    else{
        LOG(LS_ERROR) << __FUNCTION__ << ": Not able to get the "
            << "device watcher pointer while setting "
            << (isMic ? "mic" : "speaker") << " guid";
        return false;
    }
}

//Asynchronously get the current volume stats for system and session
bool Win32DeviceManager::PostGetCurrentVolStats(VolumeStats &volStats, unsigned long msecTimeout){
    HRESULT hr = S_OK;
    bool success = false;
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher*>(watcher());
    if (NULL != deviceWatcher){
        hr = deviceWatcher->PostGetCurrentVolStats(volStats, msecTimeout);
        if (SUCCEEDED(hr)){
            success = true;
        }
        else{
            LOG(LS_ERROR) << "PostGetCurrentVolStats failed";
        }
    }
    else{
        LOG(LS_ERROR) << "Not able to get the device watcher pointer";
    }
    return success;
}

// Post a message to change mic volume
bool Win32DeviceManager::PostSetMicVolume(float micVol) {
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->PostSetMicVolume(micVol);
    }
    else{
        LOG(LS_ERROR) << "Device watcher pointer is null, not able to set mic volume";
        return false;
    }
}

bool Win32DeviceManager::PostSetMicMute(bool bMute) {
    Win32DeviceWatcher *deviceWatcher = dynamic_cast<Win32DeviceWatcher* >(watcher());
    if (NULL != deviceWatcher){
        return deviceWatcher->PostSetMicMute(bMute);
    }
    else{
        LOG(LS_ERROR) << "Device watcher pointer is null, not able to set mic mute status";
        return false;
    }
}

Win32DeviceWatcher::Win32DeviceWatcher(Win32DeviceManager* manager)
    : DeviceWatcher(manager),
      manager_(manager),
      audio_notify_(NULL),
      video_notify_(NULL),
      device_enumerator_supported_(false),
      audio_listener_enabled_(false),
      audio_listener_(NULL) {

}

Win32DeviceWatcher::~Win32DeviceWatcher() {
}

void Win32DeviceWatcher::EnableAudioNotifyMechanism(bool enable){
    audio_listener_enabled_ = enable;
}
bool Win32DeviceWatcher::Start() {
    if (!Create(NULL, _T("libjingle Win32DeviceWatcher Window"),
        0, 0, 0, 0, 0, 0)) {
        return false;
    }

    if (audio_listener_enabled_){

        //IMMDeviceEnumerator is supported above vista
        OSVERSIONINFO info = { 0 };
        info.dwOSVersionInfoSize = sizeof(info);
        GetVersionEx(&info);
        if (info.dwMajorVersion >= WIN7_MAJOR_VERSION)
        {
            if ((info.dwMajorVersion == WIN7_MAJOR_VERSION && info.dwMinorVersion >= WIN7_MINOR_VERSION) || (info.dwMajorVersion != WIN7_MAJOR_VERSION))  //Vista has same major version as of Win7
                device_enumerator_supported_ = true;
        }

        LOG(LS_INFO) << "Audio Volume Notification mechanism is enabled";
        if (device_enumerator_supported_){
            HRESULT hr = E_FAIL;
            LOG(LS_INFO) << "Win32DeviceWatcher: Device enumerator supported";
            audio_listener_ = new(std::nothrow) AudioDeviceListenerWin;
            if (NULL != audio_listener_){
                hr = audio_listener_->AttachWatcher(this);
            }
            else{
                LOG(LS_ERROR) << "Win32DeviceWatcher: Error while allocating memory for AudioDeviceListenerWin ";
            }
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Error while attaching watcher to audio listener " << hr << " " << GetLastError();
                device_enumerator_supported_ = false;
            }
        }
        else{
            LOG(LS_ERROR) << "Device enumerator is not supported "
                "switching back to old method";
        }
    }
    else{
        LOG(LS_INFO) << "Audio Volume Notification mechanism is disabled";
    }
    if (!device_enumerator_supported_){
        audio_notify_ = Register(KSCATEGORY_AUDIO);
        if (!audio_notify_) {
            Stop();
            return false;
        }
    }

  video_notify_ = Register(KSCATEGORY_VIDEO);
  if (!video_notify_) {
    Stop();
    return false;
  }

  return true;
}

void Win32DeviceWatcher::Stop() {

  UnregisterDeviceNotification(video_notify_);
  video_notify_ = NULL;

  if (!device_enumerator_supported_){
    UnregisterDeviceNotification(audio_notify_);
  }
  else{
      if (NULL != audio_listener_){
          audio_listener_->Terminate();
          delete audio_listener_;
      }
      else{
          LOG(LS_ERROR) << "audio listener pointer is null, cannot terminate";
      }
  }
  audio_notify_ = NULL;
  audio_listener_ = NULL;
  Destroy();
}

HDEVNOTIFY Win32DeviceWatcher::Register(REFGUID guid) {
  DEV_BROADCAST_DEVICEINTERFACE dbdi;
  dbdi.dbcc_size = sizeof(dbdi);
  dbdi.dbcc_devicetype = DBT_DEVTYP_DEVICEINTERFACE;
  dbdi.dbcc_classguid = guid;
  dbdi.dbcc_name[0] = '\0';
  return RegisterDeviceNotification(handle(), &dbdi,
                                    DEVICE_NOTIFY_WINDOW_HANDLE);
}

void Win32DeviceWatcher::Unregister(HDEVNOTIFY handle) {
    UnregisterDeviceNotification(handle);
}

//notify() will be only called if IMMDeviceEnumerator is active otherwise
//OnMessage() will be called via RegisterDeviceNotification. In SurfacePro
//OnMessage() is not coming for Analog ear phone with 3.5mm jack with
//KSCATEGORY_AUDIO, AudioDevListenerWin is added to resolve the problem,
//which will notify only for audio endpoints.
void Win32DeviceWatcher::notify(bool arrival){
    char * msg = arrival ? "arrival" : "removal";
    LOG(LS_INFO) << "Win32DeviceWatcher: Device " << msg << " notification through IMMDeviceEnumerator";
    if (NULL != manager_){
        manager_->SignalDevicesChange(arrival);
    }
    else{
        LOG(LS_ERROR) << "DeviceManager pointer is null , cannot signal for device change";
    }
}

//Receive notification for system and session volume changes
void Win32DeviceWatcher::OnVolumeChanged(const VolumeStats& vol){
    if (NULL != manager_){
        manager_->SignalVolumeChanges(vol);
    }
    else{
        LOG(LS_ERROR) << "Device manager pointer is null";
    }
}

//Get the current volume stats of system and session
HRESULT Win32DeviceWatcher::GetCurrentVolStats(VolumeStats &volStats){
    if (NULL != audio_listener_){
        return audio_listener_->GetCurrentVolStats(volStats);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null";
        return E_FAIL;
    }
}

//Set the speaker guid which is currently in use
bool Win32DeviceWatcher::PostSetSpeakerGUID(const std::string& guid){
    if (NULL != audio_listener_){
        audio_listener_->PostSetSpeakerGUID(guid);
        return true;
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting speaker guid";
        return false;
    }
}

//Set the mic guid which is currently in use
bool Win32DeviceWatcher::PostSetMicGUID(const std::string& guid){
    if (NULL != audio_listener_){
        audio_listener_->PostSetMicGUID(guid);
        return true;
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting mic guid";
        return false;
    }
}

bool Win32DeviceWatcher::PostSetSessionVolume(float spkVol){
    if (NULL != audio_listener_){
        return audio_listener_->PostToAudioDeviceListener(MSG_SET_SESSION_VOLUME, spkVol);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting session volume";
        return false;
    }
}

bool Win32DeviceWatcher::PostSetSpeakerEndpointVolume(float spkVol){
    if (NULL != audio_listener_){
        return audio_listener_->PostToAudioDeviceListener(MSG_SET_SPEAKER_VOLUME, spkVol);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting speaker Endpoint Volume";
        return false;
    }
}

bool Win32DeviceWatcher::PostSetSpeakerSessionMute(bool bMute) {
    if (NULL != audio_listener_){
        return audio_listener_->PostToAudioDeviceListener(MSG_SET_SESSION_MUTE, bMute);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting speaker session mute";
        return false;
    }
}

bool Win32DeviceWatcher::PostSetSpeakerEndpointMute(bool bMute) {
    if (NULL != audio_listener_){
        return audio_listener_->PostToAudioDeviceListener(MSG_SET_SPEAKER_MUTE, bMute);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting speaker endpoint mute";
        return false;
    }
}
bool Win32DeviceWatcher::GetAudioHardwareDetailsFromDeviceId(EDataFlow typeDev,
    const std::string& deviceId,
    std::string& vendorId,
    std::string& productId,
    std::string& classId,
    std::string& deviceIdKey,
    bool idKeyWithType){
    if (NULL != audio_listener_){
        return audio_listener_->GetAudioHardwareDetailsFromDeviceId(typeDev,
            deviceId, vendorId, productId, classId, deviceIdKey, idKeyWithType);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while getting"
            " audio hardware details";
        return false;
    }
}
bool Win32DeviceWatcher::DisableDucking(){
    if (NULL != audio_listener_){
        return audio_listener_->DisableDucking();
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null, not able"
            " set ducking preference";
        return false;
    }
}

bool Win32DeviceWatcher::OnMessage(UINT uMsg, WPARAM wParam, LPARAM lParam,
    LRESULT& result)
{
    if (uMsg == WM_DEVICECHANGE) {
        if (wParam == DBT_DEVICEARRIVAL ||
            wParam == DBT_DEVICEREMOVECOMPLETE) {
                bool devStatus;
                if(wParam == DBT_DEVICEARRIVAL)
                {
                    devStatus = true;
                }
                else
                {
                    devStatus = false;
                }

                DEV_BROADCAST_DEVICEINTERFACE* dbdi =
                    reinterpret_cast<DEV_BROADCAST_DEVICEINTERFACE*>(lParam);
                if (dbdi->dbcc_classguid == KSCATEGORY_AUDIO ||
                    dbdi->dbcc_classguid == KSCATEGORY_VIDEO) {
                    LOG(LS_INFO) << "Win32DeviceWatcher:Device changed notification ";
                    manager_->SignalDevicesChange(devStatus);
                }
        }
        result = 0;
        return true;
    }

    return false;
}

// Set the device guid via another thread and wait for maximum msecTimeout duration for
// operation to complete.
bool Win32DeviceWatcher::SetDeviceGUIDTimeout(bool isMic,
    const std::string& guid,
    unsigned long msecTimeout){
    if (NULL != audio_listener_){
        return audio_listener_->PostSetDeviceGUIDTimeout(isMic, guid, msecTimeout);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting speaker guid";
        return false;
    }
}

//Get the current volume stats of system and session
HRESULT Win32DeviceWatcher::PostGetCurrentVolStats(VolumeStats &volStats, unsigned long msecTimeout){
    if (NULL != audio_listener_){
        return audio_listener_->PostGetCurrentVolStats(volStats, msecTimeout);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null";
        return E_FAIL;
    }
}

bool Win32DeviceWatcher::PostSetMicVolume(float micVol) {
    if (NULL != audio_listener_){
        return audio_listener_->PostToAudioDeviceListener(MSG_SET_MIC_VOLUME, micVol);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting mic Volume";
        return false;
    }
}

bool Win32DeviceWatcher::PostSetMicMute(bool bMute) {
    if (NULL != audio_listener_){
        return audio_listener_->PostToAudioDeviceListener(MSG_SET_MIC_MUTE, bMute);
    }
    else{
        LOG(LS_ERROR) << "Audio listener pointer is null while setting mic mute status";
        return false;
    }
}
};  // namespace cricket
