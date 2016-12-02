//
//  micvolumenotification.h
//  Skinny
//
//  Created by Tarun Chawla on 20 April 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//

#ifndef MIC_VOLUME_NOTIFICATION_H
#define MIC_VOLUME_NOTIFICATION_H

#include "win32devicemanager.h"
#include "audiodevlistener.h"

struct IAudioEndpointVolumeCallback;

namespace cricket{
    class AudioDeviceListenerWin;

    //MicVolumeNotification class for mic volume related notifications
    class MicVolumeNotification :public IAudioEndpointVolumeCallback {
    public:
        MicVolumeNotification();
        ~MicVolumeNotification();

        STDMETHOD_(ULONG, AddRef)() override;
        STDMETHOD_(ULONG, Release)() override;
        STDMETHOD(QueryInterface)(REFIID iid, void** object) override;

        // IAudioEndpointVolumeCallback
        IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);

        HRESULT GetAudioEndPoint(const std::string& micGUID);
        HRESULT AttachAudioEndpoint();
        HRESULT AttachDevListener(AudioDeviceListenerWin *listener, CComPtr<IMMDeviceEnumerator> enumerator, const std::string& micGUID="");
        HRESULT GetMicVolumeStats(VolumeStats &micVolumeStats);
        std::string GetMicGUID();
        void Terminate();
        bool SetMicVolume(float micVol);
        bool SetMicMute(bool bMute);
    private:
        LONG                            m_cRef;
        CComPtr<IMMDeviceEnumerator>    m_enumerator;
        AudioDeviceListenerWin*         m_audioDevListener;
        std::string                     m_micGUID;
        CComPtr<IMMDevice>              m_micEndpoint;
        CComPtr<IAudioEndpointVolume>   m_micEndPointVolume;
        VolumeStats                     m_micStats;
    };
}
#endif