//
//  sipmanagerbase.h
//
//
//  Created by Deepak on 12/12/12.
//  Copyright (c) 2012 Bluejeans. All rights reserved.
//

#ifndef BJN_SIP_MANAGER_H_
#define BJN_SIP_MANAGER_H_

#include <algorithm>
#include <sstream>
#include "talk/base/thread.h"
#include "talk/base/messagequeue.h"
#include "talk/base/scoped_ptr.h"
#include "talk/session/phone/devicemanager.h"

#ifdef WIN32
#include "talk/session/phone/win32devicemanager.h"
#endif

#include "BwMgr.h"
#include "rtxmanager.h"
#ifdef ENABLE_GD
#include "graceful_events.h"
#endif
#include "pj_utils.h"
#include "rtplogger.h"
#include "error.h"
#include "certificates.h"
#include <pjsua-lib/pjsua.h>
#include <pjsua-lib/pjsua_internal.h>
#include <pjsip/sip_transport.h>
#include <pjsip/sip_transport_multiport_bjn.h>
#include <pjsip-simple/errno.h>
#include <moving_average.h>
#include "subscriptionfactory.h"
#ifdef BJN_WEBRTC_VOE
#include "../../../webrtc/webrtc/bjn_audio_events.h"
#endif
#include "audio_network_monitor.h"
#include <deque>
#include <fbr_cpu_monitor.h>
#include <map>

#ifdef WIN32
#include <endpointvolume.h>
#include <Mmdeviceapi.h>     // MMDevice
#endif

#define LOCALMEDIASTREAM "localMediaStream"
#define REMOTEMEDIASTREAM "remoteMediaStream"
#define REMOTECONTENTSTREAM "remoteContentStream"

#define MAX_GUID_LENGTH 128

#define HD_SUPPORT (1)

#define AUDIO_SDP   "audio"
#define VIDEO_SDP   "video"
#define CONTENT_SDP "content"

#define RECEIVER(dir) (dir == ATTR_DIR_RECVONLY || dir == ATTR_DIR_SENDRECV)
#define SENDER(dir) (dir == ATTR_DIR_SENDONLY || dir == ATTR_DIR_SENDRECV)

#ifdef WIN32
static wchar_t *strVal = L"Uninstall";
#endif

#if !(TARGET_OS_IPHONE || TARGET_IS_ANDROID)
#define ENABLE_RTP_LOGGER
#endif

#define SCREEN_SHARING_FRIENDLY_NAME_T "Screen Share"
#define SCREEN_SHARING_UNIQUE_ID_T "{7306149c-b8c7-4227-9946-6d6316edc64f}"

#define SUB_ID_NONE     0
#define EXPIRES_DEFAULT ((uint32_t)-1)
#define EXPIRES_NONE    0

#define ICE_MIN_PORT   5000
#define ICE_PORT_RANGE 1000
#define AGGRESSIVE_REICE_INTERVAL 5 
#define DEFAULT_MAX_REICE_INTERVAL 20


#define EXCESSIVE_CPU_LOAD 96 // Range 0 - 100

#define CPU_FREQ_INFO_LOG_TIMER 60000
#define FAN_SPEED_INFO_LOG_TIMER 10000

typedef uint32_t SubId_t;

class Config_Parser_Handler;

namespace BJ {
    class PCapReader;
    class PCapWriter;
}

class AbsSendTimeRtpHdrExtSender;

enum local_devices_status{
    LOCAL_DEVICES_OK,
    LOCAL_CAM_BUSY,
    LOCAL_NO_CAM,
};

enum relay_protocol_t {
    RELAY_NONE,
    RELAY_UDP,
    RELAY_TCP,
    RELAY_TCP_VIA,
    RELAY_TLS,
    RELAY_TLS_VIA,
};

const std::string relayProtocolStr[] = {
    "Direct",
    "UDP Relay",
    "TCP Relay",
    "TCP Relay via Proxy",
    "TLS Relay",
    "TLS Relay via Proxy",
};

// Used to represent an audio or video capture or render device.
struct Device {
    std::string name;
    std::string id;
    bool builtIn;
};


enum AudioDeviceSelection {
    AUDIO_DEVICE_SELECT_AUTO = 0,
    AUDIO_DEVICE_SELECT_PAIRED,
    AUDIO_DEVICE_SELECT_MANUAL,
    AUDIO_DEVICE_SELECT_NUM_STATES
};

static const std::string AudioDeviceSelectionStr[AUDIO_DEVICE_SELECT_NUM_STATES] = {
    "auto",
    "paired",
    "manual",
};

class LocalMediaInfo
{
public:
    LocalMediaInfo()
    : video_capture_name("")
    , videocapDevID("")
    , videocapDevVisibility(true)
    , audio_capture_name("")
    , audiocapDevID("")
    , audiocapDevVisibility(true)
    , audio_playout_name("")
    , audioplayoutDevID("")
    , audioplayoutDevVisibility(true)
    , devType(0)
    , devAction(0)
    , audio_capture_selection(AUDIO_DEVICE_SELECT_AUTO)
    , audio_playback_selection(AUDIO_DEVICE_SELECT_AUTO)
    { }

    LocalMediaInfo(const LocalMediaInfo& other) {
        *this = other;
    }

    LocalMediaInfo& operator=(const LocalMediaInfo& other) {
        video_capture_name          = other.video_capture_name;
        videocapDevID               = other.videocapDevID;
        videocapDevVisibility       = other.videocapDevVisibility;
        audio_capture_name          = other.audio_capture_name;
        audiocapDevID               = other.audiocapDevID;
        audiocapDevVisibility       = other.audiocapDevVisibility;
        audio_playout_name          = other.audio_playout_name;
        audioplayoutDevID           = other.audioplayoutDevID;
        audioplayoutDevVisibility   = other.audioplayoutDevVisibility;
        devType                     = other.devType;
        devAction                   = other.devAction;
        audio_capture_selection	    = other.audio_capture_selection;
        audio_playback_selection    = other.audio_playback_selection;

        return *this;
    }

    int getDevType()
    {
        return devType;
    }

    void setDevType(int dType)
    {
        devType=dType;
    }

    int getDevAction()
    {
        return devAction;
    }

    void setDevAction(int dAction)
    {
        devAction=dAction;
    }

    void setVideoCaptureName(std::string name) {
        video_capture_name = name;
    }

    std::string get_Camera()
    {
        return video_capture_name;
    }

    void setVideocapDeviceID(std::string id) {
        videocapDevID = id;
    }

    std::string get_CameraID()
    {
        return videocapDevID;
    }

    void setVideocapDeviceVisibility(bool visibility) {
        videocapDevVisibility = visibility;
    }

    bool get_CameraVisibility()
    {
        return videocapDevVisibility;
    }

    void setAudioCaptureName(std::string name) {
        audio_capture_name = name;
    }

    std::string get_Microphone()
    {
        return audio_capture_name;
    }

    void setAudiocapDeviceID(std::string id) {
        audiocapDevID = id;
    }

    std::string get_MicrophoneID()
    {
        return audiocapDevID;
    }

    void setAudiocapDeviceVisibility(bool visibility) {
        audiocapDevVisibility = visibility;
    }

    bool get_MicrophoneVisibility()
    {
        return audiocapDevVisibility;
    }

    void setAudioPlayoutName(std::string name) {
        audio_playout_name = name;
    }

    std::string get_Speaker()
    {
        return audio_playout_name;
    }

    void setAudioplayoutDeviceID(std::string id) {
        audioplayoutDevID = id;
    }

    std::string get_SpeakerID()
    {
        return audioplayoutDevID;
    }

    void setAudioplayoutDeviceVisibility(bool visibility) {
        audioplayoutDevVisibility = visibility;
    }

    bool get_SpeakerVisibility()
    {
        return audioplayoutDevVisibility;
    }

    void setAudioCaptureSelection(AudioDeviceSelection select) {
        audio_capture_selection = select;
    };

    AudioDeviceSelection getAudioCaptureSelection() {
        return audio_capture_selection;
    };

    void setAudioPlaybackSelection(AudioDeviceSelection select) {
        audio_playback_selection = select;
    };

    AudioDeviceSelection getAudioPlaybackSelection() {
        return audio_playback_selection;
    };

private:
    std::string video_capture_name;
    std::string videocapDevID;
    bool        videocapDevVisibility;
    std::string audio_capture_name;
    std::string audiocapDevID;
    bool        audiocapDevVisibility;
    std::string audio_playout_name;
    std::string audioplayoutDevID;
    bool        audioplayoutDevVisibility;
    int         devType;
    int         devAction;
    AudioDeviceSelection audio_capture_selection;
    AudioDeviceSelection audio_playback_selection;
};

struct CreateRemoteStream{
    std::string stream_label;
    std::string name;
    uint32_t audio_ssrc;
    uint32_t video_ssrc;
    uint32_t med_idx;
};

struct CreateLocalStreams{
    bool success;
    cricket::local_devices_status status;
};

struct IndicationInfo{
    bool presenter;
    std::string presenterGuid;
};

struct LocalDevices {
    std::vector<cricket::Device> audioCaptureDevices;
    std::vector<cricket::Device> audioPlayoutDevices;
    std::vector<cricket::Device> videoCaptureDevices;
};

typedef void (*pj_log_cb) (int level, const char *data, int len);

namespace bjn_sky {
#define ATTR_DIR_SENDONLY "sendonly"
#define ATTR_DIR_RECVONLY "recvonly"
#define ATTR_DIR_SENDRECV "sendrecv"
#define ATTR_DIR_INACTIVE "inactive"
#define ATTR_BW_TIAS      "TIAS"

// map for audio config option passed in from the javascript plugin API
typedef std::map<std::string, int> AudioConfigOptions;
// map for video config option passed in from the javascript plugin API
typedef std::map<std::string, int> DebugOptions;
typedef std::vector<std::string > MeetingFeatureOptions;

struct MeetingFeatures {
    MeetingFeatures()
        : mInitialized(false)
        , mTCPPresentation(true)
        , mTypingDetection(true)
        , mSingleStream(false)
        , mRtcpMux(true)
        , mForceTurn(false)
        , mBjnScreenCodec(false)
        , mNewCpuMonitor(false)
        , mRtxVideo(true)
        , mFecOnUniqueSSRC(false)
        , mTurnTls(true)
        , mMjpegLinux(false)
        , mH264HWAccl(true)
        , mH264CapPub(true)
        , mGpuLogWin(true)
        , mCameraRestart(false)
        , mMacCameraCaps(false)
        , mMacDeferredScreenCapture(false)
        , mMacDeltaScreenCapture(false)
        , mVideoFrameRefCount(false)
        , mHwCcRenderWindows(true)
        , mEnableCustomAec(true)
        , mVIWebRtcBwmgr(true)
        , mVOWebRtcBwmgr(false)
        , mWinScreenShareDD(true)
        , mWebRTCFrameCropping(true)
        , mJoinFlow3Widget(false)
        , mDynamicContentSize(false)
        , mWinDDCaptureComplete(true)
        , mTcpBandwidth(true)
        , mNewNackReqLogic(true)
        , mWinAppShareGlobalHooks(true)
        , mNat64Ipv6(true)
        , mPreferIpv6(false)
        , mRemoteDesktopControl(false)
        , mMaxFreqBasedResCap(true)
        , mBiasedAudioDeviceAutoSelection(true)
        , mDisplayAudioGlitchBanner(false)
        , mWinNetworkMonitor(true)
        , mX11NetworkMonitor(false)
        , mMacNetworkMonitor(false)
        , mCacheAudioRtpSeqTs(true)
        , mLabelAutomaticDevice(false)
        , mEnableDisableAudioEngine(false)
        , mDepreferUsbOnThunderbolt(true)
        , mLogFanSpeed(true)
        , mScreenShareStaticMode(true)
        , mCapturerControlUnderStream(false)
        , mWinBaseBoardManufacturerModel(false)
        , mBwmgrAdjustedTimeouts(true)
        {}

    void Initialize() {
        *this = MeetingFeatures();
        mInitialized = true;
    }

    bool mInitialized;
    bool mTCPPresentation;
    bool mTypingDetection;
    bool mSingleStream;
    bool mRtcpMux;
    bool mForceTurn;
    bool mBjnScreenCodec;
    bool mNewCpuMonitor;
    bool mRtxVideo;
    bool mFecOnUniqueSSRC;
    bool mTurnTls;
    bool mMjpegLinux;
    bool mH264HWAccl;
    bool mH264CapPub;
    bool mGpuLogWin;
    bool mCameraRestart;
    bool mMacCameraCaps;
    bool mMacDeferredScreenCapture;
    bool mMacDeltaScreenCapture;
    bool mVideoFrameRefCount;
    bool mHwCcRenderWindows;
    bool mEnableCustomAec;
    bool mVIWebRtcBwmgr; // As in Denim VI.  A.K.A. vi_bwmgr2
    bool mVOWebRtcBwmgr; // As in Denim VO.  A.K.A. vo_bwmgr2
    bool mWinScreenShareDD;
    bool mWebRTCFrameCropping;
    bool mJoinFlow3Widget;
    bool mDynamicContentSize;
    bool mWinDDCaptureComplete;
    bool mTcpBandwidth;
    bool mNewNackReqLogic;
    bool mWinAppShareGlobalHooks;
    bool mNat64Ipv6;
    bool mPreferIpv6;
    bool mRemoteDesktopControl;
    bool mMaxFreqBasedResCap;
    bool mBiasedAudioDeviceAutoSelection;
    bool mDisplayAudioGlitchBanner;
    bool mWinNetworkMonitor;
    bool mX11NetworkMonitor;
    bool mMacNetworkMonitor;
    bool mCacheAudioRtpSeqTs;
    bool mLabelAutomaticDevice;
    bool mEnableDisableAudioEngine;
    bool mDepreferUsbOnThunderbolt;
    bool mLogFanSpeed;
    bool mScreenShareStaticMode;
    bool mCapturerControlUnderStream;
    bool mWinBaseBoardManufacturerModel;
    bool mBwmgrAdjustedTimeouts;
};

struct sdp_attrs {
    std::string  sdp_media;
    std::string  sdp_attr;
    std::string  sdp_val;
    int          media_idx;
    bool         skip_media;
};

enum msg_id_t {
    MSG_NONE,
    MSG_CLOSE,
    MSG_ADD_LOCAL_RENDERER,
    MSG_REMOVE_LOCAL_RENDERER,
    MSG_ADD_REMOTE_RENDERER,
    MSG_REMOVE_REMOTE_RENDERER,
    MSG_LOGIN,
    MSG_LOGOUT,
    MSG_CHANGE_DEVICE,
    MSG_LOGIN_TIMEOUT,
    MSG_PRESENTATION_REQ,
    MSG_PRESENTATION_REL_REQ,
    MSG_ENUM_DEVICE,
    MSG_INIT_CONFIG_PATH,
    MSG_INIT_USERAGENT,
    MSG_MAKE_CALL,
    MSG_SET_CAPDEV_OFF,
    MSG_SET_CAPDEV_ON,
    MSG_REFRESH_SCREEN_CAPTURE,
    MSG_SET_LOG_LEVEL,
    MSG_SET_PJLOG_LEVEL,
    MSG_SEND_MEDIA_INVITE,
    MSG_EXIT_SKINNY,
    MSG_SET_PRESENTATION_RATIO,
    MSG_REG_EVENT,
    MSG_SUBSCRIBE_EVENT,
    MSG_UNSUBSCRIBE_EVENT,
    MSG_PRESENTATION_START,
    MSG_PRESENTATION_STOP,
    MSG_SEND_DEVICE_GUID,
    MSG_REINIT_VID_CODECS,
    MSG_HANGUP,
    MSG_SEND_AUDI_DEVICE_WARN,
    MSG_MUTE_SPEAKER,
    MSG_TMMBR_RECV,
    MSG_NETEVENT,
    MSG_LOG_RTP_STATS,
    MSG_AUDIO_ONLY_MODE,
    MSG_ENABLE_MEETING_FEATURES,
    MSG_ENABLE_MEDIA_STREAMS,
    MSG_SEND_GRACEFUL_EVENT_NOTIFICATION,
    MSG_SET_GRACEFUL_EVENT_NOTIFICATION_FILTER,
    MSG_SET_GRACEFUL_EVENT_NOTIFICATION,
    MSG_PJSUA_POLL_EVENTS,
    MSG_STOP_MED_SESSION,
    MSG_SIP_KA_TIMER,
    MSG_BWMGR_TIMER,
    MSG_UPDATE_REMOTE_RENDERER,
    MSG_CPU_LOAD_TIMER,
    MSG_RTCP_RX_STATS,
    MSG_RTCP_TX_STATS,
    MSG_SET_DEBUG_OPTIONS,
    MSG_APPLY_MEDIA_BW,
    MSG_SEND_TMMBR,
    MSG_MEDIA_SIZE_CHANGE,
    MSG_MEDIA_ENC_FORMAT_CHANGE,
    MSG_MEDIA_HW_CODEC_DISABLE,
    MSG_MEDIA_UPDATE_CUR_FORMAT,
    MSG_ENABLE_REFERENCE_COUNTING,
    MSG_INIT_AUDIO_CONFIG_OPTIONS,
    MSG_RESTART_MEDIA_SESSIONS,
    MSG_RDC_REQUEST,
    MSG_RDC_RELEASE,
    MSG_RDC_GRANT_STATUS,
    MSG_RDC_SEND_INPUT,
    MSG_SEND_AUDIO_DSP_STATS,
    MSG_ICE_COMPLETE,
    MSG_SEND_EC_MODE,
    MSG_GRACEFUL_AUD_EVENT_NOTIFICATION,
    MSG_AUD_EVENT_VAD_NOTIFY,
    MSG_PROCESS_AUD_NETSTATS,
    MSG_PLATFORM_SPECIFIC, /* This should be the last message */
};

enum browser_msg_id_t {
    APP_MSG_NONE,
    APP_MSG_CREATE_LOCAL_STREAM,
    APP_MSG_PC_CREATE_REMOTE_STREAM,
    APP_MSG_NOTIFY_LOCAL_DEVICES,
    APP_MSG_PC_REMOVE_REMOTE_STREAM,
    APP_MSG_NOTIFY_LOCAL_DEVICES_CHANGE,
    APP_MSG_PRESENTATION_REQ_RESP,
    APP_MSG_PRESENTATION_IND,
    APP_MSG_PC_CALL_STATUS,
    APP_MSG_PC_P2P_CALL_STATUS,
    APP_MSG_DEVICE_ENHANCEMENTS,
    APP_MSG_SPK_FAIL,
    APP_MSG_FAULTY_SPEAKER_NOTIFY,
    APP_MSG_FAULTY_MICROPHONE_NOTIFY,
    APP_MSG_SIMULATE_MOUSE_EVENT,
    APP_MSG_INIT_COMPLETE,
    APP_MSG_SWITCH_SRC_COMPLETE, // TODO OS specific
    APP_MSG_NOTIFY_WARNING,
    APP_MSG_CONTENT_SIZE_CHANGE,
    APP_MSG_VIDEO_SIZE_CHANGE,
    APP_MSG_CAPTURE_DEV_ON_COMPLETE,
    APP_MSG_CAPTURE_DEV_OFF_COMPLETE,
    APP_MSG_CONTENT_STREAM_ADD_COMPLETE,
    APP_MSG_CONTENT_STREAM_REMOVE_COMPLETE,
    APP_MSG_SCREEN_CAPTURE_DEV_REFRESH_COMPLETE,
    APP_MSG_NOTIFY_EVENT,
    APP_MSG_CREATE_COMPLETE,
    APP_MSG_NOTIFY_VAD_IND,
    APP_MSG_UNSUPPORTED_AUDIO_DEVICE,
    APP_MSG_NOTIFY_CALL_DISCONNECT,
    APP_MSG_CONTENT_IND,
    APP_MSG_AUDIO_MUTE_STATE,
    APP_MSG_VIDEO_MUTE_STATE,
    APP_MSG_SCREEN_CAPTURE_LIST_UPDATE,
    APP_MSG_NOTIFY_SCREEN_CAPTURE_DEVICES_CHANGE,
    APP_MSG_APPLICATION_LIST_UPDATE,
    APP_MSG_NOTIFY_NO_RTP_MEDIA,
    APP_MSG_MAKE_CALL_INTERNAL,
    APP_MSG_GRACEFUL_EVENT_NOTIFICATION,
    APP_MSG_REMOTE_VIEW_AVAILABLE,
    APP_MSG_CONTENT_VIEW_AVAILABLE,
    APP_MSG_CALLID_UPDATE,
    APP_MSG_GET_ADDR_INFO,
    APP_MSG_RDC_REQUEST,
    APP_MSG_RDC_RELEASE,
    APP_MSG_RDC_GRANT_STATUS,
    APP_MSG_RDC_INDICATION,
    APP_MSG_RDC_INPUT_RECIEVED,
    APP_MSG_NOTIFY_DEVICE_STATE,
    APP_MSG_NOTIFY_DEVICE_OPERATION_STATUS,
    APP_MSG_MIC_ANIMATION_STATE,
    APP_MSG_FIBER_EVENT,
    APP_MSG_UPDATE_DEVICE_LIST,
    APP_MSG_CAPTURE_STATUS
};


enum fiber_event_t {
    FBR_EV_NONE,
    FBR_EV_REICE_TRIGGERED,
    FBR_EV_TRANSPORT_DISCONNECTED,
    FBR_EV_NW_INTERFACE_ADDED,
    FBR_EV_NW_INTERFACE_REMOVED,
    FBR_EV_AUDIO_RTP_STOPPED,
    FBR_EV_AUDIO_RTP_RECOVERED,
    FBR_EV_VIDEO_RTP_STOPPED,
    FBR_EV_VIDEO_RTP_RECOVERED,
    FBR_EV_CONTENT_RTP_STOPPED,
    FBR_EV_CONTENT_RTP_RECOVERED,
    FBR_EV_MAX,
};

enum capturer_dev_status_t {
    CAPTURER_START_SUCCESS,
    CAPTURER_IN_USE,
    CAPTURER_FAILURE = 0xFF
};

struct TurnServerInfo
{
    TurnServerInfo()
        : address("")
        , protocol("")
        , port(0)
        , username("")
        , password("")
        , fqdn("")
        , priority(0)
    {
    }

    std::string address;
    std::string protocol;
    int         port;
    std::string username;
    std::string password;
    std::string fqdn;
    uint16_t priority;
};

struct regEvent
{
    std::string name;
    uint32_t def_expires;
    std::string mimetype;
    std::string mimesubtype;
};

struct subscribeEvent
{
    std::string event_name;
    std::string body;
};

struct InviteParam
{
    pjsua_call_id call_id;
    sdp_attrs *sdp_array;
    int size_sdp_attrs;
    pj_uint32_t format_id;
};

struct DisconnectParam
{
    pjsua_call_id call_id;
    int reason_code;
    std::string reason;
};

struct VideoCodecParams
{
    struct Params
    {
        int width;
        int height;
        int fps;
        int bitrate;

        Params(): width(0)
                , height(0)
                , fps(0)
                , bitrate(0)
        {
        }
    };

    struct QPParams
    {
        int minQP;
        int maxQP;
        int minQPContent;
        int maxQPContent;
        int minQPRDC;//for remote desktop control
        int maxQPRDC;
        int videoFreeRangeQPBitrate;

        QPParams(): minQP(-1)
                  , maxQP(-1)
                  , minQPContent(-1)
                  , maxQPContent(-1)
                  , minQPRDC(-1)
                  , maxQPRDC(-1)
                  , videoFreeRangeQPBitrate(-1)
        {
        }
    };

    struct ContentParams
    {
        int contentHighResBitrate;

        ContentParams(): contentHighResBitrate(-1)
        {
        }
    };

    Params enc;
    Params dec;
    QPParams VP8;
    ContentParams VP8Content;
};

struct make_call{
    std::string name;
    std::string uristr;
    std::vector<TurnServerInfo> servers;
    bool forceTurn;
    std::string proxyInfo;
    std::string proxyAuthStr;
};

struct msg_call_status
{
    int callStatus;
    pjsua_call_id call_id;
};

struct screen_share_param{
    std::string name;
    std::string id;
    uint32_t appId;
    uint64_t winId;
};

typedef std::deque<InviteParam> invite_queue;

enum dev_Type {
    DEVNONE,
    AUDIO_IN,
    AUDIO_OUT,
    VIDEO,
};

static const char *dev_type_str[] = {
    "NONE",
    "AUDIO_IN",
    "AUDIO_OUT",
    "VIDEO",
};

enum dev_Action {
    ACTNONE,
    ADDED,
    REMOVED,
    UPDATED
};

static const char *dev_action_str[] = {
    "NONE",
    "ADDED",
    "REMOVED",
};

static const char *esm_cause_str[] = {
    "AEC Converged",
    "No Echo Detected",
    "Echo Detected",
};

struct msg_audio_only {
    bool enable;
    bool videoSrcUnmute;
};

struct MediaStreamsDirection {
    bool audioSend;
    bool audioRecv;
    bool videoSend;
    bool videoRecv;
    bool disableContentStream;

    MediaStreamsDirection():audioSend(false),audioRecv(false),
                            videoSend(false),videoRecv(false),
                            disableContentStream(false)
    {
    }
};

struct ChangeDevInfo {
    dev_Type devType;
    dev_Action devAction;
    std::string devGuid;
    std::string devName;
};

static const int GRACEFUL_EVENT_DELAY_MS = 250;

class SystemCpuMonitor;

// These numbers once assigned should not change.
enum StatIdentifier
{
    SI_UNDEFINED = 0,
    SI_SYSTEM_LOAD = 1,
    SI_PROCESS_LOAD = 2,
    SI_WIFI_RECV_STRENGTH = 3,
    SI_ESM_STATUS = 4,
    SI_ESM_XCORR_METRIC = 5,
    SI_ESM_ECHO_PRESENT_METRIC = 6,
    SI_ESM_ECHO_REMOVED_METRIC = 7,
    SI_ESM_ECHO_DETECTED_METRIC = 8,
    SI_ESM_ECHO_BUFFER_STARVATION = 9,
    SI_ESM_ECHO_BUFFER_STARVATION_DELAY = 10,
    SI_SPEAKING_WHILE_MUTED_EVENT = 11,
    SI_CAUSING_LOCAL_ECHO_EVENT = 12,
    SI_EXCESSIVE_CPU_LOAD_EVENT = 13,
    SI_RECEIVE_NETWORK_PROBLEM_EVENT = 14,
    SI_TRANSMIT_NETWORK_PROBLEM_EVENT = 15,
    SI_ESM_ECHO_BUFER_STARVATION_EVENT = 16,
    SI_SYS_FREQ_LOAD = 17,

#ifdef BJN_WEBRTC_VOE
	// WebRTC DSP Stats
    // AEC
    SI_AEC_LINEAR_ERLE_MAX = 20,
    SI_AEC_FILTERED_DELAY_MEAN = 22,
    SI_AEC_PRIMARY_LATENCY_MEAN = 25,
    SI_AEC_RESET_COUNTER_MAX = 29,
    SI_AEC_BUFFERED_DATA_MIN = 30,
    SI_AEC_RAW_SKEW_MIN = 33,
    SI_AEC_RAW_SKEW_MAX = 35,
    SI_AEC_FILTERED_SKEW_MEAN = 37,

    // AGC
    SI_AGC_MICROPHONE_LEVEL_MEAN = 40,
    SI_AGC_DIGITAL_GAIN_MEAN = 43,
    SI_AGC_SATURATION_EVENTS_MEAN = 46,
    SI_AGC_ADAPTATION_GATING_MEAN = 49,

    // ESM
    SI_ESM_NOISE_LEVEL_MAX = 53,
    SI_ESM_SIGNAL_LEVEL_MAX = 56,
    SI_ESM_TYPING_EVENT_MEAN = 58,
    SI_ESM_SPKR_LEVEL_MAX = 59,

    // Microphone and Speaker Settings
    SI_SPK_MUTE_STATE = 60,
    SI_SPK_VOLUME_STATE = 61,
    SI_MIC_MUTE_STATE = 62,
    SI_MIC_VOLUME_STATE = 63,

    // Jitter Buffer Statistics
    SI_JITTER_CURRENT_BUFFER_SIZE_MS = 64,
    SI_JITTER_PREFERRED_BUFFER_SIZE_MS = 65,
    SI_JITTER_JITTER_PEAKS_FOUND = 66,
    SI_JITTER_CURRENT_PKT_LOSS = 67,
    SI_JITTER_CURRENT_DISCARD_LOSS = 68,
    SI_JITTER_CLOCKDRIFT_PPM = 72,
    SI_JITTER_MEAN_WAITING_TIME_MS = 73,
    SI_JITTER_MEDIAN_WAITING_TIME_MS = 74,
    SI_JITTER_MIN_WAITING_TIME_MS = 75,
    SI_JITTER_ADDED_SAMPLES = 76,

    // SKY-5005 AUDIO GLITCH
    SI_AUDIO_DRIVER_GLITCH_EVENT = 77,
#endif
    // FBR-537: Graceful Degradation for Choppy AO Stream
    SI_CHOPPY_RX_CLIENT = 78,
    SI_CHOPPY_TX_CLIENT = 79,

    // Fan Speed statistics
    SI_FAN_SPEED = 80,
};

// Derived to provide constructors/mutators to facilitate setting items
struct StatElement : public bjn_stat_element
{
    StatElement(StatIdentifier id, float val) { SetFloat(id, val); }
    StatElement(StatIdentifier id, int32 val) { SetInt(id, val); }

    StatElement& operator()(StatIdentifier id, float val) {
        SetFloat(id, val);
        return *this;
    };

    StatElement& operator()(StatIdentifier id, int32 val) {
        SetInt(id, val);
        return *this;
    };

    void SetFloat(StatIdentifier id, float val)
    {
        fval = val;
        identifier = (uint8)id | 0x80;
    }

    void SetInt(StatIdentifier id, int32 val)
    {
        ival = val;
        identifier = id;
    }
};

typedef struct NetworkInfo {
    static const int stat_ok; /*(0)*/
    static const int stat_uninitialized; /*(-1)*/
    static const int stat_unknown; /*(-2)*/
    int wlanSignalQuality; /*strength(0-100)*/
    int connectionStatus; /*stat_ok(0), stat_uninitialized(-1), stat_unknown(-2)*/
    NetworkInfo():wlanSignalQuality(0),connectionStatus(NetworkInfo::stat_uninitialized){}
} NetworkInfo;

static bool operator ==(const NetworkInfo &lhs, const NetworkInfo &rhs)
{
    return lhs.wlanSignalQuality == rhs.wlanSignalQuality &&
        lhs.connectionStatus == rhs.connectionStatus;
}

static inline bool operator !=(const NetworkInfo &lhs, const NetworkInfo &rhs)
{
    return !(lhs == rhs);
}

struct ChannelTransport
{
    AbsSendTimeRtpHdrExtSender *mAbsSendTimeHdrExtSender;
    BJ::PCapReader    *mPcapReader;
    pjmedia_transport *mPcapSaver;
    relay_protocol_t mRelayCall;

    ChannelTransport();
    ~ChannelTransport();
};

class SipManager
    : public talk_base::MessageHandler
    , protected LocalMediaInfo
    , public RtxEventHandler
{
public:
    SipManager(talk_base::Thread* main_thread,
               talk_base::MessageHandler *browser_msg_hndler,
               talk_base::Thread *browser_thread, pj_log_cb log_cb);

    virtual ~SipManager();
    void initconfigpath(std::string);
    void inituseragent(std::string);

    inline VideoCodecParams& getVideoCodecParams()
    {
        return video_codec_params_;
    }

    void setVideoCodecEncParams(int width, int height, int fps)
    {
        video_codec_params_.enc.width       = width;
        video_codec_params_.enc.height      = height;
        video_codec_params_.enc.fps         = fps;
    }
    void  initializeVideoCodecs();
    virtual void OnMessage(talk_base::Message *msg);
    void postAudioOnlyMode(bool enable, bool videoSrcUnmute);
    void callMediaDirection(MediaStreamsDirection mediaDir);
    void callMediaDirection(bool audio, bool video, bool videoSrcUnmute, bool content, bool disableContent = false);
    void callMediaSendReInvite(std::string audioDirection,
                               std::string videoDirection,
                               std::string contentDirection);
    void prepareAndSendReinvite(std::string audioDirection,
                                std::string videoDirection,
                                std::string contentDirection);
    void applyVideoDirection(std::string direction, media_index med_idx);
    void applyAudioDirection(std::string direction);
    void postGetDevices();
    void GetDevices();
    void PrintDevices(const std::vector<Device>& names);
    void InitPJSip();
    void InitSipManager();
    void Quit(unsigned int reason = PJSIP_SC_OK);
    void ExitSipUA();
    void postClose();
    void postExitSipUA();
    void postNetworkEvent(bool recreate, bool reinitMedia, int delayMs=0);
    void postMakeCall(std::string name,
                      std::string uri,
                      std::vector<TurnServerInfo>& servers,
                      bool forceTurn,
                      std::string proxyInfoStr,
                      std::string proxyAuthStr);

    void makeCall(std::string name,
                  std::string uri,
                  std::vector<TurnServerInfo>& servers,
                  bool forceTurn,
                  std::string proxyInfoStr,
                  std::string proxyAuthStr);
    void destroyExistingTransports(bool recreate);
    void startMediaSessions();
    void stopMediaSession(int med_idx);
    void onCallMediaState(pjsua_call_id call_id);
    void onCallMediaEvent(pjsua_call_id call_id,
                                 unsigned med_idx,
                                 pjmedia_event *event);
    void onCallTsxEvent(pjsua_call_id call_id,
                  pjsip_transaction *tsx,
                  pjsip_event *e);
    void sendCallTsxEventResponse(int status_code,
                                  pjsua_call_id call_id,
                                  pjsip_rx_data *rdata,
                                  pjsip_transaction *tsx);

    // handled sip transactions
    pj_status_t xmlPresentationControl(pjsua_call_id call_id,
                                       const pj_str_t *xml_st);
    pj_status_t xmlRdcControl(pjsua_call_id call_id, const pj_str_t *xml_st);

    void onCallState(pjsua_call_id call_id, pjsip_event *e);
    void onIncomingCall(pjsua_acc_id   acc_id,
                        pjsua_call_id  call_id,
                        pjsip_rx_data* rdata);
    void keepAliveTimer();
    void initPJSipTransport(pjsip_transport_type_e tp_type);
    void closeAndRecreateTransport();
    void restartKeepAliveTimer();
    void bwMgrTimer();
    void cpuLoadTimer();
    void postTokenRelease();
    void postTokenReq();
    void postPresentationStart(const std::string& screenName,
                               const std::string& screenId,
                               uint32_t appId,
                               uint64_t winId);
    void postPresentationStop();

    virtual void  getPlatformLogfilePathWithGuid(std::string& path);
    virtual std::string getPlatformGuid() = 0;
    virtual void  getConfigPath(std::string& path)=0;
    virtual std::string  getPcapPath()
    {return ""; };
    virtual void  setConfigPath(std::string)=0;
    virtual void  setUserAgent(std::string)=0;
    virtual void  GetPlatformDevices()=0;
    virtual void  getUserAgent(std::string& useragent)=0;
    virtual void platformMakeCall(pj_str_t*,pjsua_call_setting*,bool,std::string)=0;
    virtual pj_status_t platformGetErrorStatus()=0;
    virtual bool isScreenSharingDevice(const std::string& screenId)=0;

    virtual int platformGetVideoBitrateForCpuLoad()=0;
    int getCpuLoadTimerDurationInMilliSec() const;
    virtual int platformGetSystemMaxCpuThreshold() { return MAX_CPU_THRESHOLD; };
    virtual int platformGetSystemExcessiveCpuLoadLimit() { return EXCESSIVE_CPU_LOAD; };
    virtual int platformGetSystemCpuLoad() = 0;
    virtual int platformGetProcessLoad() = 0;
    virtual float platformGetFanSpeed() {return 0.0f;}
    virtual int platformGetCurrentCpuFreqUsage() = 0;
    virtual bool platformGetTurnServerReachability() { return false; };
    virtual NetworkInfo platformGetNetworkInfo() = 0;
    virtual void platformGetContentDevice(
                    pjmedia_vid_dev_index &content_idx,
                    pjmedia_vid_dev_index &rend_idx,
                    const std::string& screenName,
                    const std::string& screenId) = 0;
    virtual void * platformVideoUnmuteComplete() { return NULL; }
    virtual void platformHandleMessage(talk_base::Message *msg);
    virtual void startNetworkMonitor() {return;}
    void registerMainThread();

    void postRemoveLocalRenderer();
    void postAddLocalRenderer(void *window);
    void postRemoveRemoteRenderer(int med_idx);
    void postAddRemoteRenderer(int med_idx);
    void postUpdateRemoteRenderer(void *window,int med_idx);
    void postMuteVideo(bool enable);
    void postMute(bool enable);
    void postChangeDevices(LocalMediaInfo &selectedDevices);
    void setEndpointLoggingPathAndGuid(void);
    void SetUiInfo(std::string& uiInfo);
    void postSetPresentationRatio(float video, float content);
    void setPresentationRatio(float video, float content);

    void postFaultyMicrophoneNotification();
    void postFaultySpeakerNotification();
    void postCaptureDevOff();
    void postCaptureDevOn();
    void setCaptureDevOff();
    void setCaptureDevOn();
    void setLocalViewFlag(bool val) { localViewFlag = val; }
    pj_uint8_t isCapturerOn() { return capturerOn; }
    bool videoMuted();
    bool appVideoMuted();
    bool audioMuted();
    bool isContentBeingShared();
    bool updateCodecParam(int width, int height,int fps, int med_idx);
    pjsua_call_id getCallId() { return call_id_; }
    pjmedia_vid_dev_index getCapDev() { return cap_idx_;}
    bool sendReInvite(pjsua_call_id call_id,
                      bool pushInvite,
                      sdp_attrs[], int size_sdp_attrs, pj_uint32_t format_id = PJMEDIA_FORMAT_UNDEF);
    void setSelSpeakerGUID(std::string guid) { speakerGUID = guid; }
    std::string getSelSpeakerGUID() { return speakerGUID; }
    void postRefreshScreenCapture(pjmedia_rect_size size);
    void refreshScreenCapture(pjmedia_rect_size size);
    void mediaSizeChangeEvent(pjmedia_event_fmt_changed_data fmt_changed, unsigned med_idx);
    pj_status_t mediaEncoderFormatChangeEvent(pjsua_call_id call_id, pjmedia_format_id format_id);
    void mediaHwCodecDisableEvent(pjsua_call_id call_id, pjmedia_format_id format_id);
    void postSetLogLevel(const unsigned int level);
    void postSetPJLogLevel(int level);
    bool isEncryptedCall() { return encryptionSupport; }
    void addContentStream(bool start,
                          const std::string& screenName,
                          const std::string& screenId,
                          pj_uint32_t appId,
                          std::string& contentDirection);
    void onCallSDPCreated(pjsua_call_id call_id,
                          pjmedia_sdp_session *local_sdp,
                          pj_pool_t *pool,
                          const pjmedia_sdp_session *rem_sdp);
    void onStreamDestroyed(pjsua_call_id call_id,
                          pjsua_call_media *call_med,
                          unsigned stream_idx);
    void onStreamCreated(pjsua_call_id call_id,
                          pjsua_call_media *call_med,
                          unsigned stream_idx,
                          pjmedia_port **p_port);
    pjmedia_transport* onCreateMediaTranport(
                          pjsua_call_id call_id,
                          unsigned media_idx,
                          pjmedia_transport *base_tp,
                          unsigned flags);
    void postRegisterEvent(std::string name,uint32_t def_expires,
                           std::string mimetype,std::string mimesubtype);
    void postSubscribe(std::string event_name,std::string body);
    void postUnsubscribe(std::string event_name);
    void eventNotify(std::string event_name, std::string notify_body);
    void getBandwidthInfo(std::string &bwInfoStr);
    void postReinitVideoCodecs();
    const Config_Parser_Handler& getConfigParser() { return *cph_instance; }
    void setDelayBandwidth(bool enable);
    void *getLocalWindow() {
        return local_window_;
    }
    void postMuteSpeaker(bool val);
    virtual void platformAddRemoteRenderer(int med_idx) {return;}
    void enableTCPPresentation(bool tcpPresentation) {
        mMeetingFeatures.mTCPPresentation = tcpPresentation;
    }
    relay_protocol_t isTCPRelay(pjsua_call_id call_id, int med_idx);
    relay_protocol_t getRelayCall(int med_idx) const {
        return mTransports[med_idx].mRelayCall;
    }
    void onTransportState(pjsip_transport* tp, pjsip_transport_state state,
                          const pjsip_transport_state_info* info);
    void onIceTransportError(pj_ice_strans_op op,
                             pj_status_t status,
                             void *param);
    void postHangUp(pjsua_call_id call_id, int reason=PJSIP_SC_OK, std::string reasonStr="Normal Call Clearing");
    void changeSDPForTCPRelay(pjmedia_sdp_media *m, unsigned mi, std::string &protocol);
    void addMediaAttrWithPTToSDP(pjmedia_sdp_media *m, pjmedia_format_id formatId);
    void removeMediaAttrWithPTFromSDP(pjmedia_sdp_media *m, const char *name, pj_str_t rtpPTStr);
    bool changeContentSDP(pjsua_call_id call_id, pjmedia_sdp_media *m);
    void changeVideoSDP(pjmedia_sdp_media *m, pj_pool_t *pool);
    bool canReIce();

    // Posts event to "client"/sip manager thread.
    void postToClient(unsigned msg_id)
    {
        if (client_thread_)
            client_thread_->Post(this, msg_id);
    }

    template <typename T>
    void postToClient(unsigned msg_id, const T &item)
    {
        if (client_thread_) {
            client_thread_->Post(this, msg_id,
                             new talk_base::TypedMessageData<T>(item));
        }
    }

    template <typename T>
    void postToClientDelayed(int msDelay, unsigned msg_id, const T &item)
    {
        if (client_thread_) {
            client_thread_->Clear(this, msg_id);
            client_thread_->PostDelayed(msDelay, this, msg_id,
                             new talk_base::TypedMessageData<T>(item));
        }
    }

    // posts event to the "browser"/main app thread
    void postToBrowser(unsigned msg_id)
    {
        browser_thread_->Post(browser_msg_hndler_, msg_id);
    }

    template <typename T>
    void postToBrowser(unsigned msg_id, const T &item)
    {
        browser_thread_->Post(browser_msg_hndler_, msg_id,
                             new talk_base::TypedMessageData<T>(item));
    }

    std::string getTurnServer(pjsua_call_id call_id, int med_idx);

    // RtxEventHandler Interface
    virtual void OnRtxEnablementChange(bool newState);
    virtual void OnRtxDelayChange(uint32_t minDelayMs);
    virtual void OnRtxOutboundLossRateChange(float lossRate);
    virtual void OnRTTChange(uint32_t avgRTT);
    virtual void OnRtxNumNackRetransmitChange(uint32_t retransmits);
    virtual void OnRtxNackIntervalChange(uint32_t retransmitInterval);

    void postSetDebugOptions(DebugOptions options);

    const MeetingFeatures& GetMeetingFeatures() const { return mMeetingFeatures; }

    void preCacheMeetingFeaturesList(const MeetingFeatureOptions& meetingFeatures);

    bool IsFecTemporalAvailable(void) const
    {
        return mFecTemporalAvailable;
    }

    void clearProxyConfig();
protected:
    virtual void initializePlatformAudioCodecs()=0;
    virtual void initializePlatformVideoCodecParams()=0;
    virtual void initializePlatformAccount()=0;
    virtual void initializePlatformVideoCodecs()=0;
    virtual void initializePlatformVideoDevices()=0;
    virtual void platformGetVolumeMuteState(AudioDspMetrics& metrics) { return; }
    virtual void getPlatformNameServer(pjsua_config *cfg) { return; }
    virtual bool platformInit() {return true; }
    virtual void platformParseCustomConfigXML() { return; }

    virtual void platformPreDeviceOperation() { return; }
    virtual void platformPostDeviceOperation() { return; }
    virtual bool platformPresentationEnable(bool enable,
                                            const std::string& screenName,
                                            const std::string& screenId,
                                            uint32_t appId,
                                            uint64_t winId)=0;
    virtual void platformPresentationConfigure(pj_uint32_t appId) { return; }

    // Allows derived class to handle pjmedia_events.  An implementor
    // should return true if it handled the event.handled, false others
    virtual bool platformOnCallMediaEvent(pjsua_call_id call_id,
                                  unsigned med_idx,
                                  pjmedia_event *event)
    {
        return false;
    }

    virtual void platformAddSdpAttributes(pjsua_call_id call_id,
                                          pjmedia_sdp_session *sdp,
                                          pj_pool_t *pool) { return; }

    void  addRemoteRenderer(int med_idx);
    void  updateRemoteRenderer(void *window, int med_idx);
    void  addLocalRenderer(void *window);
    void  initializeVideoCodecParams();
    void  initializeAccount(std::string name);
    void  initializeAudioCodecs();
    void  initializeVideoDevices();
    void HangUp(pjsua_call_id call_id, int reason=PJSIP_SC_OK, std::string reasonStr="Normal Call Clearing");

    void ChangeDevices(LocalMediaInfo &newDeviceOptions);

    void OnCreateLocalStream(bool success, cricket::local_devices_status status);
    void OnUpdateAppDeviceList(LocalMediaInfo& devices);
    bool FindDeviceIdx(std::vector<Device> devices, std::string id, int *idx);
    bool isDeviceBuiltIn(std::vector<Device> devices, std::string id);
    bool FindAudioDeviceIdx(const char *drv_name,
                            const char *dev_name,
                            pjmedia_aud_dev_index *id,
                            bool capture);
    bool FindVideoDeviceIdx(const char *drv_name,
                            const char *dev_name,
                            pjmedia_vid_dev_index *id,
                            pjmedia_dir direction);

    void muteAudioSrc(bool mute);
    void muteVideoSrc(bool mute);
    void muteAudioSink(bool mute, bool req_reinvite);
    pjsua_vid_win_id getIncomingVideoWid(pjsua_call_id call_id, int med_idx);
    pj_status_t sendReinvite(pjsua_call_id call_id, const pjmedia_sdp_session *sdp,
                             sdp_attrs sdp_array[], int size_sdp_attrs);
    pj_status_t createSdp(pjsua_call_id call_id, pjmedia_sdp_session **p_sdp,
                          sdp_attrs[], int size_sdp_attrs);
    pj_status_t modifySdpAttribute(pjsua_call *call, pj_pool_t *pool,
                                   pjmedia_sdp_session *sdp,
                                   sdp_attrs[], int size_sdp_attrs);
    bool isAudioDeviceChanged(LocalMediaInfo &selectedDeviceoptions);

    // presentation related calls
    bool sendTokenRequest(pjsua_call_id call_id);
    bool sendTokenRelease(pjsua_call_id call_id);
    bool sendPresentationMessage(pjsua_call_id call_id, const std::string& msgType);
    bool buildPresentationMessageContentBody(const std::string& primitive, std::string& msg);

    bool sendSystemInfoMessage(pjsua_call_id call_id, const std::string& msgType);
    bool buildSystemInfoMessageContentBody(const std::string& element, std::string& msg);

    bool sendRdcMessage(pjsua_call_id call_id, const std::string& msgType, const std::string& params);
    bool buildRdcMessageContentBody(const std::string& primitive, const std::string& params, std::string& msg);
    bool xmlParseTag(const pj_str_t *xml_st, const char* tag, std::string& value);
    bool sendRdcUserInput(bjn_rdc_input* input);
    void updateRdcPresentationCodecConfig();

    // Wraps the sending of requests
    pj_status_t sendSipRequestWithPayload(pjsua_call_id call_id, const char *method
        , const char *contentType, const char *payload);

    bool deviceExist();
    void reset();
    bool enableMedTx(bool enable, int med_idx);
    bool enableMedRx(bool enable, int med_idx);
    bool clearReInviteFromQ(InviteParam &invite_param);
    bool mediaStatus[MAX_MED_IDX];
    char* pjStrToChar(pj_str_t* str);
    void HandleInviteSubscription (pjsua_call_id call_id, pjsip_rx_data *rdata);
    void registerEvent(std::string name,uint32_t def_expires,
                       std::string mimetype,std::string mimesubtype);
    void subscribe (std::string event_name,std::string body);
    void unsubscribe(std::string event_name);
    void presentationEnable(bool enable,
                            const std::string& screenName,
                            const std::string& screenId,
                            uint32_t appId,
                            uint64_t winId);
    void bwMapUpdate(std::string attr, int val);

    pj_status_t parsePresenterInfo(
                    IndicationInfo& info,
                    const pj_str_t* xml_st);
    pj_status_t startInboundContentChannel(pjsua_call_id call_id,bool start);
    void postSendDeviceGuid();
    void sendDeviceGuid();
    void postNotifyUnsupportedAudioDevice(void);
    void postTmmbrRecv(unsigned bitrate);
    pjmedia_dir startDirection(int media_idx);
    bool inactiveStream(int med_idx);
    void enableMeetingFeatures(const MeetingFeatureOptions& mfeatures);

    void SetFecTemporalByRTTLoss(uint32_t avgRTT, bool bRTTChange, float lossRate, bool bLossChange);
    MediaStreamsDirection getCurrentMediaDirection() const;
    bool isAudioDirectionChangeAllowed() const;

protected:
    void updatePeriodicStats();
    // Updates saved network info, possibly logging it if needed.
    void updateNetworkInfo();
    void parseConfigFile();
    void addSingleContentStream(bool start, const std::string& screenName, const std::string& screenId, pj_uint32_t appId);
    void resetSkipMedia(pjsua_call_id call_id);
    void stopPeriodicTimer(bool reschedule);
    pjmedia_transport *attachPcapReader(unsigned mediaIdx, pjmedia_transport *upstreamTransport);
    void applyRemoteNegoVideoBandwidth();
    bool isNegotiatedAstRtpHeaderExtension(pjsua_call_id call_id);
    void pcapCapture(bool enable);
    void yuvCapture(bool enable, int med_idx);
    void handleDebugOptions(DebugOptions &options);
    void setDebugOptions(const std::string& cap, int value);
#ifdef BJN_WEBRTC_VOE
    void sendAudioDspMetrics(const AudioDspMetrics& metrics);
    void sendStat(StatIdentifier id, float val);
    void sendAudioEcm(const pjmedia_event_ecm_notify_data &ecm);
    void getEcMode();
#endif

    talk_base::Thread *client_thread_;
    talk_base::Thread* browser_thread_;
    talk_base::MessageHandler *browser_msg_hndler_;
    pj_pool_t *pool;
    pjsua_acc_id    acc_id_;
    pjsua_call_id   call_id_;
    bool call_inprogress_;

    std::string uiInfo_;
    LocalMediaInfo selectedDeviceoptions_;
    bool call_success_;

    pjmedia_port* mem_recorder_port_;
    pjsua_conf_port_id slot;
    pjmedia_vid_dev_index cap_idx_;
    pjmedia_vid_dev_index rend_idx_;
    pjmedia_vid_dev_index rend_local_idx_;

    pjmedia_port *file_port;
    pjmedia_snd_port *snd_port;
    bool isSipInitialized;

    Config_Parser_Handler *cph_instance;

    BwMgr    bw_mgr_;
    void     *remote_window_[MAX_MED_IDX];
    void     *local_window_;

    bool faulty_microphone_;
    bool faulty_speaker_;
    unsigned int checkCount_;

    pjsua_vid_preview_param pvpParam;
    bool localViewFlag;
    pj_uint8_t capturerOn;
    std::string speakerGUID;

    bool     sendPresentationMode;
    bool     recvContentMode;
    bool     receivingPresentation;
    bool     rdcControlee;

    invite_queue invite_queue_;
    VideoCodecParams video_codec_params_;
    ChangeDevInfo dev_;
    std::vector<Device> audioCaptureDevices_;
    std::vector<Device> audioPlayoutDevices_;
    std::vector<Device> videoCaptureDevices_;
    bool quit_in_progress_;
    bool encryptionSupport;
    bool dual_stream_;
    bool aud_dev_change_inprogress_;
    bool no_vid_device_;
    bool prev_no_aud_device_;

    std::string config_path_;
    std::string useragent_;
    bool ready_reinvite_;
    bool mute_once_media_cb_;
    std::map<std::string,int> bwInfo;
    pj_mutex_t *bw_info_mutex_;
    pj_log_cb log_cb_;
    std::string cap_dev_id_;
    std::string play_dev_id_;
    pjmedia_vid_codec_factory *bjn_vid_codecs_fac_;
    make_call *make_call_param_;
    std::string mAudioDirection;
    std::string mVideoDirection;
    std::string mAppVideoDirection;
    std::string mContentDirection;
    Device mLastVideoCaptureDevice;
    pjsua_call_setting callSettings;
    std::string mCurrScreenName;
    std::string mCurrScreenId;
    pj_uint32_t mCurrScreenAppId;
    BJ::PCapWriter    *mPcapWriter;

    bool mReportDeviceIds;
    MeetingFeatures mMeetingFeatures;
    int mMinPort;
    int mPortRange;
    bool mNoPrflxRelayCand;
    RtxManager *rtx_mgr_;
    PJThreadDescRegistration mMainThreadDesc;

    FbrCpuMonitor     mFbrCpuMonitor;
    MovingAverage<float> mContentSendFPS;
    MovingAverage<unsigned> mContentSendBw;
    NetworkInfo       cached_network_info_;
    unsigned cpu_freq_load_timer_;
    unsigned fan_speed_load_timer_;
    pj_uint32_t       mLastMediaFormat;
    ChannelTransport mTransports[MAX_MED_IDX];

#ifdef ENABLE_RTP_LOGGER
    RtpLogger rtpLogger;
#endif
#ifdef ENABLE_GD
    // Graceful Degradation
    int mSystemMaxCpuThreshold;
    GracefulEventCollector  mGracefulEventCollector;
    SystemCpuMonitor  *mSystemCpuMonitor;
    AudioNetmon           mAudioNetmon;
#endif
    MeetingFeatureOptions mMeetingFeaturesOptions;
    bool                     mFecTemporalAvailable; //check if fec and temporal layer being enabled or disabled
    std::string mName;
    int mReIceCount;
    bool mNetworkDown;
    bool mReIcePending;
    int  mReIceInterval;
    pjmedia_aud_dev_index mSpkIdx;
    pjmedia_aud_dev_index mMicIdx;
    bool mAudioEngineEnabled;
  };
}
#endif // BJN_SIP_MANAGER_H_
