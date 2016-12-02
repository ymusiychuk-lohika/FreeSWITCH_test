//
//  micvolumenotification.cc
//  Skinny
//
//  Created by Tarun Chawla on 20 April 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//

#include "micvolumenotification.h"

namespace cricket{
    MicVolumeNotification::MicVolumeNotification() :
          m_cRef(0)
        , m_enumerator(NULL)
        , m_audioDevListener(NULL)
        , m_micEndpoint(NULL)
        , m_micEndPointVolume(NULL){}

    MicVolumeNotification::~MicVolumeNotification(){
        Terminate();
    }

    STDMETHODIMP_(ULONG) MicVolumeNotification::AddRef() {
        return InterlockedIncrement(&m_cRef);
    }

    STDMETHODIMP_(ULONG) MicVolumeNotification::Release() {
        ULONG ulRef = InterlockedDecrement(&m_cRef);
        if (0 == ulRef)
        {
            delete this;
        }
        return ulRef;
    }

    STDMETHODIMP MicVolumeNotification::QueryInterface(REFIID iid, void** object) {
        if (IID_IUnknown == iid){
            AddRef();
            *object = static_cast<IUnknown*>(this);
        }
        else if (__uuidof(IAudioEndpointVolumeCallback) == iid){
            AddRef();
            *object = static_cast<IAudioEndpointVolumeCallback*>(this);
        }
        else{
            *object = NULL;
            return E_NOINTERFACE;
        }
        return S_OK;
    }

    HRESULT MicVolumeNotification::GetAudioEndPoint(const std::string& micGUID){
        HRESULT hr = E_FAIL;
        m_micGUID = micGUID;
        LOG(LS_INFO) << "Core Audio: GetAudioEndPoint micguid = " << micGUID.c_str();
        if (m_micGUID.empty()){
            LOG(LS_INFO) << "Core Audio: Mic guid is empty";
            //This will get the default audio device according to OS,
            //our default will be auto or user selected
            hr = m_enumerator->GetDefaultAudioEndpoint(eCapture, eCommunications, &m_micEndpoint);

            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Unable to get the default audio endpoint(mic) hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                return hr;
            }

            LPWSTR deviceGUID;
            std::wstring wmicGUID;
            hr = m_micEndpoint->GetId(&deviceGUID);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Unable to get the audio device(mic) guid hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                return hr;
            }
            wmicGUID = std::wstring(deviceGUID);
            m_micGUID = talk_base::ToUtf8(wmicGUID);
        }
        else{
            LOG(LS_INFO) << "Core Audio: Mic guid is present";
            std::wstring wMicGUID = talk_base::ToUtf16(m_micGUID);
            hr = m_enumerator->GetDevice(wMicGUID.c_str(), &m_micEndpoint);
             if (FAILED(hr)){
                 LOG(LS_ERROR) << "Core Audio: Unable to the audio device using mic guid hr = "
                     << std::hex << hr << " GetLastError = " << GetLastError();
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
    HRESULT MicVolumeNotification::AttachAudioEndpoint(){
        HRESULT hr = E_FAIL;
        do{
            if (NULL == m_micEndpoint){
                LOG(LS_ERROR) << "Core Audio: Mic end point is null";
                break;
            }

            hr = m_micEndpoint->Activate(__uuidof(m_micEndPointVolume), CLSCTX_INPROC_SERVER, NULL, (void**)&m_micEndPointVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to activate the audio end "
                    "point volume(mic) hr = " << std::hex << hr << " " << GetLastError();
                break;
            }

            hr = m_micEndPointVolume->RegisterControlChangeNotify(this);

            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to register control for "
                    "system volume(mic) hr = " << std::hex << hr << " " << GetLastError();
                break;
            }
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while attaching default end point ";
            return hr;
        }
        return hr;
    }

    HRESULT MicVolumeNotification::AttachDevListener(AudioDeviceListenerWin *listener, CComPtr<IMMDeviceEnumerator> enumerator, const std::string& micGUID){
        HRESULT hr = E_FAIL;
        LOG(LS_INFO) << "Core Audio: AttachDevListener micguid = " << micGUID.c_str();
        do{
            if (NULL == enumerator){
                LOG(LS_ERROR) << "Core Audio: Enumerator pointer passed is null";
                break;
            }
            m_enumerator = enumerator;

            hr = GetAudioEndPoint(micGUID);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to get the end point(mic) hr = "
                    << std::hex << hr << " " << GetLastError();
                break;
            }

            hr = AttachAudioEndpoint();

            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Failed to attach mic end point";
                break;
            }
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Failed to attach default end point(mic)";
            return hr;
        }

        m_audioDevListener = listener;
        return hr;
    }

    HRESULT MicVolumeNotification::GetMicVolumeStats(VolumeStats &micVolumeStats){
        HRESULT hr = S_OK;
        BOOL mute;
        //getting mic volume stats
        do{
            if (NULL == m_micEndPointVolume){
                LOG(LS_ERROR) << "Core Audio: Mic end point pointer is null";
                hr = E_FAIL;
                break;
            }

            hr = m_micEndPointVolume->GetMasterVolumeLevelScalar(&m_micStats.m_micVolume);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting master volume";
                break;
            }

            hr = m_micEndPointVolume->GetMute(&mute);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while getting mic mute state";
                break;
            }
            m_micStats.m_micMuteState = (mute > 0) ? true : false;
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while getting mic vol stats";
            return hr;
        }

        micVolumeStats.m_micVolume = m_micStats.m_micVolume;
        micVolumeStats.m_micMuteState = m_micStats.m_micMuteState;
        LOG(LS_INFO) << "Core Audio: Mic Volume = " << m_micStats.m_micVolume
            << "Mic Mute State = " << m_micStats.m_micMuteState;
        return hr;
    }

    std::string MicVolumeNotification::GetMicGUID(){
        return m_micGUID;
    }

    // ----------------------------------------------------------------------
    //  OnNotify()
    //
    //  This is called by the audio core when system volume or
    //  mute state for the endpoint we are monitoring is changed
    // ----------------------------------------------------------------------
    HRESULT MicVolumeNotification::OnNotify(PAUDIO_VOLUME_NOTIFICATION_DATA pNotify){
        bool mute = (pNotify->bMuted > 0) ? true : false;
        if ((fabs(m_micStats.m_micVolume - pNotify->fMasterVolume) > DBL_EPSILON)
            || (m_micStats.m_micMuteState != mute)){
            m_micStats.m_micMuteState = mute;
            m_micStats.m_micVolume = pNotify->fMasterVolume;
            if (NULL != m_audioDevListener){
                m_audioDevListener->OnNotifyMicVolume(m_micStats);
            }
            else{
                LOG(LS_ERROR) << "Core Audio: Audio device listener is null while sending notification for mic vol stats";
            }
        }
        return S_OK;
    }

    void MicVolumeNotification::Terminate(){
        HRESULT hr = S_OK;
        LOG(LS_INFO) << "Core Audio: Terminate micguid = " << GetMicGUID().c_str();
        if (NULL != m_micEndPointVolume){
            hr = m_micEndPointVolume->UnregisterControlChangeNotify(this);
        }
        else{
            LOG(LS_INFO) << "Core Audio: IAudioEndPointVolume pointer for mic is null";
        }
        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while unregistering "
                "control change for audio devices(mic) hr = "
                << std::hex << hr << " " << GetLastError();
        }
        m_micEndPointVolume = NULL;
        m_enumerator = NULL;
        m_micEndpoint = NULL;
        m_audioDevListener = NULL;
    }

    bool MicVolumeNotification::SetMicVolume(float micVol) {
        LOG(LS_INFO) << "Core Audio: Core Audio SetMicVolume called";
        HRESULT hr = E_FAIL;

        //setting session volume
        do{
            if (NULL == m_micEndPointVolume){
                LOG(LS_ERROR) << "Core Audio: mic endpoint volume is null";
                hr = E_FAIL;
                break;
            }

            hr = m_micEndPointVolume->SetMasterVolumeLevelScalar(micVol, NULL);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while setting master mic volume hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while setting mic vol";
            return false;
        }
        else{
            return true;
        }
    }

    bool MicVolumeNotification::SetMicMute(bool bMute) {
        LOG(LS_INFO) << "Core Audio: Core Audio SetMicMute called, with value " << bMute;
        HRESULT hr = E_FAIL;

        //setting session volume
        do{
            if (NULL == m_micEndPointVolume){
                LOG(LS_ERROR) << "Core Audio: mic endpoint volume is null";
                hr = E_FAIL;
                break;
            }

            hr = m_micEndPointVolume->SetMute(bMute ? TRUE : FALSE, NULL);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while setting master mic mute hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }
        } while (0);

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Error while setting mic mute";
            return false;
        }
        else{
            return true;
        }
    }
}
