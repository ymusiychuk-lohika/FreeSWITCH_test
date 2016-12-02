//
//  audioendpointnotification.h
//  Skinny
//
//  Created by Tarun Chawla on 20 April 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//

#ifndef AUDIO_ENDPOINT_NOTIFICATION_H
#define AUDIO_ENDPOINT_NOTIFICATION_H

#include "win32devicemanager.h"
#include "audiodevlistener.h"

struct IMMNotificationClient;

namespace cricket{
    class AudioDeviceListenerWin;
    //AudioEndPointNotification to listen for arrival/removal of audio devices
    class AudioEndPointNotification : public IMMNotificationClient{
    public:
        AudioEndPointNotification();
        ~AudioEndPointNotification();

        // IMMNotificationClient implementation.
        STDMETHOD_(ULONG, AddRef)() override;
        STDMETHOD_(ULONG, Release)() override;
        STDMETHOD(QueryInterface)(REFIID iid, void** object) override;
        STDMETHOD(OnPropertyValueChanged)(LPCWSTR device_id,
            const PROPERTYKEY key) override;
        STDMETHOD(OnDeviceAdded)(LPCWSTR device_id) override;
        STDMETHOD(OnDeviceRemoved)(LPCWSTR device_id) override;
        STDMETHOD(OnDeviceStateChanged)(LPCWSTR device_id, DWORD new_state) override;
        STDMETHOD(OnDefaultDeviceChanged)(EDataFlow flow, ERole role,
            LPCWSTR new_default_device_id) override;

        HRESULT AttachDevListener(AudioDeviceListenerWin *listener, CComPtr<IMMDeviceEnumerator> enumerator);
        void Terminate();
    private:
        LONG                            m_cRef;
        CComPtr<IMMDeviceEnumerator>    m_enumerator;
        AudioDeviceListenerWin*         m_audioDevListener;
        HRESULT PrintDeviceName(LPCWSTR pwstrId);
    };
}
#endif