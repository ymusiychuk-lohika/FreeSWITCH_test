//
//  speakervolumenotification.cc
//  Skinny
//
//  Created by Tarun Chawla on 20 April 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//

#include "speakervolumenotification.h"

namespace cricket{
    //SpeakerVolumeNotification

    SpeakerVolumeNotification::SpeakerVolumeNotification() :
        m_cRef(0)
        , m_enumerator(NULL)
        , m_speakerEndpoint(NULL)
        , m_speakerEndPointVolume(NULL)
        , m_audioSessionManager(NULL)
        , m_audioSessionControl(NULL)
        , m_audioDevListener(NULL){}

    SpeakerVolumeNotification::~SpeakerVolumeNotification(){
        Terminate();
    }

    STDMETHODIMP_(ULONG) SpeakerVolumeNotification::AddRef() {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) SpeakerVolumeNotification::Release() {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        if (0 == ulRef)
        {
            delete this;
        }
        return ulRef;
    }

    STDMETHODIMP SpeakerVolumeNotification::QueryInterface(REFIID iid, void** object) {
        if (__uuidof(IAudioEndpointVolumeCallback) == iid){
            AddRef();
            *object = static_cast<IAudioEndpointVolumeCallback*>(this);
        }
        else if (__uuidof(IAudioSessionEvents) == iid){
            AddRef();
            *object = static_cast<IAudioSessionEvents*>(this);
        }
        else{
            *object = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    HRESULT SpeakerVolumeNotification::GetAudioEndPoint(const std::string& speakerGUID){
        HRESULT hr = E_FAIL;
        LOG(LS_INFO) << "Core Audio: GetAudioEndPoint speakerGUID = " << speakerGUID.c_str();
        m_speakerGUID = speakerGUID;
        if (m_speakerGUID.empty()){
            LOG(LS_INFO) << "Core Audio: Speaker guid is empty";
            //This will get the default audio device according to OS,
            //our default will be auto or user selected
            hr = m_enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &m_speakerEndpoint);

            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Unable to get the default audio endpoint hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                return hr;
            }

            LPWSTR deviceGUID;
            std::wstring wspkGUID;
            hr = m_speakerEndpoint->GetId(&deviceGUID);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Unable to get the audio device guid hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                return hr;
            }
            wspkGUID = std::wstring(deviceGUID);
            m_speakerGUID = talk_base::ToUtf8(wspkGUID);
        }
        else{
            LOG(LS_INFO) << "Core Audio: Speaker guid is present";
            std::wstring wSpeakerGUID = talk_base::ToUtf16(m_speakerGUID);
            hr = m_enumerator->GetDevice(wSpeakerGUID.c_str(), &m_speakerEndpoint);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Unable to get the audio device "
                    " using speaker guid hr = " << std::hex << hr <<
                    " GetLastError = " << GetLastError();
                return hr;
            }
        }
        return hr;
    }

    // ----------------------------------------------------------------------
    //  AttachDefaultAudioEndpoint()
    //
    //  This will register for audio end point system volume changes
    //  onNotify will be called whenever there is a change in system volume
    // ----------------------------------------------------------------------
    HRESULT SpeakerVolumeNotification::AttachAudioEndpoint(){
        HRESULT hr = E_FAIL;

        do{
            if (NULL == m_speakerEndpoint){
                LOG(LS_ERROR) << "Core Audio: AudioEnd point is null ";
                break;
            }

            hr = m_speakerEndpoint->Activate(__uuidof(m_speakerEndPointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&m_speakerEndPointVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to activate audio endpoint"
                    " volume hr = " << std::hex << hr << " " << GetLastError();
                break;
            }

            hr = m_speakerEndPointVolume->RegisterControlChangeNotify(this);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to register control for "
                    " system volume " << std::hex << hr <<
                    "GetLastError = " << GetLastError();
                break;
            }

        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Failed to attach any audio end point "
                "for system volume " << std::hex << hr << " " << GetLastError();
            return hr;
        }
        return hr;
    }

    // ----------------------------------------------------------------------
    //  RegisterForAudioSession()
    //
    //  This will register for any changes related to audio session
    //  OnSimpleVolumeChanged will be called for updating any changes
    // ----------------------------------------------------------------------
    HRESULT SpeakerVolumeNotification::RegisterForAudioSession(){

        HRESULT hr = E_FAIL;
        CComPtr<IAudioSessionControl> sessionControl;

        do{
            if (NULL == m_speakerEndpoint){
                LOG(LS_ERROR) << "Core Audio: Speaker end point is null";
                break;
            }

            hr = m_speakerEndpoint->Activate(__uuidof(IAudioSessionManager2), CLSCTX_INPROC_SERVER,
                NULL, reinterpret_cast<void **>(&m_audioSessionManager));
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to activate audio "
                    " session manager " << std::hex << hr <<
                    " GetLastError = " << GetLastError();
                break;
            }

            hr = m_audioSessionManager->GetAudioSessionControl(NULL, 0, &sessionControl);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to get audio session "
                    " control " << std::hex << hr << " " << GetLastError();
                break;
            }

            hr = sessionControl->QueryInterface(IID_PPV_ARGS(&m_audioSessionControl));
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to query interface to audio "
                    "session control hr = " << std::hex << hr << "" << GetLastError();
                break;
            }

            hr = m_audioSessionControl->RegisterAudioSessionNotification(this);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to register for audio "
                    " session notifications" << std::hex << hr <<
                    " GetLastError = " << GetLastError();
                break;
            }
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Failed to register for audio session "
                " hr = " << std::hex << hr << " " << GetLastError();
            return hr;
        }
        return hr;
    }

    HRESULT SpeakerVolumeNotification::AttachDevListener(AudioDeviceListenerWin *listener, CComPtr<IMMDeviceEnumerator> enumerator, const std::string& speakerGUID){
        HRESULT hr = E_FAIL;
        do{
            if (NULL == enumerator){
                LOG(LS_ERROR) << "Core Audio: Enumerator pointer passed is null";
                break;
            }

            m_enumerator = enumerator;

            hr = GetAudioEndPoint(speakerGUID);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to get the default endpoint"
                    " hr = " << std::hex << hr << " " << GetLastError();
                break;
            }

            hr = AttachAudioEndpoint();
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to attach default audio endpoint ";
                break;
            }

            //registering for session volume

            hr = RegisterForAudioSession();
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to register for audio session";
                break;
            }
        } while (0);

        if (SUCCEEDED(hr)){
            m_audioDevListener = listener;
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Failed to attach default end point(speaker)";
            return hr;
        }
        return hr;
    }

    HRESULT SpeakerVolumeNotification::GetSpeakerVolStats(VolumeStats &speakerVolStats){
        HRESULT hr = E_FAIL;
        BOOL mute;
        CComPtr<ISimpleAudioVolume> sessionVolume;

        //getting session volume stats
        do{
            if (NULL == m_audioSessionManager){
                LOG(LS_ERROR) << "Core Audio: Audio session manager is null";
                hr = E_FAIL;
                break;
            }

            hr = m_audioSessionManager->GetSimpleAudioVolume(NULL, 0, &sessionVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting simple audio session";
                break;
            }

            hr = sessionVolume->GetMasterVolume(&m_speakerStats.m_sessionVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while gettting session master volume";
                break;
            }

            hr = sessionVolume->GetMute(&mute);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting session mute state";
                break;
            }
            m_speakerStats.m_sessionMuteState = (mute > 0) ? true : false;
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while getting session vol";
            return hr;
        }
        speakerVolStats.m_sessionVolume = m_speakerStats.m_sessionVolume;
        speakerVolStats.m_sessionMuteState = m_speakerStats.m_sessionMuteState;

        //getting system volume stats
        do{
            if (NULL == m_speakerEndPointVolume){
                LOG(LS_ERROR) << "Core Audio: Speaker end point pointer is null";
                hr = E_FAIL;
                break;
            }

            hr = m_speakerEndPointVolume->GetMasterVolumeLevelScalar(&m_speakerStats.m_systemVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting master volume";
                break;
            }

            hr = m_speakerEndPointVolume->GetMute(&mute);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting mic mute state";
                break;
            }

            m_speakerStats.m_systemMuteState = (mute > 0) ? true : false;
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while getting speaker system vol stats";
            return hr;
        }
        speakerVolStats.m_systemVolume = m_speakerStats.m_systemVolume;
        speakerVolStats.m_systemMuteState = m_speakerStats.m_systemMuteState;

        LOG(LS_INFO) << "Core Audio: System Volume = " << m_speakerStats.m_systemVolume
            << " System Mute State = " << m_speakerStats.m_systemMuteState
            << " Session Volume = " << m_speakerStats.m_sessionVolume
            << " Session mute State = " << m_speakerStats.m_sessionMuteState;

        return hr;
    }

    bool SpeakerVolumeNotification::SetSessionVolume(float spkVol){

        LOG(LS_INFO) << "Core Audio: Core Audio SetSessionVolume called";
        HRESULT hr = E_FAIL;
        CComPtr<ISimpleAudioVolume> sessionVolume;

        //setting session volume
        do{
            if (NULL == m_audioSessionManager){
                LOG(LS_ERROR) << "Core Audio: Audio session manager is null";
                hr = E_FAIL;
                break;
            }

            hr = m_audioSessionManager->GetSimpleAudioVolume(NULL, 0, &sessionVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting simple audio session"
                    "hr = "<<hr<<" GetLastError = "<<GetLastError();
                break;
            }

            hr = sessionVolume->SetMasterVolume(spkVol, NULL);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while setting session volume hr = "<<hr
                    << " GetLastError = " << GetLastError();
                break;
            }

        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while setting session vol";
            return false;
        }
        else{
            return true;
        }
    }

    bool SpeakerVolumeNotification::SetSpeakerSessionMute(bool bMute) {
        LOG(LS_INFO) << "Core Audio: Core Audio SetSpeakerSessionMute called";
        HRESULT hr = E_FAIL;
        CComPtr<ISimpleAudioVolume> sessionVolume;

        //setting session volume
        do{
            if (NULL == m_audioSessionManager){
                LOG(LS_ERROR) << "Core Audio: Audio session manager is null";
                hr = E_FAIL;
                break;
            }

            hr = m_audioSessionManager->GetSimpleAudioVolume(NULL, 0, &sessionVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting simple audio session"
                    "hr = " << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }

            hr = sessionVolume->SetMute(bMute, NULL);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while setting session mute hr = " << std::hex
                    << hr << " GetLastError = " << GetLastError();
                break;
            }

        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while setting session mute";
            return false;
        }
        else{
            return true;
        }
    }

    bool SpeakerVolumeNotification::SetSpeakerEndpointVolume(float spkVol) {
        LOG(LS_INFO) << "Core Audio: Core Audio SetSpeakerEndpointVolume called";
        HRESULT hr = E_FAIL;

        //setting speaker endpoint volume
        do{
            if (NULL == m_speakerEndPointVolume){
                LOG(LS_ERROR) << "Core Audio: m_speakerEndPointVolume is null";
                hr = E_FAIL;
                break;
            }

            hr = m_speakerEndPointVolume->SetMasterVolumeLevelScalar(spkVol, NULL);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while setting speaker endpoint volume hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }

        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while setting speaker endpoint vol";
            return false;
        }
        else{
            return true;
        }
    }

    bool SpeakerVolumeNotification::SetSpeakerEndpointMute(bool bMute) {
        LOG(LS_INFO) << "Core Audio: Core Audio SetSpeakerEndpointMute called";
        HRESULT hr = E_FAIL;

        //setting speaker endpoint volume
        do{
            if (NULL == m_speakerEndPointVolume){
                LOG(LS_ERROR) << "Core Audio: m_speakerEndPointVolume is null";
                hr = E_FAIL;
                break;
            }

            hr = m_speakerEndPointVolume->SetMute(bMute, NULL);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while setting speaker endpoint mute hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }

        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Error while setting speaker endpoint mute";
            return false;
        }
        else{
            return true;
        }
    }

    std::string SpeakerVolumeNotification::GetSpeakerGUID(){
        return m_speakerGUID;
    }

    // ----------------------------------------------------------------------
    //  OnNotify()
    //
    //  This is called by the audio core when system volume or
    //  mute state for the endpoint we are monitoring is changed
    // ----------------------------------------------------------------------
    HRESULT SpeakerVolumeNotification::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify){
        bool mute = (pNotify->bMuted > 0) ? true : false;
        if ((fabs(m_speakerStats.m_systemVolume - pNotify->fMasterVolume) > DBL_EPSILON)
            || (m_speakerStats.m_systemMuteState != mute)){
            m_speakerStats.m_systemMuteState = mute;
            m_speakerStats.m_systemVolume = pNotify->fMasterVolume;
            if (NULL != m_audioDevListener){
                m_audioDevListener->OnNotifySpeakerVolume(m_speakerStats);
            }
            else{
                LOG(LS_ERROR) << "Core Audio: Audio device listener is null while sending system vol stats";
            }
        }
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  OnSimpleVolumeChanged()
    //
    //  This is called by the audio core when session volume or
    //  mute state for the session we are monitoring is changed
    // ----------------------------------------------------------------------
    STDMETHODIMP SpeakerVolumeNotification::OnSimpleVolumeChanged(float newSessionVolume, BOOL newSessionMuteState, LPCGUID EventContext)
    {
        bool mute = newSessionMuteState > 0 ? true : false;
        if ((fabs(m_speakerStats.m_sessionVolume - newSessionVolume) > DBL_EPSILON)
            || (m_speakerStats.m_sessionMuteState != mute)){
            m_speakerStats.m_sessionMuteState = mute;
            m_speakerStats.m_sessionVolume = newSessionVolume;
            if (NULL != m_audioDevListener){
                m_audioDevListener->OnSimpleVolumeChanged(m_speakerStats);
            }
            else{
                LOG(LS_ERROR) << "Core Audio: Audio device listener is null while sending session vol stats";
            }
        }
        return S_OK;
    }

    void SpeakerVolumeNotification::Terminate(){

        LOG(LS_INFO) << "Core Audio: Terminate speakerguid = " << GetSpeakerGUID().c_str();

        HRESULT hr = E_FAIL;
        if (NULL != m_speakerEndPointVolume){
            hr = m_speakerEndPointVolume->UnregisterControlChangeNotify(this);
        }
        else{
            LOG(LS_INFO) << "Core Audio: IAudioEndPointVolume pointer for speaker is null";
        }
        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while unregistering "
                "control change for audio devices(speaker) hr = "
                << std::hex << hr << " " << GetLastError();
        }

        if (NULL != m_audioSessionControl){
            hr = m_audioSessionControl->UnregisterAudioSessionNotification(this);
        }
        else{
            LOG(LS_INFO) << "Core Audio: IAudioSessionControl2 pointer is null";
        }
        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Win32DeviceWatcher: Error while unregistering "
                "audio session notification for audio devices hr = "
                << std::hex << hr << " " << GetLastError();
        }

        m_speakerEndPointVolume = NULL;
        m_audioSessionControl = NULL;
        m_audioSessionManager = NULL;
        m_speakerEndpoint = NULL;
        m_audioDevListener = NULL;
        m_enumerator = NULL;
    }
}