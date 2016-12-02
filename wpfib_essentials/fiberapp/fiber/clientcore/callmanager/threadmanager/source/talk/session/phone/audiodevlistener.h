//
//  audiodevlistener.h
//  Skinny
//
//  Created by Tarun Chawla on 24 March 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//

#ifndef AUDIO_DEV_LISTENER_H
#define AUDIO_DEV_LISTENER_H

#include <string>

#include <atlcomcli.h>
#include <Propsys.h>
#include <Functiondiscoverykeys_devpkey.h>
#include <audiopolicy.h>

#include "talk/session/phone/devicemanager.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/win32.h"
#include "talk/base/messagehandler.h"
#include "talk/base/thread.h"

#include "audioendpointnotification.h"
#include "speakervolumenotification.h"
#include "micvolumenotification.h"

struct IMMNotificationClient;
struct IAudioEndpointVolumeCallback;


namespace cricket{
    class Win32DeviceWatcher;
    class AudioDeviceListenerWin;
    class AudioEndPointNotification;
    class SpeakerVolumeNotification;
    class MicVolumeNotification;

#define SAFE_RELEASE(pointer)  \
              if ((pointer) != NULL)  \
                  { (pointer)->Release(); (pointer) = NULL; }

    enum dispatcher_msg_id {
        MSG_CHANGE_ENDPOINT_MIC
        , MSG_CHANGE_ENDPOINT_SPEAKER
        , MSG_ENDPOINT_VOLUME_CHANGED
        , MSG_SET_SPEAKER_MUTE
        , MSG_SET_SPEAKER_VOLUME
        , MSG_SET_SESSION_MUTE
        , MSG_SET_SESSION_VOLUME
        , MSG_CHANGE_ENDPOINT_GUID_TIMEOUT
        , MSG_UPDATE_VOLUME_AND_POST
        , MSG_GET_VOLUME_STATS_TIMEOUT
        , MSG_SET_MIC_VOLUME
        , MSG_SET_MIC_MUTE
    };

    // This PROPERTYKEY isn't defined in any of the public Microsoft header files
    // but we need it to map the audio device id to the hardware id string
    const GUID HWKEYPROPERTY_GUID
        = { 0xB3F8FA53, 0x0004, 0x438E, { 0x90, 0x03, 0x51, 0xA4, 0x6E, 0x13, 0x9B, 0xFC } };
    static const DWORD HWKEYPROPERTY_PID = 2;
    static const PROPERTYKEY PKEY_AudioHardwareIdString = { HWKEYPROPERTY_GUID, HWKEYPROPERTY_PID };

    // class AudioDeviceListenerWin:
    // This class will listen for callbacks for the audio end points
    // Callbacks will notify the changes in the state of
    // audio end points which will be used for removal or addition
    // of audio devices and the information related to the
    // audio end point for which state has changed, also this class will
    // Notify the volume changes for system(speaker & mic) and session

    class AudioDeviceListenerWin
        : public talk_base::MessageHandler{

    public:

        AudioDeviceListenerWin();
        ~AudioDeviceListenerWin();

        HRESULT OnDeviceStateChanged(bool arrival);
        HRESULT OnNotifySpeakerVolume(const VolumeStats& speakerVolStats);
        HRESULT OnNotifyMicVolume(const VolumeStats& micVolStats);
        HRESULT OnSimpleVolumeChanged(const VolumeStats& sessionVolStats);

        //MessageHandler message reciever
        void OnMessage(talk_base::Message* pmsg);

        HRESULT AttachWatcher(Win32DeviceWatcher* watcher, const std::string& speakerGUID="", const std::string& micGUID="");
        HRESULT ChangeEndpoint(const std::string& speakerGUID, const std::string& micGUID);
        HRESULT GetCurrentVolStats(VolumeStats &volStats);
        void PostSetSpeakerGUID(const std::string& guid);
        void PostSetMicGUID(const std::string& guid);
        void Terminate();
        bool DisableDucking();
        bool GetAudioHardwareDetailsFromDeviceId(EDataFlow typeDev,
            const std::string& deviceId,
            std::string& vendorId,
            std::string& productId,
            std::string& classId,
            std::string& deviceIdKey,
            bool idKeyWithType);
        void HardwareKeyFromHardwareId(const std::string& hardwareId,
            unsigned int endpointType,
            std::string& vendorId,
            std::string& productId,
            std::string& classId);
        std::string StringToUpper(std::string& strToConvert);

        // Used to post messages to dispatcher thread.
        template <typename T>
        bool PostToAudioDeviceListener(
                dispatcher_msg_id msg,
                const T& data)
        {
            m_dispatcherThread.Post(this,
                msg,
                new talk_base::TypedMessageData<T>(data));
            return true;
        }
        bool PostSetDeviceGUIDTimeout(bool isMic, const std::string& guid, unsigned long msecTimeout);
        HRESULT PostGetCurrentVolStats(VolumeStats &volStats, unsigned long msecTimeout);
       // User to post scoped message data to dispatcher thread
        template <typename T>
        bool PostScopedMsgToAudioDeviceListener(
            dispatcher_msg_id msg,
            T* const data)
        {
            m_dispatcherThread.Post(this,
                msg,
                new talk_base::ScopedMessageData<T>(data));
            return true;
        }
    private:

        bool SetSessionVolume(float spkVol);
        bool SetSpeakerEndpointVolume(float spkVol);
        bool SetSpeakerSessionMute(bool bMute);
        bool SetSpeakerEndpointMute(bool bMute);
        bool SetMicVolume(float micVol);
        bool SetMicMute(bool bMute);
        HRESULT ProcessMicChange(const std::string& guid);
        HRESULT ProcessSpeakerChange(const std::string& guid);
        //Object to listen for audio end point
        //changes such as arrival or removal
        CComPtr<AudioEndPointNotification> m_audioEndPointNotification;

        //Object to listen for volume changes of speaker
        //It includes system and session volume
        CComPtr<SpeakerVolumeNotification> m_speakerEndPointVolumeNotification;

        //Object to listen mic volume changes
        CComPtr<MicVolumeNotification>     m_micEndPointVolumeNotification;

        // to which audio arrival and removal notification is to be sent.
        Win32DeviceWatcher*                m_deviceWatcher;
        CComPtr<IMMDeviceEnumerator>       m_enumerator;
        bool                               m_bNeedCouninitialize;
        bool                               m_bErrorState;
        VolumeStats                        m_volStat;
        talk_base::Thread                  m_dispatcherThread;
        talk_base::CriticalSection         m_CS;
        HRESULT PrintDeviceName(LPCWSTR  pwstrId);
    };
}
#endif
