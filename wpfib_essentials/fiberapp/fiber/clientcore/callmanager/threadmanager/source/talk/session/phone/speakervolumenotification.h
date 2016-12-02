//
//  speakervolumenotification.h
//  Skinny
//
//  Created by Tarun Chawla on 20 April 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//

#ifndef SPEAKER_VOLUME_NOTIFICATION_H
#define  SPEAKER_VOLUME_NOTIFICATION_H

#include "win32devicemanager.h"
#include "audiodevlistener.h"

struct IAudioEndpointVolumeCallback;

namespace cricket{
    class AudioDeviceListenerWin;

    //SpeakerVolumeNotification for speaker volume related notifications
    class SpeakerVolumeNotification :public IAudioEndpointVolumeCallback
        , IAudioSessionEvents {
    public:
        SpeakerVolumeNotification();
        ~SpeakerVolumeNotification();

        STDMETHOD_(ULONG, AddRef)() override;
        STDMETHOD_(ULONG, Release)() override;
        STDMETHOD(QueryInterface)(REFIID iid, void** object) override;

        // IAudioEndpointVolumeCallback
        IFACEMETHODIMP OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify);

        // IAudioSessionEvents
        STDMETHOD(OnDisplayNameChanged) (LPCWSTR, LPCGUID) { return S_OK; };
        STDMETHOD(OnIconPathChanged) (LPCWSTR, LPCGUID) { return S_OK; };
        STDMETHOD(OnSimpleVolumeChanged) (float NewSimpleVolume, BOOL NewMute, LPCGUID EventContext);
        STDMETHOD(OnChannelVolumeChanged) (DWORD, float NewChannelVolumesVolume[], DWORD, LPCGUID) { return S_OK; };
        STDMETHOD(OnGroupingParamChanged) (LPCGUID, LPCGUID) { return S_OK; };
        STDMETHOD(OnStateChanged) (AudioSessionState) { return S_OK; };
        STDMETHOD(OnSessionDisconnected) (AudioSessionDisconnectReason) { return S_OK; };

        HRESULT GetAudioEndPoint(const std::string& speakerGUID);
        HRESULT AttachAudioEndpoint();
        HRESULT RegisterForAudioSession();
        HRESULT AttachDevListener(AudioDeviceListenerWin *listener, CComPtr<IMMDeviceEnumerator> enumerator, const std::string& speakerGUID="");
        HRESULT GetSpeakerVolStats(VolumeStats &speakerVolStats);
        std::string GetSpeakerGUID();
        bool SetSessionVolume(float spkVol);
        bool SetSpeakerEndpointVolume(float spkVol);
        bool SetSpeakerSessionMute(bool bMute);
        bool SetSpeakerEndpointMute(bool bMute);
        void Terminate();

    private:
        LONG                            m_cRef;
        CComPtr<IMMDeviceEnumerator>    m_enumerator;
        CComPtr<IMMDevice>              m_speakerEndpoint;
        CComPtr<IAudioEndpointVolume>   m_speakerEndPointVolume;
        CComPtr<IAudioSessionManager2>  m_audioSessionManager;
        CComPtr<IAudioSessionControl2>  m_audioSessionControl;
        AudioDeviceListenerWin*         m_audioDevListener;
        std::string                     m_speakerGUID;
        VolumeStats                     m_speakerStats;
    };
}
#endif