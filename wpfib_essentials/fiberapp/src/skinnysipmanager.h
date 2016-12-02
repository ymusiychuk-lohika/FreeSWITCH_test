#ifndef BJN_SKINNY_SIP_MANAGER_H_
#define BJN_SKINNY_SIP_MANAGER_H_

#include "sipmanagerbase.h"
#include "talk/base/logging_libjingle.h"
#include "talk/base/messagequeue.h"
#include "talk/base/windowpickerfactory.h"
#include <pjmedia.h>
#include <pj/errno.h>
#include <pjmedia_videodev.h>
#include <pjmedia_audiodev.h>
#include "VideoCodecFactory.h"
#include "error.h"
#include "bjnhelpers.h"
#include "bjn_cpu_monitor.h"
#include "audiodevconfig.h"

#include "IDeviceController.h"
#include "network_monitor.h"


#define MAX_TIME_LEVEL_DETECTION 15 // Seconds -Count
#define POLL_INTERVAL 150
#define DECAY_POLL_INTERVAL (150 * 7)
#define MAX_AUDIO_DEVS 10

namespace bjn_sky {

struct AudioFeatureEnablements {
    AudioFeatureEnablements() :
        mTypingDetection(true),
        mEnableCustomAec(false),
        mSpeakerAutoUnmute(false),
        mCoreAudioWin(false),
        mAnimatedMicIcon(false),
        mAudioGlitchAutoReset(false),
        mReportAudioDspStats(false),
        mCoreAudioDevNotify(false),
        mAecDelayLogging(false),
        mAecModifiedBuffer(false),
        mCacheRtpSeqTs(false),
        mEnablePostAudioCallbacksMessages(false),
        mEnableDisableAudioEngine(false),
        mDigitalAgcLinux(false),
        mEnableCustomAecExternal(false) {}
	~AudioFeatureEnablements() {};

	void SetAudioFeatures(std::map<std::string, bool> featureMap) {
        std::map<std::string, bool>::const_iterator it;
        for(it = featureMap.begin(); it != featureMap.end(); ++it) {
            if((it->first) == "typing_detection") {
                mTypingDetection = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " << 
                    "typing_detection = " << 
                    (mTypingDetection ? "true":"false");
            }
#ifdef FB_MACOSX
            else if((it->first) == "enable_custom_aec_mac") {
#elif FB_WIN
            else if((it->first) == "enable_custom_aec_win") {
#else
            else if((it->first) == "enable_custom_aec_linux") {
#endif
                mEnableCustomAec = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " << 
                    "enable_custom_aec = " << 
                    (mEnableCustomAec ? "true":"false");
            }
            else if(it->first == "speaker_auto_system_unmute") {
                mSpeakerAutoUnmute = true;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                "mSpeakerAutoUnmute = " <<
                (mSpeakerAutoUnmute ? "true":"false");
            }
#if FB_WIN
            else if((it->first) == "enable_core_audio_win") {
                mCoreAudioWin = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    "enable_core_audio_win = " <<
                    (mCoreAudioWin ? "true":"false");

            }
#endif
            else if((it->first) == "animated_mic_icon") {
                mAnimatedMicIcon = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true":"false");

            }
            else if((it->first) == "audio_glitch_auto_reset") {
                mAudioGlitchAutoReset = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true":"false");
            }
            else if((it->first) == "report_audio_dsp_stats_skinny") {
                mReportAudioDspStats = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true":"false");
            }
            else if ((it->first) == "win_core_audio_dev_notify2") {
                mCoreAudioDevNotify = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true" : "false");
            }
            else if ((it->first) == "enable_post_for_audiocb_messages"){
                mEnablePostAudioCallbacksMessages = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true" : "false");
            }
            else if((it->first) == "aec_delay_logging") {
                mAecDelayLogging = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true":"false");
            }
            else if ((it->first) == "aec_modified_buffer_logic") {
                mAecModifiedBuffer = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true" : "false");
            }
            else if ((it->first) == "cache_audio_rtp_seq_ts") {
                mCacheRtpSeqTs = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true" : "false");
            }
            else if ((it->first) == "enable_disable_audio_engine") {
                mEnableDisableAudioEngine = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                it->first << " = " <<
                (it->second ? "true" : "false");
            }
#if FB_X11
            else if ((it->first) == "digital_agc_linux") {
                mDigitalAgcLinux = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true" : "false");
            }
#endif
            else if((it->first) == "enable_custom_aec_external") {
                mEnableCustomAecExternal = it->second;
                LOG(LS_WARNING) << "SetAudioFeatures() " <<
                    it->first << " = " <<
                    (it->second ? "true" : "false");
            }
        }
	};

	bool mTypingDetection;
	bool mEnableCustomAec;
	bool mSpeakerAutoUnmute;
	bool mCoreAudioWin;
	bool mAnimatedMicIcon;
	bool mAudioGlitchAutoReset;
	bool mReportAudioDspStats;
	bool mCoreAudioDevNotify;
	bool mAecDelayLogging;
	bool mAecModifiedBuffer;
	bool mCacheRtpSeqTs;
	bool mEnablePostAudioCallbacksMessages;
	bool mEnableDisableAudioEngine;
	bool mDigitalAgcLinux;
	bool mEnableCustomAecExternal;
};

AudioFeatureEnablements* getAudioFeatures(void);

enum skinny_msg_id {
    MSG_SET_AUDIO_OPTIONS = MSG_PLATFORM_SPECIFIC+1, // This should always be first
    MSG_SET_AUDIO_CONFIG_OPTIONS,
    MSG_CHECK_SPEAKER,
    MSG_MONITOR_LEVEL,
    MSG_START_SPK_MONITOR,
    MSG_START_MIC_MONITOR,
    MSG_ENDPOINT_VOLUME_CHANGED,
    MSG_SET_SPK_VOLUME,
    MSG_STOP_TEST_SND,
    MSG_DEVICES_CHANGED,
    MSG_INIT_MEDIA,
    MSG_AUTO_MIC_LEVEL_CHECK,
    MSG_ENUMERATE_MONITORS,
    MSG_DESKTOPS_CHANGED,
    MSG_ENUMERATE_APPLICATIONS,
    MSG_SEND_ESM_STATUS,
    MSG_SEND_ECBS_STATUS,
    MSG_SET_FOCUS_APP,
    MSG_START_PRESENTATION2,
    MSG_SET_DEVICE_STATE,
    MSG_SET_DEVICE_MUTE,
    MSG_SETUP_HUDDLE_MODE,
    MSG_REENUMERATE_HUDDLE_DEVICES,
    MSG_NETWORK_INTERFACE_DOWN,
    MSG_NETWORK_INTERFACE_UP,
    MSG_AUD_EVENT_MIC_ANIMATION,
    MSG_ENABLE_AUDIO_ENGINE,
};

enum DeviceArrayIndex
{
    VIDEO_DEVICE_NAME,
    MIC_DEVICE_NAME,
    SPEAKER_DEVICE_NAME,
    VIDEO_DEVICE_ID,
    MIC_DEVICE_ID,
    SPEAKER_DEVICE_ID,

    // Don't add device array indexes after this
    DEVICE_SIZE
};

class SharingDescription
{
public:
    SharingDescription() : mId(0) { };
    SharingDescription(int id, const std::string& screenName, const std::string& screenId)
        : mId(id)
        , mScreenName(screenName)
        , mScreenId(screenId)
    { };
    int                                 mId;
    std::string                         mScreenName;
    std::string                         mScreenId;
    std::string                         mName;
    std::string                         mThumbnailPNG;
    std::string                         mFileDescription;
    std::string                         mProductName;
    std::string                         mIntegrityLevel;
    talk_base::WindowDescriptionList    mWindowList;
};

typedef std::vector<SharingDescription> SharingDescriptionList;

struct ScreenDeviceConfigChange
{
    ScreenDeviceConfigChange(bool layout, bool end)
        : mLayoutChanged(layout), mEndSharing(end) {}
    bool mLayoutChanged;
    bool mEndSharing;
};

struct SipManagerConfig
{
    SipManagerConfig()
        : mMaxBandwidth(0)
        , mCustomConfigXML("")
    {}

    SipManagerConfig(int maxBandWidth, std::string &customConfigXML)
        : mMaxBandwidth(maxBandWidth)
        , mCustomConfigXML(customConfigXML)
    {}
    SipManagerConfig& operator=(const SipManagerConfig& rhs) {
        mMaxBandwidth = rhs.mMaxBandwidth;
        if(!rhs.mCustomConfigXML.empty()) mCustomConfigXML = rhs.mCustomConfigXML;
        return *this;
    }
    int mMaxBandwidth;      // Max configured bandwidth by UI in kbps
    std::string mCustomConfigXML;
};

struct SpeakerMicAutoDetails {
    std::string mMicId;
    std::string mMicName;
    bool mAutoMic;
    std::string mSpeakerId;
    std::string mSpeakerName;

    SpeakerMicAutoDetails() : mAutoMic(false) {
    }

    void clear() {
        mMicId = mMicName = mSpeakerId = mSpeakerName = "";
        mAutoMic = false;
    }

    bool isSpkValid() {
        if (mSpeakerId.empty() || mSpeakerName.empty()) {
            return false;
        }
        return true;
    }

    bool isMicValid() {
        if (mMicId.empty() || mMicName.empty()) {
            return false;
        }
        return true;
    }
};

typedef std::vector<ChangeDevInfo> ChangeDevInfoList;

class skinnySipManager
    : public SipManager
    , public sigslot::has_slots<> {
public:
    skinnySipManager(talk_base::Thread* main_thread,
        talk_base::MessageHandler *browser_msg_hndler,
        talk_base::Thread *browser_thread, const SipManagerConfig& sipManagerConfig);

    bool isMicUSBPair(std::string deviceId, const std::string& classId);
    bool getPreferredDeviceId(bool capture,
                            std::string &selectedDeviceName,
                            std::string &selectedDeviceId);

    bool getAutomaticMic(std::string &micId, std::string &micName, bool bPostMicCheck=true);
    bool getAutomaticSpk(std::string &spkId, std::string &spkName);
    bool getMatchingDevice(std::string deviceId, DeviceConfigType devType,
                           std::vector<Device> &fromDevices, std::string &name, std::string &id);
    virtual ~skinnySipManager();
    void DeinitPJSip();
    bool getDefaultDeviceId(bool capture, std::string &name, std::string &id);
    static int getMeetingInstances();
    void   initializePlatformAudioCodecs();
    void   initializePlatformVideoCodecParams();
    void   initializePlatformAccount();
    void   initializePlatformVideoCodecs();
    void   initializePlatformVideoDevices();
    void   getConfigPath(std::string& path);
    void   platformGetVolumeMuteState(AudioDspMetrics& metrics);
    std::string   getPcapPath();
    void   getPlatformLogfilePathWithGuid(std::string& path);
    void   getUserAgent(std::string& useragent);
    void   setConfigPath(std::string);
    void   setUserAgent(std::string);
    void   GetPlatformDevices();
    void*  getLocalWindow(unsigned);
    void*  getRemoteWindow();
    void   enterBackground();
    void   enterForeground() { return; };
    void   switchSrc(bool) { return; };
    void   platformRegisterThread();
    void   startPreviewCap() { return; };
    void   platformMakeCall(pj_str_t*,pjsua_call_setting*,bool,std::string);
    pj_status_t platformGetErrorStatus();
    bool isScreenSharingDevice(const std::string& screenId);
    static void platformPrintLog(int level, const char *data, int len);
    int platformGetVideoBitrateForCpuLoad();
    int platformGetProcessLoad();
    int platformGetSystemCpuLoad();
    int platformGetCurrentCpuFreqUsage();
    float platformGetFanSpeed();
    void platformGetContentDevice(pjmedia_vid_dev_index &content_idx, pjmedia_vid_dev_index &rend_idx,
                                  const std::string& screenName, const std::string& screenId);
    bool findAndSetAddedDevice();
    bool findAndSetRemovedDevice();
    void DeviceCpy(std::vector<Device> &Loc_Dev, std::vector<cricket::Device> &cric_Dev);
    void platformHandleMessage(talk_base::Message *msg);
    void postSetAudioConfigOptions(AudioConfigOptions& options);
    void postSetAudioOptions(int options);
    void postGetMicVolLvl(int *level);
    void postStopMicVolume();
    void postStartMicVolume();
    void postGetSpeakerVolLvl(int *level);
    void postStopSpeakerVolume();
    void postStartSpeakerVolume();
    void postCheckSpeaker(LocalMediaInfo &selectedDevices);
    void postGetMicrophoneVol(unsigned* vol);
    void postGetMicrophoneMute(bool* status);
    void postGetSpeakerVol(unsigned* vol);
    void postSetSpeakerVol(unsigned* vol);
    void postGetSpeakerMute(bool* status);
    void postSetDeviceState(uint16_t usage, bool isMute);
    void postSetDeviceMute(IDeviceController::muteTypes usage, bool isMute);
    void postInitializeMediaEngine(LocalMediaInfo &selectedDevices);
    void postEnumerateScreenDevices(int width, int height);
    void postEnumerateApplications(int width, int height);
    bool isAppSharingSupported();
    void getPlatformNameServer(pjsua_config *cfg);
    bool platformInit();

    void disableDeviceEnhancements(std::string micId, std::string micName, int devType);
    void SetLocalDevicesOptions(LocalMediaInfo &selectedDeviceoptions);
    int checkSpeaker(LocalMediaInfo &selectedDeviceoptions);
    void monitorSpeakerMic();
    void getSpeakerSettings();
    void platformPreDeviceOperation();
    void platformPostDeviceOperation();
    pj_bool_t platformHangUp(pj_status_t);
    bool platformPresentationEnable(bool enable,
                                    const std::string& screenName,
                                    const std::string& screenId,
                                    uint32_t appId,
                                    uint64_t winId);
    void platformPresentationConfigure(pj_uint32_t appId);

    bool platformOnCallMediaEvent(pjsua_call_id call_id,
                                  unsigned med_idx,
                                  pjmedia_event *event);
    void platformAddSdpAttributes(pjsua_call_id call_id,
                                  pjmedia_sdp_session *sdp,
                                  pj_pool_t *pool);

    void getMicSettings();
    pj_status_t getMicrophoneVol();
    void getMicrophoneMute();
    pj_status_t getSpeakerVol();
    pj_status_t setSpeakerVol(unsigned* spkVol);
    void getSpeakerMute();
    void stopTestSound();
    bool isDeviceSame(Device savedDevice, Device newDevice);
    void postHidEvents(uint16_t usage, bool state);
    bool IsValidDevicesInfo(const std::vector<std::string>& devicesInfo);

#ifdef FB_WIN
    pj_status_t winGetSpeakerVol(std::string id, float* vLevel);
    pj_status_t winGetSpeakerMute(std::string id, bool* muteStatus);
    HRESULT disableDucking();
#elif FB_MACOSX
    void turnOffAllVideoStreams();
#endif
    void OnDevicesChange(bool);
#ifdef WIN32
    void OnVolumeChange(const cricket::VolumeStats& volstat);
    bool GetCurrentVolStats(cricket::VolumeStats &volStats);
    bool SetSpeakerVolumeViaCoreAudio(float spkVol);
#endif
    void OnDesktopsChange(bool layoutChanged, bool endSharing);
    void updateSipmanagerConfig(const SipManagerConfig& sipManagerConfig);
    void setSingleStream(bool singleStream) {
        mSingleStream = singleStream;
    }
    CPU_Monitor& getCPUMonitor() {
        return mCpuMonitor;
    }
    NetworkInfo platformGetNetworkInfo();
    void updateVideoDevice();
    void postMakeCallInternal();
    void postSetFocusApp(uint32_t app_id);
    void setFocusApp(uint32_t app_id);

    std::string getPlatformGuid();
    void platformParseCustomConfigXML();
    void resetSavedSpeakerState();
    void resetSavedMicrophoneState();
    void setReIceInterval();
    void startNetworkMonitor();
    void enableAudioEngine(bool enable);
    void sendAudioEngineState();

private:

    void sendEsmStatus(const pjmedia_event_esm_notify_data &esm);
    void sendEchoCancellerBufferStarvation(const pjmedia_event_ecbs_data &ecbs);
    void handleNetworkUpEvent();
    void handleNetworkDownEvent();
    void updateAutomaticDeviceLabel(
        AudioDeviceSelection micSelection,
        AudioDeviceSelection spkrSelection,
        const std::string& micId,
        const std::string& micName,
        const std::string& spkId,
        const std::string& spkName);
    void updateUsbOnThunderboltMap(const std::string& devName,
                                    const std::string& devGuid,
                                    bool added,
                                    bool capture);
    void updateUsbOnThunderboltMap(const std::vector<Device> &devList,
                                    bool added,
                                    bool capture);
    bool isUsbDeviceInThnMap(const std::string& devGuid) const;

    bool logCPUParams;
    cricket::DeviceManagerInterface* device_manager_;
#ifdef WIN32
    cricket::VolumeStats m_volStats;
#endif
    talk_base::WindowPicker* window_picker_;
    int auto_level_check_count_;
    int level_determined_;
    bool speakerPlay;
    std::vector<std::string> name_server_;
    int      spk_vol_;
    bool     spk_mute_;
    unsigned spk_lvl_;
    int      spk_decay_timer_;
    bool     mic_mute_;
    int      mic_vol_;
    unsigned mic_lvl_;
    int      mic_decay_timer_;
#ifdef WIN32
    HRESULT hr;
    EDataFlow typeDev;
    UINT deviceCount;
    long deviceIndex;
    IMMDevice *device;
    IMMDeviceEnumerator *deviceEnumerator;
    IMMDeviceCollection *deviceCollection;
    LPWSTR pwszID;
    std::wstring oString;
#endif
    IDeviceController *mDeviceController;

private:
    SipManagerConfig mSipManagerConfig;
    CPU_Monitor mCpuMonitor;
    bool mSingleStream; //Enable/Disable dual stream based on CPU. Configured by UI.
    bool mIsHuddleModeEnabled;
    NetworkMonitor *mNetworkMonitor;
    SpeakerMicAutoDetails mAutoSpkMic;
    std::map<std::string, bool> usbOnThnMap;
};

int isXP();
}
#endif
