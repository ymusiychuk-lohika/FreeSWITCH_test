//
//  audioendpointnotification.cc
//  Skinny
//
//  Created by Tarun Chawla on 20 April 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//

#include "audioendpointnotification.h"

namespace cricket{
    //AudioEndPointNotification

    AudioEndPointNotification::AudioEndPointNotification() :
        m_cRef(0),
        m_enumerator(NULL),
        m_audioDevListener(NULL){
    }

    AudioEndPointNotification::~AudioEndPointNotification(){
        Terminate();
    }

    STDMETHODIMP_(ULONG) AudioEndPointNotification::AddRef() {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) AudioEndPointNotification::Release() {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        if (0 == ulRef)
        {
            delete this;
        }
        return ulRef;
    }

    STDMETHODIMP AudioEndPointNotification::QueryInterface(REFIID iid, void** object) {
        if (IID_IUnknown == iid){
            AddRef();
            *object = static_cast<IUnknown*>(this);
        }
        else if (__uuidof(IMMNotificationClient) == iid){
            AddRef();
            *object = static_cast<IMMNotificationClient*>(this);
        }
        else{
            *object = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  PrintDeviceName()
    //
    //  Given an endpoint ID string, log the friendly device name
    // ----------------------------------------------------------------------
    HRESULT AudioEndPointNotification::PrintDeviceName(LPCWSTR pwstrId)
    {
        if (NULL == m_enumerator){
            LOG(LS_ERROR) << "Core Audio: IMMDeviceEnumerator is null, cannot print device name";
            return E_FAIL;
        }
        if (NULL == m_audioDevListener){
            LOG(LS_ERROR) << "Core Audio: Device Listener is null, cannot proceed";
            return E_FAIL;
        }
        HRESULT hr = E_FAIL;
        CComPtr<IMMDevice> pDevice = NULL;
        CComPtr<IPropertyStore> pProps = NULL;
        PROPVARIANT varString;

        PropVariantInit(&varString);

        hr = m_enumerator->GetDevice(pwstrId, &pDevice);
        if (SUCCEEDED(hr))
        {
            hr = pDevice->OpenPropertyStore(STGM_READ, &pProps);
        }
        else
        {
            LOG(LS_ERROR) << "Core Audio: Error while getting IMMDevice reference "
                << std::hex << hr << " " << GetLastError();
            return hr;
        }
        if (SUCCEEDED(hr))
        {
            // Get the endpoint device's friendly-name property
            hr = pProps->GetValue(PKEY_Device_FriendlyName, &varString);
            if (FAILED(hr))
            {
                LOG(LS_ERROR) << "Core Audio: Error while getting friendly name "
                    << std::hex << hr << " " << GetLastError();
                return hr;
            }
        }
        else
        {
            LOG(LS_ERROR) << "Core Audio: Error while opening property store "
                << std::hex << hr << " " << GetLastError();
            return hr;
        }
        if (NULL != varString.pwszVal){
            LOG(LS_INFO) << "Core Audio: Device name: " << talk_base::ToUtf8(varString.pwszVal) << " EndPoint ID String:" << talk_base::ToUtf8(pwstrId);
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Device name pointer is null";
        }

        PropVariantClear(&varString);
        return hr;
    }
    //right now this notification has no use
    STDMETHODIMP AudioEndPointNotification::OnPropertyValueChanged(
        LPCWSTR device_id, const PROPERTYKEY key) {
        return S_OK;
    }

    // We don't care when devices are added, we are concerned about device state change notification
    STDMETHODIMP AudioEndPointNotification::OnDeviceAdded(LPCWSTR device_id) {
        return S_OK;
    }

    // We don't care when devices are removed, we are concerned about device
    // state change notification for "disabling, unplugging and not present"
    // Ex we only want to know when the device is active not when added
    STDMETHODIMP AudioEndPointNotification::OnDeviceRemoved(LPCWSTR device_id) {
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  OnDeviceStateChanged()
    //
    //  Based on current audio endpoint state we will notify the Win32DeviceWatcher
    //  to enumerate device for removal or arrival
    // ----------------------------------------------------------------------
    STDMETHODIMP AudioEndPointNotification::OnDeviceStateChanged(LPCWSTR device_id,
        DWORD new_state) {
        if (NULL == device_id){
            LOG(LS_ERROR) << "Core Audio: device_id pointer is null";
            return E_FAIL;
        }
        LOG(LS_INFO) << "Core Audio: Device ID String: " << talk_base::ToUtf8(device_id) << " new state:" << new_state;
        PrintDeviceName(device_id);
        if (NULL == m_audioDevListener){
            LOG(LS_ERROR) << "Core Audio: Device listener pointer is null";
            return E_FAIL;
        }
        switch (new_state){
        case DEVICE_STATE_ACTIVE:  m_audioDevListener->OnDeviceStateChanged(true); break;
        case DEVICE_STATE_DISABLED:  m_audioDevListener->OnDeviceStateChanged(false); break;
        case DEVICE_STATE_NOTPRESENT:  m_audioDevListener->OnDeviceStateChanged(false); break;
        case DEVICE_STATE_UNPLUGGED:  m_audioDevListener->OnDeviceStateChanged(false); break;
        }
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  OnDefaultDeviceChanged()
    //
    //  This callback gives information about the flow and role of device
    //  e.g Microphone flow will be capture and role is multimedia device
    // ----------------------------------------------------------------------
    STDMETHODIMP AudioEndPointNotification::OnDefaultDeviceChanged(
        EDataFlow flow, ERole role, LPCWSTR new_default_device_id) {
        char  *pszFlow = "unknown";
        char  *pszRole = "unknown";

        PrintDeviceName(new_default_device_id);

        switch (flow)
        {
        case eRender:
            pszFlow = "eRender";
            break;
        case eCapture:
            pszFlow = "eCapture";
            break;
        }

        switch (role)
        {
        case eConsole:
            pszRole = "eConsole";
            break;
        case eMultimedia:
            pszRole = "eMultimedia";
            break;
        case eCommunications:
            pszRole = "eCommunications";
            break;
        }
        if (new_default_device_id){
            LOG(LS_INFO) << "Core Audio: AudioDeviceListenerWin: New default device: flow = " << pszFlow << ", role = " << pszRole << ", Device ID String: " << talk_base::ToUtf8(new_default_device_id);
        }
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  AttachDevListener()
    //
    //  This attaches the device listener object to end point notification
    //  Whenever there is change in device status we will notify the device
    //  listener which in turn notifies watcher
    // ----------------------------------------------------------------------
    HRESULT AudioEndPointNotification::AttachDevListener(AudioDeviceListenerWin *listener, CComPtr<IMMDeviceEnumerator> enumerator){
        HRESULT hr = E_FAIL;
        if (NULL != enumerator){
            m_enumerator = enumerator;
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Enumerator pointer passed is null";
            return E_FAIL;
        }
        hr = m_enumerator->RegisterEndpointNotificationCallback(this);
        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Registering for end point notification is failed";
            return hr;
        }
        m_audioDevListener = listener;

        return hr;
    }

    void AudioEndPointNotification::Terminate(){
        HRESULT hr = S_OK;
        LOG(LS_INFO) << "Core Audio: Terminate";
        if (NULL == m_enumerator){
            LOG(LS_INFO) << "Core Audio: Enumerator is already null";
        }
        else{
            hr = m_enumerator->UnregisterEndpointNotificationCallback(this);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while unregistering end point "
                    "notification hr = " << std::hex << hr <<
                    "GetLastError = " << GetLastError();
            }
        }
        m_enumerator = NULL;
        m_audioDevListener = NULL;
    }
}