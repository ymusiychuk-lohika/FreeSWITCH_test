#include "skinnysipmanager.h"
#include "audiodevconfig.h"
#include <pjmedia-audiodev/audiodev_imp.h>
#include <pj/assert.h>
#include <pjmedia/rtp.h>
#include <pj/log.h>
#include <pj/rand.h>
#include <pj/os.h>
#include "pj_utils.h"
#include "voe_errors.h"
#include "voe_base.h"
#include "voe_codec.h"
#include "voe_volume_control.h"
#include "voe_dtmf.h"
#include "voe_rtp_rtcp.h"
#include "voe_audio_processing.h"
#include "voe_file.h"
#include "voe_hardware.h"
#include "voe_network.h"
#include "voe_neteq_stats.h"
#include "voe_video_sync.h"
#include "engine_configurations.h"
#include "talk/base/stream.h"
#include "bjnhelpers.h"
#include "config_handler.h"
#include "boost/assign.hpp"

#ifdef FB_X11
#include <arpa/inet.h>
#endif

#define THIS_FILE		"webrtc_voe_dev.c"
#define RTP_HEADER_SIZE 12
#define EXTRA_AUDIO_DEBUG_LOGS 0

using namespace webrtc;
const CodecInst g722 = {9, "G722", 16000, 320, 1, 64000};
const CodecInst opus = {120, "OPUS", 16000, 320, 1, 32000};

class MonitorStream : public OutStream {
public:
  virtual bool Write(const void *buf, int len) {
    //fwrite(buf, 1, len, file);
    return true;
  }
#if 0
  MonitorStream() {
      std::string filePath = BJN::getBjnLogfilePath() + "/" + "mic.pcm";
      file = fopen(filePath.c_str(), "wb");
  }
private:
  FILE *file;
#endif
};

enum device_mute_state {
    UNKNOWN,
    UNMUTED,
    MUTED,
};

std::string getMuteStateString(device_mute_state muteState)
{
    std::string muteStateString;
    switch(muteState)
    {
        case UNMUTED:
            muteStateString = "Unmuted";
            break;
        case MUTED:
            muteStateString = "Muted";
            break;
        case UNKNOWN:
        default:
            muteStateString = "Unknown";
            break;
    }
    return muteStateString;
}


/* webrtc_voe_audio device info */
struct webrtc_voe_audio_dev_info
{
    pjmedia_aud_dev_info	 info;
    unsigned			 dev_id;
};

class BJNTraceCallback: public TraceCallback
{
public:
    void Print(TraceLevel level, const char *traceString, int length)
    {
        mCapThread.Register("webrtc_trace");
        //TODO: Map levels correctly here.
        PJ_LOG(4, (THIS_FILE,"%.*s", length, traceString));
    }
private:
    PJDynamicThreadDescRegistration mCapThread;
};

class BJNObserver : public VoiceEngineObserver {
 public:

     virtual void CallbackOnError(int channel, int err_code);
private:
    PJDynamicThreadDescRegistration mCapThread;
};

void BJNObserver::CallbackOnError(const int channel, const int err_code) {
  mCapThread.Register("webrtc_observer");
  if (err_code == VE_RECEIVE_PACKET_TIMEOUT) {
    PJ_LOG(4, (THIS_FILE,"  RECEIVE PACKET TIMEOUT \n"));
  } else if (err_code == VE_PACKET_RECEIPT_RESTARTED) {
    PJ_LOG(4, (THIS_FILE,"  PACKET RECEIPT RESTARTED \n"));
  } else if (err_code == VE_RUNTIME_PLAY_ERROR) {
    PJ_LOG(4, (THIS_FILE,"  RUNTIME PLAY ERROR \n"));
  } else if (err_code == VE_RUNTIME_REC_ERROR) {
    PJ_LOG(4, (THIS_FILE,"  RUNTIME RECORD ERROR \n"));
  } else if (err_code == VE_REC_DEVICE_REMOVED) {
    PJ_LOG(4, (THIS_FILE,"  RECORD DEVICE REMOVED \n"));
  }
}

/* webrtc_voe_audio factory */
struct webrtc_voe_audio_factory
{
    pjmedia_aud_dev_factory	 base;
    pj_pool_t			*pool;
    pj_pool_factory		*pf;

    unsigned			 dev_count;
    struct webrtc_voe_audio_dev_info	*dev_info;

    VoiceEngine* m_voe;
    VoEBase* base1;
    VoECodec* codec;
    VoEVolumeControl* volume;
    VoERTP_RTCP* rtp_rtcp;
    VoEAudioProcessing* apm;
    VoENetwork* netw;
    VoEFile* file;
    VoEHardware* hardware;
    VoENetEqStats* neteqst;
    VoEVideoSync* vid_sync;
    BJNTraceCallback *trace_callback;
    BJNObserver *voe_observer;
    bjn_sky::skinnySipManager* sipManager;
    int frameCounter;
    const Config_Parser_Handler* cph_instance;
};

class BJNExternalTranport;
class BJNVoERxVadCallback;
class BJNVoEEsmStateCallback;
class BJNVoeEcStarvationCallback;
class BJNVoEUnsupportedDeviceCallback;
class BJNVoEAudioNotificationCallback;
class BJNVoEAudioStatsCallback;
class BJNVoEAudioMicAnimationCallback;

/* Sound stream. */
struct webrtc_voe_audio_stream
{
    pjmedia_aud_stream	 base;		    /**< Base stream	       */
    pjmedia_aud_param	 param;		    /**< Settings	       */
    pj_pool_t           *pool;              /**< Memory pool.          */
    void                *user_data;         /**< Application data.     */
    struct webrtc_voe_audio_factory *sf;
    BJNExternalTranport *bjnTransport;
    int chan;
    int num_pkts_since_last_log;
    pjsua_call_media *call_med;
    MonitorStream *file_monitor_;
    char cap_device_id[128];
    char play_device_id[128];
    BJNVoERxVadCallback *bjnVadObserver;
    BJNVoEEsmStateCallback *bjnEsmObserver;
    BJNVoeEcStarvationCallback *bjnEcObserver;
    BJNVoEUnsupportedDeviceCallback *bjnAudDeviceObserver;
    BJNVoEAudioNotificationCallback *bjnAudNotificationObserver;
    BJNVoEAudioStatsCallback   		*bjnAudioStatsObserver;
    BJNVoEAudioMicAnimationCallback *bjnAudioMicAnimationObserver;

    pj_time_val lastRXRtcp;
    unsigned rtcp_tx_cnt;
    unsigned rtcp_rx_cnt;
    bool     typing_detection_enabled;
    unsigned original_mic_level; //client's mic level at the start, to be recovered at the stop
    unsigned original_spk_level; //client's spk level at the start
    device_mute_state original_spk_mute_state; //client's spj mute status level at the start
    bool restore_spk_state;
    bool enable_custom_aec;
    bool enable_custom_aec_external;
};

/* Prototypes */
static pj_status_t webrtc_voe_factory_init(pjmedia_aud_dev_factory *f);
static pj_status_t webrtc_voe_factory_destroy(pjmedia_aud_dev_factory *f);
static pj_status_t webrtc_voe_factory_refresh(pjmedia_aud_dev_factory *f);
static unsigned    webrtc_voe_factory_get_dev_count(pjmedia_aud_dev_factory *f);
static pj_status_t webrtc_voe_factory_get_dev_info(pjmedia_aud_dev_factory *f,
					     unsigned index,
					     pjmedia_aud_dev_info *info);
static pj_status_t webrtc_voe_factory_default_param(pjmedia_aud_dev_factory *f,
					      unsigned index,
					      pjmedia_aud_param *param);
static pj_status_t webrtc_voe_factory_create_stream(pjmedia_aud_dev_factory *f,
					      const pjmedia_aud_param *param,
					      pjmedia_aud_rec_cb rec_cb,
					      pjmedia_aud_play_cb play_cb,
					      void *user_data,
					      pjmedia_aud_stream **p_aud_strm);

static pj_status_t webrtc_voe_stream_get_param(pjmedia_aud_stream *strm,
					 pjmedia_aud_param *param);
static pj_status_t webrtc_voe_stream_get_cap(pjmedia_aud_stream *strm,
				       pjmedia_aud_dev_cap cap,
				       void *value);
static pj_status_t webrtc_voe_stream_set_cap(pjmedia_aud_stream *strm,
				       pjmedia_aud_dev_cap cap,
				       const void *value);
static pj_status_t webrtc_voe_stream_start(pjmedia_aud_stream *strm);
static pj_status_t webrtc_voe_stream_stop(pjmedia_aud_stream *strm);
static pj_status_t webrtc_voe_stream_destroy(pjmedia_aud_stream *strm);
static void webrtc_voe_stream_rtp_cb(pjmedia_aud_stream *strm, void *usr_data,
                                     void*pkt, pj_ssize_t len);
static void webrtc_voe_stream_rtcp_cb(pjmedia_aud_stream *strm, void *usr_data,
                                      void*pkt, pj_ssize_t len);

/* Additional prototype for SKY-5005 */
static pj_status_t webrtc_voe_audio_reset_driver(webrtc_voe_audio_stream *strm, const std::string& cap, int value);

/* Operations */
static pjmedia_aud_dev_factory_op factory_op =
{
    &webrtc_voe_factory_init,
    &webrtc_voe_factory_destroy,
    &webrtc_voe_factory_get_dev_count,
    &webrtc_voe_factory_get_dev_info,
    &webrtc_voe_factory_default_param,
    &webrtc_voe_factory_create_stream,
    &webrtc_voe_factory_refresh
};

static pjmedia_aud_stream_op stream_op =
{
    &webrtc_voe_stream_get_param,
    &webrtc_voe_stream_get_cap,
    &webrtc_voe_stream_set_cap,
    &webrtc_voe_stream_start,
    &webrtc_voe_stream_stop,
    &webrtc_voe_stream_destroy,
    &webrtc_voe_stream_rtp_cb,
    &webrtc_voe_stream_rtcp_cb,
};

struct webrtc_audio_dsp_config {
    webrtc_audio_dsp_config():
          aec_enabled(true)
        , aec_tail_length_ms(50)
        , bjn_aec_tail_length_ms(200)
        , aec_mode(kEcConference)
        , aecm_mode(kAecmLoudSpeakerphone)
        , aecm_cngenabled(true)
        , ns_enabled(true)
        , ns_mode(kNsHighSuppression)
        , keynoise_enabled(true)
        , keynoise_mode(kKeynoiseHigh)
        , agc_enabled(true)
#if FB_MACOSX
        , agc_mode(kAgcAdaptiveDigital)
#else
        , agc_mode(kAgcAdaptiveAnalog)
#endif        
        , agc_const_gain_enabled(false)
        , agc_max_gain_db(18)
        , agc_digital_gain_db(0)
        , esm_enabled(true)
        , esm_mode(kEsMonitor)
        , esm_level(kEsSuppLow)
        , audio_delay_offset_ms(0)
        , delay_estimator_enabled(false)
        , cng_mode(kCngModeStatic)
        , mic_level(60)
        , aec_nlp_delay_filter(false)
        , aec_delay_mode(kEcReportedLatency)
        , aec_static_delay(60)
        , bjn_esm_mode(kEsMonitor)
        , bjn_esm_level(kEsSuppLow) {};
    ~webrtc_audio_dsp_config() {};

    bool                aec_enabled;
    int                 aec_tail_length_ms;
    int                 bjn_aec_tail_length_ms;
    webrtc::EcModes     aec_mode;
    webrtc::AecmModes   aecm_mode;
    bool                aecm_cngenabled;
    bool                ns_enabled;
    webrtc::NsModes     ns_mode;
    bool                keynoise_enabled;
    webrtc::KeynoiseSuppLevel    keynoise_mode;
    bool                agc_enabled;
    webrtc::AgcModes    agc_mode;
    bool                agc_const_gain_enabled;
    int                 agc_max_gain_db;
    int                 agc_digital_gain_db;
    bool                esm_enabled;
    webrtc::EsModes     esm_mode;
    webrtc::EsSuppression esm_level;
    int                 audio_delay_offset_ms;
    bool                delay_estimator_enabled;
    webrtc::ComfortNoiseMode cng_mode;
    int                 mic_level;
    bool                aec_nlp_delay_filter;
    webrtc::EcDelayModes aec_delay_mode;
    int16_t             aec_static_delay;
    webrtc::EsModes		bjn_esm_mode;
    webrtc::EsSuppression bjn_esm_level;
};

webrtc::EsModes MapEsmMode(int level) {
    switch(level) {
        case 0: return webrtc::kEsMonitor;
        case 1: return webrtc::kEsAutomatic;
        case 2: return webrtc::kEsEnabled;
        default: return webrtc::kEsMonitor;
    }
}

webrtc::EsSuppression MapEsmLevel(int level) {
    switch(level) {
        case 0: return webrtc::kEsSuppLow;
        case 1: return webrtc::kEsSuppModerate;
        case 2: return webrtc::kEsSuppHalfDuplex;
        default: return webrtc::kEsSuppModerate;
    }
}

std::string MapEsmModeToString(webrtc::EsModes mode) {
    switch(mode) {
        case webrtc::kEsMonitor: return "Monitor";
        case webrtc::kEsAutomatic: return "Automatic";
        case webrtc::kEsEnabled: return "Enabled";
        default: return "Undefined";
    }
}

std::string MapEsmLevelToString(webrtc::EsSuppression level) {
    switch(level) {
        case webrtc::kEsSuppLow: return "Low";
        case webrtc::kEsSuppModerate: return "Moderate";
        case webrtc::kEsSuppHalfDuplex: return "HalfDuplex";
        default: return "Undefined";
    }
}

//FBR-513 report ec mode to Indigo
void PushEcMode(struct webrtc_voe_audio_stream *strm,webrtc::EcModes &ec)
{
    EcModes ecm;
    bool enableFlg;
    if(strm->sf->apm->GetEcStatus(enableFlg,ecm)==0)
    {
        if((ecm != ec) && (enableFlg=false))
        {
            PJ_LOG(4, (THIS_FILE, "EC in conflict mode: config %d but actual %d",ec, ecm));
            ecm = kMaxEcType;
        }
    }
    else
        ecm = kMaxEcType;

    pjmedia_event aud_event;
    aud_event.type = PJMEDIA_EVENT_AUDIO;
    aud_event.aud_subtype = PJMEDIA_AUD_EVENT_ECM_NOTIFY;
    aud_event.data.ecm_notify.ec_mode = ecm;
    pjmedia_event_publish(NULL,strm, &aud_event, (pjmedia_event_publish_flag) 0);
}

webrtc::ComfortNoiseMode MapCngMode(int level) {
    switch(level) {
        case 1: return webrtc::kCngModeStatic;
        case 2: return webrtc::kCngModeAdaptive;
        default: return webrtc::kCngModeOff;
    }
}

class BJNVoERxVadCallback: public VoERxVadCallback
{
public:
    BJNVoERxVadCallback(webrtc_voe_audio_stream *strm)
        : _stream(strm){}

    void OnRxVad(int channel, int vadDecision) {
        mVadThread.Register("webrtc_vad");
        if (bjn_sky::getAudioFeatures()->mEnablePostAudioCallbacksMessages){
            pjmedia_event_vad_notify_data vad_notify;
            vad_notify.voice_detected = vadDecision;
            _stream->sf->sipManager->postToClient(bjn_sky::MSG_AUD_EVENT_VAD_NOTIFY, vad_notify);
        }
        else{
            pjmedia_event aud_event;
            aud_event.type = PJMEDIA_EVENT_AUDIO;
            aud_event.aud_subtype = PJMEDIA_AUD_EVENT_VAD_NOTIFY;
            aud_event.data.vad_notify.voice_detected = vadDecision;
            pjmedia_event_publish(NULL, _stream, &aud_event, (pjmedia_event_publish_flag)0);
        }
    }

    virtual ~BJNVoERxVadCallback() {
    }
protected:
    PJDynamicThreadDescRegistration mVadThread;
    webrtc_voe_audio_stream *_stream;
};

class BJNVoEEsmStateCallback: public VoEEsmStateCallback
{
public:
    BJNVoEEsmStateCallback(webrtc_voe_audio_stream *strm)
        : _stream(strm){}

    void OnToggleEsmState(bool esmEnabled, EsmMetrics esmMetrics ) {
        mEsmThread.Register("webrtc_esm");

        if (bjn_sky::getAudioFeatures()->mEnablePostAudioCallbacksMessages){
            pjmedia_event_esm_notify_data esm_notify;
            esm_notify.esm_state = esmEnabled;
            esm_notify.esm_xcorr_metric = esmMetrics.mXCorrMetric;
            esm_notify.esm_echo_present_metric = esmMetrics.mEchoPresentMetric;
            esm_notify.esm_echo_removed_metric = esmMetrics.mEchoRemovedMetric;
            esm_notify.esm_echo_detected_metric = esmMetrics.mEchoDetectedMetric;
            _stream->sf->sipManager->postToClient(bjn_sky::MSG_SEND_ESM_STATUS,esm_notify);
        }
        else{
            pjmedia_event aud_event;
            aud_event.type = PJMEDIA_EVENT_AUDIO;
            aud_event.aud_subtype = PJMEDIA_AUD_EVENT_ESM_NOTIFY;
            aud_event.data.esm_notify.esm_state = esmEnabled;
            aud_event.data.esm_notify.esm_xcorr_metric = esmMetrics.mXCorrMetric;
            aud_event.data.esm_notify.esm_echo_present_metric = esmMetrics.mEchoPresentMetric;
            aud_event.data.esm_notify.esm_echo_removed_metric = esmMetrics.mEchoRemovedMetric;
            aud_event.data.esm_notify.esm_echo_detected_metric = esmMetrics.mEchoDetectedMetric;
            pjmedia_event_publish(NULL, _stream, &aud_event, (pjmedia_event_publish_flag)0);
        }

    }

    virtual ~BJNVoEEsmStateCallback() {
    }
protected:
    webrtc_voe_audio_stream *_stream;
    PJDynamicThreadDescRegistration mEsmThread;
};

class BJNVoeEcStarvationCallback: public VoeEcStarvationCallback
{
public:
    BJNVoeEcStarvationCallback(webrtc_voe_audio_stream *strm)
        : _stream(strm){}

    void OnBufferStarvation(int bufferedDataMS, int delayMS) {
        mEcThread.Register("webrtc_ec");
        if (bjn_sky::getAudioFeatures()->mEnablePostAudioCallbacksMessages){
            pjmedia_event_ecbs_data ecbs_notify;
            ecbs_notify.buffered_data = bufferedDataMS;
            ecbs_notify.delay_ms = delayMS;
            _stream->sf->sipManager->postToClient(bjn_sky::MSG_SEND_ECBS_STATUS, ecbs_notify);
        }
        else{
            pjmedia_event aud_event;
            aud_event.type = PJMEDIA_EVENT_AUDIO;
            aud_event.aud_subtype = PJMEDIA_AUD_EVENT_ECBS_NOTIFY;
            aud_event.data.ecbs_notify.buffered_data = bufferedDataMS;
            aud_event.data.ecbs_notify.delay_ms = delayMS;
            pjmedia_event_publish(NULL, _stream, &aud_event, (pjmedia_event_publish_flag)0);
        }
    }

    virtual ~BJNVoeEcStarvationCallback() {
    }
protected:
    webrtc_voe_audio_stream *_stream;
    PJDynamicThreadDescRegistration mEcThread;
};

class BJNVoEUnsupportedDeviceCallback : public VoEUnsupportedDeviceCallback {
public:
    BJNVoEUnsupportedDeviceCallback(webrtc_voe_audio_stream *strm)
        : _stream(strm){}

    void OnEsmEnabled(bool enabled) {
        mAudWarnThread.Register("webrtc_aud_warn");
        pjmedia_event aud_event;
        aud_event.type = PJMEDIA_EVENT_AUDIO;
        aud_event.aud_subtype = PJMEDIA_AUD_EVENT_UNSUPPORTED_DEVICE;
        pjmedia_event_publish(NULL, _stream, &aud_event, (pjmedia_event_publish_flag) 0);
    }

    virtual ~BJNVoEUnsupportedDeviceCallback() { }
protected:
    webrtc_voe_audio_stream *_stream;
    PJDynamicThreadDescRegistration mAudWarnThread;
};

class BJNVoEAudioNotificationCallback : public VoEAudioNotificationCallback {
public:
    BJNVoEAudioNotificationCallback(webrtc_voe_audio_stream *strm)
        : _stream(strm) {}

    void OnAudioNotification(GracefulEventNotificationList& notificationList) {
        mAudWarnThread.Register("graceful_audio_notification");

        if (bjn_sky::getAudioFeatures()->mEnablePostAudioCallbacksMessages){
            _stream->sf->sipManager->postToClient
                (bjn_sky::MSG_GRACEFUL_AUD_EVENT_NOTIFICATION, notificationList);
        }
        else{
            pjmedia_event aud_event;
            aud_event.type = PJMEDIA_EVENT_AUDIO;
            aud_event.aud_subtype = PJMEDIA_GRACEFUL_AUD_EVENT_NOTIFICATION;
            aud_event.data.aud_notify.notification_list = static_cast<void*>(&notificationList);
            pjmedia_event_publish(NULL, _stream, &aud_event, (pjmedia_event_publish_flag)0);
        }
        if(bjn_sky::getAudioFeatures()->mAudioGlitchAutoReset) {
            std::string glitchStr = AUDIO_DRIVER_GLITCH_STR;
            bjn_sky::AudioConfigOptions options;
            GracefulEventNotificationList::iterator it =
                std::find(notificationList.begin(), notificationList.end(), glitchStr);
            if (it != notificationList.end()) {
                options["reset_driver"] = 1;
                if (bjn_sky::getAudioFeatures()->mEnablePostAudioCallbacksMessages){
                    _stream->sf->sipManager->postSetAudioConfigOptions(options);
                }
                else{
                    pjmedia_event aud_event;
                    aud_event.type = PJMEDIA_EVENT_AUDIO;
                    aud_event.aud_subtype = PJMEDIA_AUD_EVENT_CONFIG_CHANGE;
                    aud_event.data.ptr = &options;
                    pjmedia_event_publish(NULL,
                        _stream,
                        &aud_event,
                        (pjmedia_event_publish_flag)0);
                }
            }
        }
    }

    virtual ~BJNVoEAudioNotificationCallback() { }

protected:
    webrtc_voe_audio_stream *_stream;
    PJDynamicThreadDescRegistration mAudWarnThread;
};

class BJNVoEAudioStatsCallback: public VoEAudioStatsCallback
{
public:
    BJNVoEAudioStatsCallback(webrtc_voe_audio_stream *strm)
        : _stream(strm){}

    void ReportAudioDspStats(AudioDspMetrics& metrics) {
        mAudioStatsThread.Register("webrtc_audio_stats");

        metrics.NormalizeMetrics();

//In case of Windows Speaker and Mic State will filled in skinnysipmanager
//Just to make it explicit that stats will be fetched from webrtc in case
// of mac and linux not any other platform, putting under linux and mac macros
#if defined(OSX) || defined(LINUX)
        // Capture Speaker and Mic State
        unsigned int level = 0;
        bool mute = false;
        _stream->sf->volume->GetSpeakerVolume(level);
        metrics.mSpkrVolume = static_cast<float>(level);
        _stream->sf->volume->GetSystemOutputMute(mute);
        metrics.mSpkrMuteState = static_cast<float>(mute);
        _stream->sf->volume->GetMicVolume(level);
        metrics.mMicVolume = static_cast<float>(level);
        _stream->sf->volume->GetSystemInputMute(mute);
        metrics.mMicMuteState = static_cast<float>(mute);
#endif

        // Capture Jitter Buffer Stats
        _stream->sf->neteqst->GetNetworkStatistics(_stream->chan, metrics.mNetworkStats);

        // Publish Stats to Sip Manager
        if (bjn_sky::getAudioFeatures()->mEnablePostAudioCallbacksMessages){
            _stream->sf->sipManager->postToClient(bjn_sky::MSG_SEND_AUDIO_DSP_STATS, metrics);
        }
        else{
            pjmedia_event aud_event;
            aud_event.type = PJMEDIA_EVENT_AUDIO;
            aud_event.aud_subtype = PJMEDIA_AUD_EVENT_DSP_STATS;
            aud_event.data.ptr = static_cast<void*>(&metrics);
            pjmedia_event_publish(NULL, _stream, &aud_event, (pjmedia_event_publish_flag) 0);
        }

    };

    virtual ~BJNVoEAudioStatsCallback() { }
protected:
    webrtc_voe_audio_stream *_stream;
    PJDynamicThreadDescRegistration mAudioStatsThread;
};

class BJNVoEAudioMicAnimationCallback: public VoEAudioMicAnimationCallback
{
public:
    BJNVoEAudioMicAnimationCallback(webrtc_voe_audio_stream *strm)
        : _stream(strm){}

    void ReportMicAnimationState(int state) {
        mAudioMicAnimationThread.Register("webrtc_mic_animation");

        // Publish Stats to Sip Manager
        if (bjn_sky::getAudioFeatures()->mEnablePostAudioCallbacksMessages){
            _stream->sf->sipManager->postToClient
                (bjn_sky::MSG_AUD_EVENT_MIC_ANIMATION,state);
        }
        else{
            pjmedia_event aud_event;
            aud_event.type = PJMEDIA_EVENT_AUDIO;
            aud_event.aud_subtype = PJMEDIA_AUD_EVENT_MIC_ANIMATION;
            aud_event.data.aud_mic_animation.mic_state = state;
            pjmedia_event_publish(NULL, _stream, &aud_event, (pjmedia_event_publish_flag)0);
        }
    };

    virtual ~BJNVoEAudioMicAnimationCallback() { }

protected:
    webrtc_voe_audio_stream *_stream;
    PJDynamicThreadDescRegistration mAudioMicAnimationThread;
};

class BJNExternalTranport : public Transport
{
public:
    BJNExternalTranport(webrtc_voe_audio_stream *strm)
        : _stream(strm) {
        }

    int SendPacket(int channel,const void *data,int len);
    int SendRTCPPacket(int channel, const void *data, int len);
private:
    webrtc_voe_audio_stream *_stream;
    PJDynamicThreadDescRegistration mRtpThread;
    PJDynamicThreadDescRegistration mRtcpThread;
};

static inline int get_rtp_seq_num(const void *data)
{
    const pjmedia_rtp_hdr *hdr = (pjmedia_rtp_hdr *) data;
    return ntohs(hdr->seq);
}

int BJNExternalTranport::SendPacket(int channel,const void *data,int len)
{
    pj_status_t status;
    mRtpThread.Register("webrtc_rtp");
    if(_stream->call_med && _stream->call_med->tp && _stream->call_med->ice_complete) {
        status = pjmedia_transport_send_rtp(_stream->call_med->tp, data, len);
        if (status != PJ_SUCCESS) {
            PJ_PERROR(4,(THIS_FILE, status, "Error sending RTP"));
        }
    }
    // Uncomment following code to test loopback audio using webrtc stack
    //_stream->sf->netw->ReceivedRTPPacket(channel, data, len);
    return len;
}

int BJNExternalTranport::SendRTCPPacket(int channel, const void *data, int len)
{
    pj_status_t status;
    mRtcpThread.Register("webrtc_rtcp");
    if(_stream->call_med && _stream->call_med->ice_complete) {
        status = pjmedia_transport_send_rtcp(_stream->call_med->tp, data, len);
        if (status != PJ_SUCCESS) {
            PJ_PERROR(4,(THIS_FILE, status, "Error sending RTCP"));
        }
    }
    // Uncomment following code to test loopback audio using webrtc stack
    //_stream->sf->netw->ReceivedRTCPPacket(channel, data, len);
    _stream->rtcp_tx_cnt++;
    return len;
}



/****************************************************************************
 * Factory operations
 */
/*
 * Init webrtc_voe_audio audio driver.
 */
pjmedia_aud_dev_factory* pjmedia_webrtc_voe_audio_factory(pj_pool_factory *pf, bjn_sky::skinnySipManager* sipManager)
{
    struct webrtc_voe_audio_factory *f;
    pj_pool_t *pool;

    pool = pj_pool_create(pf, "webrtc_voe audio", 1000, 1000, NULL);
    f = PJ_POOL_ZALLOC_T(pool, struct webrtc_voe_audio_factory);
    f->pf = pf;
    f->pool = pool;
    f->base.op = &factory_op;
    f->sipManager = sipManager;

    return &f->base;
}


/* API: init factory */
static pj_status_t webrtc_voe_factory_init(pjmedia_aud_dev_factory *f)
{
    struct webrtc_voe_audio_factory *nf = (struct webrtc_voe_audio_factory*)f;

    nf->m_voe = VoiceEngine::Create();
    nf->base1 = VoEBase::GetInterface(nf->m_voe);
    nf->codec = VoECodec::GetInterface(nf->m_voe);
    nf->apm = VoEAudioProcessing::GetInterface(nf->m_voe);
    nf->volume = VoEVolumeControl::GetInterface(nf->m_voe);
    nf->rtp_rtcp = VoERTP_RTCP::GetInterface(nf->m_voe);
    nf->netw = VoENetwork::GetInterface(nf->m_voe);
    nf->file = VoEFile::GetInterface(nf->m_voe);
    nf->hardware = VoEHardware::GetInterface(nf->m_voe);
    nf->neteqst = VoENetEqStats::GetInterface(nf->m_voe);
    nf->vid_sync = VoEVideoSync::GetInterface(nf->m_voe);

    //VoiceEngine::SetTraceFilter(kTraceAll);
    VoiceEngine::SetTraceFilter(kTraceWarning | kTraceError | kTraceCritical);

    nf->trace_callback = new BJNTraceCallback();
    VoiceEngine::SetTraceCallback(nf->trace_callback);

#if FB_WIN
    // Audio Device Layer needs to be set here prior to initializing voice engine
    if(bjn_sky::getAudioFeatures()->mCoreAudioWin && !bjn_sky::isXP()) {
        nf->hardware->SetAudioDeviceLayer(webrtc::kAudioWindowsCore);
    } else {
        nf->hardware->SetAudioDeviceLayer(webrtc::kAudioWindowsWave);
    }
#endif

    nf->base1->Init();
    nf->voe_observer = new BJNObserver();
    nf->base1->RegisterVoiceEngineObserver(*nf->voe_observer);
    nf->cph_instance = NULL;

    const std::string audio_layer_strings[5] = {"default", "wave", "core", "alsa", "pulse"};
    webrtc::AudioLayers audio_layer;
    nf->hardware->GetAudioDeviceLayer(audio_layer);

    PJ_LOG(4, (THIS_FILE, "webrtc_voe audio initialized with %s audio layer",
        audio_layer_strings[audio_layer].c_str()));
    return webrtc_voe_factory_refresh(f);
}

/* API: destroy factory */
static pj_status_t webrtc_voe_factory_destroy(pjmedia_aud_dev_factory *f)
{
    struct webrtc_voe_audio_factory *nf = (struct webrtc_voe_audio_factory*)f;

    nf->base1->DeRegisterVoiceEngineObserver();
    nf->base1->Terminate();
    if (nf->base1) nf->base1->Release();
    if (nf->codec) nf->codec->Release();
    if (nf->volume) nf->volume->Release();
    if (nf->rtp_rtcp) nf->rtp_rtcp->Release();
    if (nf->apm) nf->apm->Release();
    if (nf->netw) nf->netw->Release();
    if (nf->file) nf->file->Release();
    if (nf->hardware) nf->hardware->Release();
    if (nf->neteqst) nf->neteqst->Release();
    if (nf->vid_sync) nf->vid_sync->Release();

    VoiceEngine::Delete(nf->m_voe);
    pj_pool_t *pool = nf->pool;
    nf->pool = NULL;
    pj_pool_release(pool);

    return PJ_SUCCESS;
}

/* API: refresh the list of devices */
static pj_status_t webrtc_voe_factory_refresh(pjmedia_aud_dev_factory *f)
{
    struct webrtc_voe_audio_factory *nf = (struct webrtc_voe_audio_factory*)f;
    struct webrtc_voe_audio_dev_info *ndi;

    /* Initialize input and output devices here */
    int rec_devices = 0;
    int play_devices = 0;
    unsigned i = 0;
    nf->hardware->GetNumOfRecordingDevices(rec_devices);
    nf->hardware->GetNumOfPlayoutDevices(play_devices);
    nf->dev_count = play_devices + rec_devices;

    PJ_LOG(4, (THIS_FILE, "Found %d audio devices", nf->dev_count));
                    
    nf->dev_info = (struct webrtc_voe_audio_dev_info*)
 		   pj_pool_calloc(nf->pool, nf->dev_count,
 				  sizeof(struct webrtc_voe_audio_dev_info));

    char dn[128] = { 0 };
    char guid[128] = { 0 };

    for(i = 0; i < (unsigned) rec_devices; i++) {
        ndi = &nf->dev_info[i];
        pj_bzero(ndi, sizeof(*ndi));
        nf->hardware->GetRecordingDeviceName(i, dn, guid);
        strcpy(ndi->info.name, dn);
#ifdef FB_WIN
        strcpy(ndi->info.driver, guid);
#else
        strcpy(ndi->info.driver, dn);
#endif
        ndi->info.input_count = 2;
        ndi->info.output_count = 0;
        ndi->info.default_samples_per_sec = 16000;
        /* Set the device capabilities here */
        ndi->info.caps = PJMEDIA_AUD_DEV_CAP_EC;
        PJ_LOG(4, (THIS_FILE, " dev_id %d: %s (in=%d, out=%d)",
	    i,
	    nf->dev_info[i].info.name,
	    nf->dev_info[i].info.input_count,
	    nf->dev_info[i].info.output_count));
    }

    for(; i < nf->dev_count; i++) {
        ndi = &nf->dev_info[i];
        pj_bzero(ndi, sizeof(*ndi));
        nf->hardware->GetPlayoutDeviceName((i - rec_devices), dn, guid);
        strcpy(ndi->info.name, dn);
#ifdef FB_WIN
        strcpy(ndi->info.driver, guid);
#else
        strcpy(ndi->info.driver, dn);
#endif
        ndi->info.input_count = 0;
        ndi->info.output_count = 2;
        ndi->info.default_samples_per_sec = 16000;
        /* Set the device capabilities here */
        ndi->info.caps = PJMEDIA_AUD_DEV_CAP_EC;
        PJ_LOG(4, (THIS_FILE, " dev_id %d: %s (in=%d, out=%d)", 
	    i,
	    nf->dev_info[i].info.name,
	    nf->dev_info[i].info.input_count,
	    nf->dev_info[i].info.output_count));
    }
    PJ_LOG(4, (THIS_FILE, "webrtc_voe audio refreshed"));
    return PJ_SUCCESS;
}

/* API: get number of devices */
static unsigned webrtc_voe_factory_get_dev_count(pjmedia_aud_dev_factory *f)
{
    struct webrtc_voe_audio_factory *nf = (struct webrtc_voe_audio_factory*)f;
    return nf->dev_count;
}

/* API: get device info */
static pj_status_t webrtc_voe_factory_get_dev_info(pjmedia_aud_dev_factory *f,
					     unsigned index,
					     pjmedia_aud_dev_info *info)
{
    struct webrtc_voe_audio_factory *nf = (struct webrtc_voe_audio_factory*)f;

    PJ_ASSERT_RETURN(index < nf->dev_count, PJMEDIA_EAUD_INVDEV);

    pj_memcpy(info, &nf->dev_info[index].info, sizeof(*info));

    return PJ_SUCCESS;
}

/* API: create default device parameter */
static pj_status_t webrtc_voe_factory_default_param(pjmedia_aud_dev_factory *f,
					      unsigned index,
					      pjmedia_aud_param *param)
{
    struct webrtc_voe_audio_factory *nf = (struct webrtc_voe_audio_factory*)f;
    struct webrtc_voe_audio_dev_info *di = &nf->dev_info[index];

    PJ_ASSERT_RETURN(index < nf->dev_count, PJMEDIA_EAUD_INVDEV);

    pj_bzero(param, sizeof(*param));
    if (di->info.input_count && di->info.output_count) {
	param->dir = PJMEDIA_DIR_CAPTURE_PLAYBACK;
	param->rec_id = index;
	param->play_id = index;
    } else if (di->info.input_count) {
	param->dir = PJMEDIA_DIR_CAPTURE;
	param->rec_id = index;
	param->play_id = PJMEDIA_AUD_INVALID_DEV;
    } else if (di->info.output_count) {
	param->dir = PJMEDIA_DIR_PLAYBACK;
	param->play_id = index;
	param->rec_id = PJMEDIA_AUD_INVALID_DEV;
    } else {
	return PJMEDIA_EAUD_INVDEV;
    }

    /* Set the mandatory settings here */
    param->clock_rate = di->info.default_samples_per_sec;
    param->channel_count = 1;
    param->samples_per_frame = di->info.default_samples_per_sec * 20 / 1000;
    param->bits_per_sample = 16;

    /* Set the device capabilities here */
    param->flags = 0;
    param->rec_id_internal = param->play_id_internal = PJ_TRUE;
    return PJ_SUCCESS;
}

/* API: create stream */
static pj_status_t webrtc_voe_factory_create_stream(pjmedia_aud_dev_factory *f,
					      const pjmedia_aud_param *param,
					      pjmedia_aud_rec_cb rec_cb,
					      pjmedia_aud_play_cb play_cb,
					      void *user_data,
					      pjmedia_aud_stream **p_aud_strm)
{
    struct webrtc_voe_audio_factory *nf = (struct webrtc_voe_audio_factory*)f;
    pj_pool_t *pool;
    struct webrtc_voe_audio_stream *strm;
    PJ_LOG(4, (THIS_FILE, "In function %s", __FUNCTION__));

    /* Create and Initialize stream descriptor */
    pool = pj_pool_create(nf->pf, "webrtc_voe_audio-dev", 1000, 1000, NULL);
    PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

    strm = PJ_POOL_ZALLOC_T(pool, struct webrtc_voe_audio_stream);
    pj_memcpy(&strm->param, param, sizeof(*param));
    strm->pool = pool;
    strm->user_data = user_data;
    strm->sf = nf;

    strm->chan = strm->sf->base1->CreateChannel();
    if (strm->chan < 0) {
        PJ_LOG(4, (THIS_FILE, "Error creating channel code = %i\n", strm->sf->base1->LastError()));
        return PJ_ENOMEM;
    }

    strm->bjnTransport = new BJNExternalTranport(strm);
    strm->sf->netw->RegisterExternalTransport(strm->chan, *strm->bjnTransport);
    /* Done */
    strm->base.op = &stream_op;
    *p_aud_strm = &strm->base;
    
    if(!nf->cph_instance)
    {
        // We assume cph_instance is created per sip manager and is valid for throught life of sip manager.
        // So only set pointer if not already set. Since we cache this pointer in global pjsua var, if 2
        // 2 calls are running in parallel then there are chances that second time we get pointer from
        // second call
        nf->cph_instance = (const Config_Parser_Handler*)param->config_parser;
    }
    strm->typing_detection_enabled = bjn_sky::getAudioFeatures()->mTypingDetection;
    strm->enable_custom_aec = bjn_sky::getAudioFeatures()->mEnableCustomAec;
    strm->enable_custom_aec_external = bjn_sky::getAudioFeatures()->mEnableCustomAecExternal;

    PJ_LOG(4, (THIS_FILE, "custom_aec = %s external_aec = %s",
                strm->enable_custom_aec ? "true":"false",
                strm->enable_custom_aec_external ? "true":"false"));

    strm->bjnAudNotificationObserver = NULL;
    strm->bjnAudioStatsObserver = NULL;
    strm->bjnAudioMicAnimationObserver = NULL;

    strm->original_mic_level = -1; //uninitialized state

    return PJ_SUCCESS;
}

/* API: Get stream info. */
static pj_status_t webrtc_voe_stream_get_param(pjmedia_aud_stream *s,
					 pjmedia_aud_param *pi)
{
    struct webrtc_voe_audio_stream *strm = (struct webrtc_voe_audio_stream*)s;

    PJ_ASSERT_RETURN(strm && pi, PJ_EINVAL);

    pj_memcpy(pi, &strm->param, sizeof(*pi));

    if (webrtc_voe_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING,
			    &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING;
    }

    if (webrtc_voe_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING,
			    &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING;
    }

    if (webrtc_voe_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_OUTPUT_MUTE_SETTING,
			    &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_OUTPUT_MUTE_SETTING;
    }

    if (webrtc_voe_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_MICROPHONE_MUTE,
			    &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_MICROPHONE_MUTE;
    }
    if (webrtc_voe_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER,
			    &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER;
    }
    if (webrtc_voe_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_OUTPUT_SIGNAL_METER,
			    &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_OUTPUT_SIGNAL_METER;
    }
    if (webrtc_voe_stream_get_cap(s, PJMEDIA_AUD_DEV_CAP_ECM,
			    &pi->output_vol) == PJ_SUCCESS)
    {
        pi->flags |= PJMEDIA_AUD_DEV_CAP_ECM;
    }
    return PJ_SUCCESS;
}

static bool webrtc_voe_get_stats(webrtc_voe_audio_stream *strm, pjsua_stream_stat **stat) 
{
    CallStatistics cs;
    unsigned int ssrc;
    CodecInst codec;
    unsigned short sequenceNumber;
    unsigned int timestamp;

    // Data we obtain locally.
    memset(&cs, 0, sizeof(cs));
    if (strm->sf->rtp_rtcp->GetRTCPStatistics(strm->chan, cs) == -1 ||
        strm->sf->rtp_rtcp->GetLocalSSRC(strm->chan, ssrc) == -1) {
        return false;
    }

    if (bjn_sky::getAudioFeatures()->mCacheRtpSeqTs ||
        bjn_sky::getAudioFeatures()->mEnableDisableAudioEngine) {
        if (strm->sf->rtp_rtcp->GetLocalSequenceNumber(strm->chan, sequenceNumber) == -1 ||
            strm->sf->rtp_rtcp->GetLocalTimestamp(strm->chan, timestamp) == -1) {
            return false;
        }
        stat[0]->rtcp.rtp_tx_last_seq = sequenceNumber;
        stat[0]->rtcp.rtp_tx_last_ts = timestamp;
    }
    stat[0]->rtcp.rtt.mean = (cs.rttMs > 0) ? cs.rttMs : -1;

    stat[0]->rtcp.tx.bytes = cs.bytesSent;
    stat[0]->rtcp.tx.pkt = cs.packetsSent;
    stat[0]->rtcp.tx.update = strm->lastRXRtcp;
    stat[0]->rtcp.tx.update_cnt = strm->rtcp_rx_cnt;
    stat[0]->rtcp.tx.ssrc = ssrc;
    std::vector<ReportBlock> receive_blocks;
    if (strm->sf->rtp_rtcp->GetRemoteRTCPReportBlocks(strm->chan, &receive_blocks) != -1 &&
        strm->sf->codec->GetRecCodec(strm->chan, codec) != -1) {
        std::vector<ReportBlock>::iterator iter;
        for (iter = receive_blocks.begin(); iter != receive_blocks.end(); ++iter) {
            // Lookup report for send ssrc only.
            if (iter->source_SSRC == ssrc) {
                stat[0]->rtcp.tx.loss_period.last = iter->fraction_lost;
                stat[0]->rtcp.tx.loss = iter->cumulative_num_packets_lost;
                // Convert samples to milliseconds.
                if (codec.plfreq / 1000 > 0) {
                    stat[0]->rtcp.tx.jitter.last = iter->interarrival_jitter / (codec.plfreq / 1000);
                }
                break;
            }
        }
    }

    stat[0]->rtcp.rx.bytes = cs.bytesReceived;
    stat[0]->rtcp.rx.pkt = cs.packetsReceived;
    stat[0]->rtcp.rx.loss_period.last = cs.fractionLost;
    stat[0]->rtcp.rx.loss = cs.cumulativeLost;
    stat[0]->rtcp.rx.update_cnt = strm->rtcp_tx_cnt;
    strm->sf->rtp_rtcp->GetRemoteSSRC(strm->chan, ssrc);
    stat[0]->rtcp.rx.ssrc = ssrc;
    // Convert samples to milliseconds.
    if (codec.plfreq / 1000 > 0) {
        stat[0]->rtcp.rx.jitter.last = cs.jitterSamples / (codec.plfreq / 1000);
    }

    pjmedia_event aud_event;
    aud_event.type = PJMEDIA_EVENT_AUDIO;
    aud_event.aud_subtype = PJMEDIA_AUD_EVENT_NETWORK_STATS;
    memcpy(&aud_event.data.aud_netstats, &stat[0]->rtcp, sizeof(pjmedia_rtcp_stat));
    pjmedia_event_publish(NULL, strm, &aud_event, (pjmedia_event_publish_flag) 0);

    return true;
}

bool webrtc_voe_start_local_monitor(webrtc_voe_audio_stream *strm, bool enable)
{
    if (enable && !strm->file_monitor_) {
        strm->file_monitor_ = new MonitorStream;
        if (strm->sf->file->StartRecordingMicrophone(strm->file_monitor_) == -1) {
            strm->sf->file->StopRecordingMicrophone();
            delete strm->file_monitor_;
            strm->file_monitor_ = NULL;
            return false;
        }
    } else if(!enable && strm->file_monitor_) {
        strm->sf->file->StopRecordingMicrophone();
        delete strm->file_monitor_;
        strm->file_monitor_ = NULL;
    }
    return true;
}

/* API: get capability */
static pj_status_t webrtc_voe_stream_get_cap(pjmedia_aud_stream *s,
                                             pjmedia_aud_dev_cap cap,
                                             void *pval)
{
    pj_status_t status = PJ_EUNKNOWN;
    struct webrtc_voe_audio_stream *strm = (struct webrtc_voe_audio_stream*)s;
    PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

    if (cap == PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING)
    {
        // retrieve output device's volume here
        if(strm->sf->volume->GetSpeakerVolume(*(unsigned*)pval) == 0)
            status = PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING)
    {
        // retrieve input device's volume here
        if(strm->sf->volume->GetMicVolume(*(unsigned*)pval) == 0)
            status = PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_CAP_OUTPUT_MUTE_SETTING)
    {
        // retrieve output device's mute status
        if(strm->sf->volume->GetSystemOutputMute(*(bool*)pval) == 0)
            status = PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_CAP_MICROPHONE_MUTE)
    {
        // retrieve output device's mute status
        if(strm->sf->volume->GetSystemInputMute(*(bool*)pval) == 0)
            status = PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_CAP_EC)
    {
        EcModes aecMode;
        if(strm->sf->apm->GetEcStatus(*(bool *)pval, aecMode) == 0)
            status = PJ_SUCCESS;
    }else if(cap == PJMEDIA_AUD_DEV_CAP_ECM)
    {
        bool aecEnable;
        if(strm->sf->apm->GetEcStatus(aecEnable, *(EcModes *)pval) == 0)
        {
            //AG: do we need to check if enable true or not?
            status = PJ_SUCCESS;
        }
    }else if(cap == PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER)
    {
        if(strm->sf->volume->GetSpeechInputLevel(*(unsigned*)pval) == 0)
            status = PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_CAP_OUTPUT_SIGNAL_METER)
    {
        if(strm->sf->volume->GetSpeechOutputLevel(strm->chan, *(unsigned*)pval) == 0)
            status = PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_CAP_STATS)
    {
        if(webrtc_voe_get_stats(strm, (pjsua_stream_stat **) &pval) == true)
            status = PJ_SUCCESS;
    } else
    {
        status = PJMEDIA_EAUD_INVCAP;
    }
    return status;
}

static pj_status_t webrtc_voe_audio_debug(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: enableAudioDebug(%d)", value));
    if(value > 0) 
    {
        strm->sf->apm->StartDebugRecording(BJN::getBjnLogfilePath(true).c_str()); 
#if EXTRA_AUDIO_DEBUG_LOGS
        bool enabledFlag;
        webrtc::EcModes aecMode;
        webrtc::EsModes esMode;
	strm->sf->apm->GetEcStatus(enabledFlag, aecMode);
        if(enabledFlag)
        {
            strm->sf->apm->SetEcAudioLogging(true, BJN::getBjnLogfilePath(true).c_str());
            strm->sf->apm->SetEcMetricsLogging(true, BJN::getBjnLogfilePath(true).c_str());
        }
        strm->sf->apm->GetEsmStatus(enabledFlag, esMode);
        if(enabledFlag)
        {
            strm->sf->apm->SetEsmLogging(true, BJN::getBjnLogfilePath(true));
        }
#endif 
    } 
    else 
    {
        strm->sf->apm->StopDebugRecording(); 
#if EXTRA_AUDIO_DEBUG_LOGS
        bool enabledFlag;
        webrtc::EcModes aecMode;
        webrtc::EsModes esMode;
        strm->sf->apm->GetEcStatus(enabledFlag, aecMode);
        if(enabledFlag)
        {
            strm->sf->apm->SetEcAudioLogging(false, BJN::getBjnLogfilePath(true).c_str());
            strm->sf->apm->SetEcMetricsLogging(false, BJN::getBjnLogfilePath(true).c_str());
        }
        strm->sf->apm->GetEsmStatus(enabledFlag, esMode);
        if(enabledFlag)
        {
            strm->sf->apm->SetEsmLogging(false, BJN::getBjnLogfilePath(true));
        }
#endif
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int flag = value;
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: %sableAudioAec()", (flag>0)?"en":"dis"));
    if(flag > 0) 
    {
        if (strm->sf->apm->SetEcStatus(true, kEcAec) == -1) {
            PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to enable AEC value"));
            return PJ_EBUG;
        }
    } else 
    {
            if (strm->sf->apm->SetEcStatus(false) == -1) {
            PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to disable AEC value"));
            return PJ_EBUG;
        }
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec_ms(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int tailLength = value;
    webrtc::EcModes aecMode;
    bool enabledFlag;
    tailLength = std::min(tailLength, 120);
    tailLength = std::max(tailLength, 20);
        
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: setAudioAecMs(%d)", tailLength));
    strm->sf->apm->GetEcStatus(enabledFlag, aecMode);
    if(enabledFlag) 
    {
        strm->sf->apm->SetEcTailLengthMs(tailLength);
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec_plotting(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: %sableAudioAecPlotting(%d)", "en"));
    // TODO (brant): Complete this feature.  Enable via debug build only.
    /*int flag = *((int*) pval);
    bool aecEnabled;
    EcModes aecMode;
    strm->sf->apm->GetEcStatus(aecEnabled, aecMode);
    if(aecEnabled){
    PJ_LOG(4, (THIS_FILE, "AEC Taps Plotting enabled = %d", flag));
    if(flag)
	    strm->sf->apm->SetEcTapsPlotting(true);
    else
	    strm->sf->apm->SetEcTapsPlotting(false);
    }*/
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec_level(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int level = value;
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: setAudioAecLevel(%d)", level));
    return PJ_SUCCESS;
    // TODO (brant): Extend AEC Suppression for tuning.
    /*int tailLength = *((int*) pval);
    webrtc::EcModes aecMode;
    bool enabledFlag;
    tailLength = std::max(tailLength, 120);
    tailLength = std::min(tailLength, 20);
    strm->sf->apm->GetEcStatus(enabledFlag, aecMode);
    strm->sf->apm->SetEcTailLengthMs(tailLength);
    PJ_LOG(4, (THIS_FILE, "setAudioAecLevel(mode = %d) called but AEC is disabled.", tailLength));*/
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec_mode(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
	// TODO (brant) double check these. I think kEcBjn = 5 not 3
    webrtc::EcModes aec_mode = kEcConference;
    switch(value) {
            case 0: aec_mode = kEcAec; break;
            case 1: aec_mode = kEcConference; break;
            case 2: aec_mode = kEcAecm; break;
            case 5: aec_mode = kEcBjn; break;
            default:aec_mode = kEcConference; break;
    }
    strm->sf->apm->SetAecmMode(kAecmSpeakerphone, true);
    strm->sf->apm->SetEcStatus(true, aec_mode);
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aecm_mode(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    webrtc::AecmModes aecm_mode = kAecmLoudSpeakerphone;
    switch(value) {
            case 0: aecm_mode = kAecmQuietEarpieceOrHeadset; break;
            case 1: aecm_mode = kAecmEarpiece; break;
            case 2: aecm_mode = kAecmLoudEarpiece; break;
            case 3: aecm_mode = kAecmSpeakerphone; break;
            case 4: aecm_mode = kAecmLoudSpeakerphone; break;
            default:aecm_mode = kAecmLoudSpeakerphone; break;
    }
    strm->sf->apm->SetEcStatus(true, kEcAecm);
    strm->sf->apm->SetAecmMode(aecm_mode, true);
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_ns(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: enableAudioNS(%d)", value));
    if(value > 0) 
    {   
        if (strm->sf->apm->SetNsStatus(true, kNsHighSuppression) == -1)
        {
            PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to enable Noise Suppression"));
            return PJ_EBUG;
        }
    } else 
    {
        if (strm->sf->apm->SetNsStatus(false) == -1)
        {
            PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to disable Noise Suppression"));
            return PJ_EBUG;
        }
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_ns_level(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int level = value;
    bool enabledFlag;
    webrtc::NsModes noiseMode;
    strm->sf->apm->GetNsStatus(enabledFlag, noiseMode);
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: setAudioNsLevel(%d) ns = %s", 
        level, enabledFlag ? "enabled":"disabled"));
    switch(level) {
        case 0: noiseMode = kNsLowSuppression; break;
        case 1: noiseMode = kNsModerateSuppression; break;
        case 2: noiseMode = kNsHighSuppression; break;
        default: noiseMode = kNsVeryHighSuppression; break;
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_key_noise(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: enableAudioKeyNoise(%d) called", value));
    if(value > 0){
        strm->sf->apm->SetTypingDetectionStatus(strm->typing_detection_enabled);
        strm->sf->apm->EnableKeyNoiseSupp(strm->typing_detection_enabled, kKeynoiseModerate);
        strm->sf->apm->SetTypingDetectionParameters(5, 40, 1, 2, 1);
    } else {
        strm->sf->apm->SetTypingDetectionStatus(false);
        strm->sf->apm->EnableKeyNoiseSupp(false, kKeynoiseModerate);
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_key_mode(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
        PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: key_mode(%d) called", value));
        webrtc::KeynoiseSuppLevel key_mode;
        switch(value) {
            case 0: key_mode = kKeynoiseLow; break;
            case 1: key_mode = kKeynoiseModerate; break;
            case 2: key_mode = kKeynoiseHigh; break;
            case 3: key_mode = kKeynoiseAutomatic; break;
            default:key_mode = kKeynoiseHigh; break;
        }
        strm->sf->apm->EnableKeyNoiseSupp(strm->typing_detection_enabled, key_mode);
        return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_agc(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    bool enableAgc = (value > 0);
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: enableAudioAgc(%d)", value));
#if FB_MACOSX
    if (strm->sf->apm->SetAgcStatus(enableAgc,  kAgcAdaptiveDigital) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to adaptive digital agc"));
        return PJ_EBUG;
    }
#else
    if (strm->sf->apm->SetAgcStatus(enableAgc,  kAgcAdaptiveAnalog) == -1) {
        PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to enable adaptive analog agc"));
        return PJ_EBUG;
    }
#endif
    /* Change the default target level. Other parameters remain unchanged. */
    if(enableAgc) {
        AgcConfig skinnyAgc;
        skinnyAgc.digitalCompressionGaindB = 9;
        skinnyAgc.targetLeveldBOv = 6;
        skinnyAgc.limiterEnable = true;
        
        if (strm->sf->apm->SetAgcConfig(skinnyAgc) == -1){
            PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to configure agc"));
            return PJ_EBUG;
        }
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_agc_const_gain(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int gain = value;
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: setAudioAgcConstGain(%d)", gain));
    if (strm->sf->apm->SetAgcStatus(true,  kAgcFixedDigital) == -1) {
        PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to set Constant Gain Mode for AGC"));
        return PJ_EBUG;
    }

    /* Change the default target level. Other parameters remain unchanged. */
    AgcConfig skinnyAgc;
    // strange implementation in webrtc uses compressor ratio as gain in fixed mode
    skinnyAgc.digitalCompressionGaindB = gain; 
    skinnyAgc.targetLeveldBOv = 0;
    skinnyAgc.limiterEnable = true;
        
    if (strm->sf->apm->SetAgcConfig(skinnyAgc) == -1){
        PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to configure agc"));
        return PJ_EBUG;
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_agc_max_gain(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int gain = value;
    /* Change the default target level. Other parameters remain unchanged. */
    AgcConfig skinnyAgc;
    skinnyAgc.digitalCompressionGaindB = 9;
    skinnyAgc.targetLeveldBOv = gain;
    skinnyAgc.limiterEnable = true;
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: setAudioAgcMaxGain(%d)", gain));
    if (strm->sf->apm->SetAgcConfig(skinnyAgc) == -1){
        PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to configure agc"));
        return PJ_EBUG;
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_delay_ms(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int delayOffset = value;
    strm->sf->apm->SetDelayOffsetMs(delayOffset);
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: setAudioDelayMs(%d)", delayOffset));
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_esm(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    strm->sf->apm->SetEsmStatus(value > 0);
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: SetEsmStatus(%s)",
                (value>0)?"true":"false"));
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_esm_mode(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    bool enabledFlag;
    webrtc::EsModes esmMode;
    strm->sf->apm->GetEsmStatus(enabledFlag, esmMode);
    if(enabledFlag)
    {
        esmMode = MapEsmMode(value);
        std::string modeString = MapEsmModeToString(esmMode);
        PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: SetEsmMode(%s)",
                    modeString.c_str()) );
        if (strm->sf->apm->SetEsmMode(esmMode) == -1) {
            PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to set esm status mode = %s",
                        modeString.c_str()) );
            return PJ_EBUG;
        }
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_esm_level(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    bool enabledFlag;
    webrtc::EsModes esmMode;
    strm->sf->apm->GetEsmStatus(enabledFlag, esmMode);
    if(enabledFlag)
    {
        webrtc::EsSuppression esmLevel = MapEsmLevel(value);
        std::string levelString = MapEsmLevelToString(esmLevel);
        PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: SetEsmLevel(%s)",
                    levelString.c_str()) );
        if (strm->sf->apm->SetEsmLevel(esmLevel) == -1) {
            PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: Failed to set esm status mode = %s",
                        levelString.c_str()) );
            return PJ_EBUG;
        }
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_cng_mode(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: SetCngMode(%d)", value));
    webrtc::ComfortNoiseMode cng_mode;
    switch(value) {
        case 1:  cng_mode = webrtc::kCngModeStatic; break;
        case 2:  cng_mode = webrtc::kCngModeAdaptive; break;
        default: cng_mode = webrtc::kCngModeOff; break;
    }
    strm->sf->apm->SetCngMode(cng_mode);
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_dnm_mute(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    bool remoteMuted = value > 0;
    strm->sf->apm->SetBjnRemoteMuteStatus(remoteMuted);
    return PJ_SUCCESS;
} 

static pj_status_t webrtc_voe_audio_spk_mute(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    strm->sf->volume->SetOutputMute(strm->chan, value != 0);
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_set_typing_detection(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: SetTypingDetection(%s)", (value>0)?"true":"false"));
    strm->typing_detection_enabled = value != 0;
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec_nlp_filtering(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    bool enabledFlag;
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: SetEcNlpDelayFiltering(%s)",
                value > 0 ? "true":"false" ));
    webrtc::EcModes aecMode;
    strm->sf->apm->GetEcStatus(enabledFlag, aecMode);
    if(enabledFlag)
    {
        strm->sf->apm->SetEcNlpDelayFiltering( static_cast<bool>(value > 0) );
    }
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_extra_delay(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int delayMs = value;
    strm->sf->vid_sync->SetExtraPlayoutDelay(strm->chan, delayMs);
    PJ_LOG(4, (THIS_FILE, "JS_AUDIO_CMD: SetExtraPlayoutDelay(%d)", delayMs));
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec_delay_mode(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    webrtc::EcDelayModes aec_delay_mode;
    switch(value) {
            case 0: aec_delay_mode = kEcReportedLatency; break;
            case 1: aec_delay_mode = kEcStaticLatency; break;
            case 2: aec_delay_mode = kEcAlgorithmicLatency; break;
            default:aec_delay_mode = kEcReportedLatency; break;
    }
    // TODO (Brant) is there a nice way to pass the delay value here? Don't think so.
    strm->sf->apm->SetEcDelayMode(aec_delay_mode, 40);
    return PJ_SUCCESS;
}

static pj_status_t webrtc_voe_audio_aec_delay_ms(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    int16_t delayms = static_cast<int16_t>(value);
    PJ_LOG(4, (THIS_FILE, "In function %s with %s %d",
                __FUNCTION__,
                cap.c_str(),
                value));
    strm->sf->apm->SetEcDelayMode(kEcStaticLatency, delayms);
    return PJ_SUCCESS;
};

static pj_status_t webrtc_voe_audio_mic_animation_state(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "In function %s with %s %d",
                __FUNCTION__,
                cap.c_str(),
                value));
    strm->sf->apm->SetMicAnimationState(value);
    return PJ_SUCCESS;
};

static pj_status_t webrtc_voe_audio_reset_driver(webrtc_voe_audio_stream *strm, const std::string& cap, int value)
{
    PJ_LOG(4, (THIS_FILE, "In function %s with %s %d",
                __FUNCTION__,
                cap.c_str(),
                value));
    pjsua_fast_dev_change_info fastDevChangeInfo = {strm->param.dir,
                                                    strm->param.rec_id,
                                                    strm->param.play_id};
    webrtc_voe_stream_set_cap(&strm->base, PJMEDIA_AUD_DEV_FAST_CHANGE_DEVICES, &fastDevChangeInfo);
    return PJ_SUCCESS;
};

enum audio_options {
    BJN_AUDIO_DEBUG,
    BJN_AUDIO_AEC,
    BJN_AUDIO_AEC_MS,
    BJN_AEC_PLOTTING,
    BJN_AUDIO_AEC_LEVEL,
    BJN_AUDIO_AEC_MODE,
    BJN_AUDIO_AECM_MODE,
    BJN_AUDIO_NS,
    BJN_AUDIO_NS_LEVEL,
    BJN_AUDIO_KEY_NOISE,
    BJN_AUDIO_KEY_MODE,
    BJN_AUDIO_AGC,
    BJN_AUDIO_AGC_CONST_GAIN,
    BJN_AUDIO_AGC_MAX_GAIN,
    BJN_AUDIO_DELAY_MS,
    BJN_AUDIO_ESM,
    BJN_AUDIO_ESM_MODE,
    BJN_AUDIO_ESM_LEVEL,
    BJN_AUDIO_DNM_MUTE,
    BJN_AUDIO_SPK_MUTE,
    BJN_AUDIO_CNG_MODE,
    BJN_AUDIO_TYPING_DETECTION,
    BJN_AUDIO_AEC_NLP_FILTERING,
    BJN_AUDIO_EXTRA_DELAY,
    BJN_AUDIO_AEC_DELAY_MODE,
    BJN_AUDIO_AEC_DELAY_MS,
    BJN_AUDIO_MIC_ANIMATION_STATE,
    BJN_AUDIO_RESET_DRIVER,
};

std::map<std::string, audio_options> audio_options_map = boost::assign::map_list_of
    ("debug", BJN_AUDIO_DEBUG)
    ("aec", BJN_AUDIO_AEC)
    ("aec_ms", BJN_AUDIO_AEC_MS)
    ("aec_plotting", BJN_AEC_PLOTTING)
    ("aec_level", BJN_AUDIO_AEC_LEVEL)
    ("aec_mode", BJN_AUDIO_AEC_MODE)
    ("aecm_mode", BJN_AUDIO_AECM_MODE)
    ("ns", BJN_AUDIO_NS)
    ("ns_level", BJN_AUDIO_NS_LEVEL)
    ("key_noise", BJN_AUDIO_KEY_NOISE)
    ("key_mode", BJN_AUDIO_KEY_MODE)
    ("agc", BJN_AUDIO_AGC)
    ("agc_const_gain", BJN_AUDIO_AGC_CONST_GAIN)
    ("agc_max_gain", BJN_AUDIO_AGC_MAX_GAIN)
    ("delay_ms", BJN_AUDIO_DELAY_MS)
    ("esm", BJN_AUDIO_ESM)
    ("esm_mode", BJN_AUDIO_ESM_MODE)
    ("esm_level", BJN_AUDIO_ESM_LEVEL)
    ("dnm_mute", BJN_AUDIO_DNM_MUTE)
    ("spk_mute", BJN_AUDIO_SPK_MUTE)
    ("cng_mode", BJN_AUDIO_CNG_MODE)
    ("typing_detection", BJN_AUDIO_TYPING_DETECTION)
    ("nlp_delay_filter", BJN_AUDIO_AEC_NLP_FILTERING)
    ("extra_delay", BJN_AUDIO_EXTRA_DELAY)
    ("aec_delay_mode", BJN_AUDIO_AEC_DELAY_MODE)
    ("aec_delay_ms", BJN_AUDIO_AEC_DELAY_MS)
    ("set_animation_state", BJN_AUDIO_MIC_ANIMATION_STATE)
    ("reset_driver", BJN_AUDIO_RESET_DRIVER);


static pj_status_t webrtc_voe_stream_set_config_cap(pjmedia_aud_stream *s, const std::string& cap, int value)
{
    struct webrtc_voe_audio_stream *strm = (struct webrtc_voe_audio_stream*)s;


	PJ_LOG(4, (THIS_FILE, "In function %s with %s %d", __FUNCTION__, cap.c_str(), value));

    switch(audio_options_map[cap]) {
        case BJN_AUDIO_DEBUG:
            return webrtc_voe_audio_debug(strm, cap, value);
        case BJN_AUDIO_AEC :
            return webrtc_voe_audio_aec(strm, cap, value);
        case BJN_AUDIO_AEC_MS :
            return webrtc_voe_audio_aec_ms(strm, cap, value);
        case BJN_AEC_PLOTTING :
            return webrtc_voe_audio_aec_plotting(strm, cap, value);
        case BJN_AUDIO_AEC_LEVEL :
            return webrtc_voe_audio_aec_level(strm, cap, value);
        case BJN_AUDIO_AEC_MODE :
            return webrtc_voe_audio_aec_mode(strm, cap, value);
        case BJN_AUDIO_AECM_MODE :
            return webrtc_voe_audio_aecm_mode(strm, cap, value);
        case BJN_AUDIO_NS :
            return webrtc_voe_audio_ns(strm, cap, value);
        case BJN_AUDIO_NS_LEVEL :
            return webrtc_voe_audio_ns_level(strm, cap, value);
        case BJN_AUDIO_KEY_NOISE :
            return webrtc_voe_audio_key_noise(strm, cap, value);
        case BJN_AUDIO_KEY_MODE :
            return webrtc_voe_audio_key_mode(strm, cap, value);
        case BJN_AUDIO_AGC :
            return webrtc_voe_audio_agc(strm, cap, value);
        case BJN_AUDIO_AGC_CONST_GAIN :
            return webrtc_voe_audio_agc_const_gain(strm, cap, value);
        case BJN_AUDIO_AGC_MAX_GAIN :
            return webrtc_voe_audio_agc_max_gain(strm, cap, value);
        case BJN_AUDIO_DELAY_MS :
            return webrtc_voe_audio_delay_ms(strm, cap, value);
        case BJN_AUDIO_ESM :
            return webrtc_voe_audio_esm(strm, cap, value);
        case BJN_AUDIO_ESM_MODE :
            return webrtc_voe_audio_esm_mode(strm, cap, value);
        case BJN_AUDIO_ESM_LEVEL :
            return webrtc_voe_audio_esm_level(strm, cap, value);
        case BJN_AUDIO_DNM_MUTE :
            return webrtc_voe_audio_dnm_mute(strm, cap, value);
        case BJN_AUDIO_SPK_MUTE :
            return webrtc_voe_audio_spk_mute(strm, cap, value);
        case BJN_AUDIO_CNG_MODE :
            return webrtc_voe_audio_cng_mode(strm, cap, value);
        case BJN_AUDIO_TYPING_DETECTION :
            return webrtc_voe_audio_set_typing_detection(strm, cap, value);
        case BJN_AUDIO_AEC_NLP_FILTERING :
            return webrtc_voe_audio_aec_nlp_filtering(strm, cap, value);
        case BJN_AUDIO_EXTRA_DELAY :
            return webrtc_voe_audio_extra_delay(strm, cap, value);
        case BJN_AUDIO_AEC_DELAY_MODE :
            return webrtc_voe_audio_aec_delay_mode(strm, cap, value);
        case BJN_AUDIO_AEC_DELAY_MS :
            return webrtc_voe_audio_aec_delay_ms(strm, cap, value);
        case BJN_AUDIO_MIC_ANIMATION_STATE :
            return webrtc_voe_audio_mic_animation_state(strm, cap, value);
        case BJN_AUDIO_RESET_DRIVER :
            return webrtc_voe_audio_reset_driver(strm, cap, value);
    }
    return PJMEDIA_EAUD_INVCAP;
}

/* API: set capability */
static pj_status_t webrtc_voe_stream_set_cap(pjmedia_aud_stream *s,
				       pjmedia_aud_dev_cap cap,
				       const void *pval)
{
    struct webrtc_voe_audio_stream *strm = (struct webrtc_voe_audio_stream*)s;

    PJ_UNUSED_ARG(strm);
    PJ_ASSERT_RETURN(s, PJ_EINVAL);
    if (cap == PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING)
    {
        float level = 0.0f;
        level = (*((unsigned int*) pval))/100.0f;
        level = level*255.0f;
        strm->sf->volume->SetSpeakerVolume((unsigned int)level);
        return PJ_SUCCESS;
    } 
    else if (cap == PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING)
    {
        float level = 0.0f;
        level = (*((unsigned int*) pval))/100.0f;
        level = level*255.0f;
        strm->sf->volume->SetMicVolume((unsigned int)level);
        return PJ_SUCCESS;
    }
    else if(cap == PJMEDIA_AUD_DEV_CAP_CALL_MED) {
        if(pval && !strm->call_med) {
            pjsua_webrtc_voe_info *voe_info = (pjsua_webrtc_voe_info*) pval;
            strm->call_med = voe_info->call_med;
            webrtc_voe_start_local_monitor(strm, false);

            //G722 uses static payload type 9.
            if(voe_info->si->tx_pt == 9) {
                strm->sf->codec->SetSendCodec(strm->chan, g722);
            } else {
                strm->sf->codec->SetSendCodec(strm->chan, opus);
            }

            strm->sf->rtp_rtcp->SetLocalSSRC(strm->chan, strm->call_med->ssrc);

            /* Use saved RTP timestamp & sequence number when the flag
             * to force these values is set. This is to ensure contiguous
             * timestamp & sequence when audio stream is created and destroyed
             * multiple times in the same call session.
             */
            if (bjn_sky::getAudioFeatures()->mCacheRtpSeqTs ||
                bjn_sky::getAudioFeatures()->mEnableDisableAudioEngine)
            {
                if (strm->call_med->rtp_tx_seq_ts_set & 1)
                {
                    PJ_LOG(4, (THIS_FILE, "Setting RTP tx sequence number to %u", strm->call_med->rtp_tx_seq));
                    strm->sf->rtp_rtcp->SetLocalSequenceNumber(strm->chan, strm->call_med->rtp_tx_seq);
                    strm->call_med->rtp_tx_seq_ts_set &= ~1;
                }

                if (strm->call_med->rtp_tx_seq_ts_set & (1 << 1))
                {
                    PJ_LOG(4, (THIS_FILE, "Setting RTP tx timestamp to %u", strm->call_med->rtp_tx_ts));
                    strm->sf->rtp_rtcp->SetLocalTimestamp(strm->chan, strm->call_med->rtp_tx_ts);
                    strm->call_med->rtp_tx_seq_ts_set &= ~(1 << 1);
                }
            }
            strm->sf->base1->StartSend(strm->chan);

            if(!strm->bjnVadObserver)
                strm->bjnVadObserver = new BJNVoERxVadCallback(strm);
            strm->sf->apm->RegisterRxVadObserver(strm->chan, *strm->bjnVadObserver);

            if(!strm->bjnEsmObserver)
                strm->bjnEsmObserver = new BJNVoEEsmStateCallback(strm);
            strm->sf->apm->RegisterEsmStateObserver(*strm->bjnEsmObserver);

            if(!strm->bjnEcObserver)
                strm->bjnEcObserver = new BJNVoeEcStarvationCallback(strm);
            strm->sf->apm->RegisterEcBufferStarvationObserver(*strm->bjnEcObserver);

            if(!strm->bjnAudDeviceObserver)
                strm->bjnAudDeviceObserver = new BJNVoEUnsupportedDeviceCallback(strm);
            strm->sf->apm->RegisterAudioDeviceWarningObserver(*strm->bjnAudDeviceObserver);

            if(!strm->bjnAudNotificationObserver)
                strm->bjnAudNotificationObserver = new BJNVoEAudioNotificationCallback(strm);
            strm->sf->apm->RegisterAudioNotificationObserver(*strm->bjnAudNotificationObserver);

            if(bjn_sky::getAudioFeatures()->mReportAudioDspStats) {
                if(!strm->bjnAudioStatsObserver)
                    strm->bjnAudioStatsObserver = new BJNVoEAudioStatsCallback(strm);
                strm->sf->apm->RegisterAudioStatsObserver(*strm->bjnAudioStatsObserver);
            }

            if(bjn_sky::getAudioFeatures()->mAnimatedMicIcon) {
                if(!strm->bjnAudioMicAnimationObserver)
                    strm->bjnAudioMicAnimationObserver = new BJNVoEAudioMicAnimationCallback(strm);
                strm->sf->apm->RegisterAudioMicAnimationObserver(*strm->bjnAudioMicAnimationObserver);
            }
        } else if(!pval) {
            strm->sf->apm->DeRegisterRxVadObserver(strm->chan);
            strm->sf->apm->DeRegisterEsmStateObserver();
            strm->sf->apm->DeRegisterEcBufferStarvationObserver();
            strm->sf->apm->DeRegisterAudioDeviceWarningObserver();
            strm->sf->apm->DeRegisterAudioNotificationObserver();
            strm->sf->apm->DeRegisterAudioStatsObserver();
            strm->sf->apm->DeRegisterAudioMicAnimationObserver();
            strm->sf->base1->StopSend(strm->chan);
            strm->call_med = (pjsua_call_media*) pval;
        }
        return PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_CAP_MUTE_AUDIO) {
        // Sip manager sends the mute value with audio send on or off.
        // So invert the value for muting input signal.
        bool mute = !(*((bool*) pval));
        strm->sf->volume->SetInputMute(strm->chan, mute);
        strm->sf->apm->SetBjnMuteStatus(mute);
        return PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_START_TEST_SOUND) {
        if(pval) {
            if (0 != strm->sf->file->StartPlayingFileLocally(strm->chan, (const char *)pval, false, kFileFormatWavFile)) {
                // VoEBase contains the last failure code!
                PJ_LOG(4, (THIS_FILE, "Failed to play file, test sound failed, last error: %d", strm->sf->base1->LastError()));
            }
        }
        else {
            strm->sf->file->StopPlayingFileLocally(strm->chan);
        }
        return PJ_SUCCESS;
    } else if(cap == PJMEDIA_AUD_DEV_FAST_CHANGE_DEVICES) {
        pjsua_fast_dev_change_info* fastDevChangeInfo = (pjsua_fast_dev_change_info *) pval;
        webrtc_voe_stream_stop(s);
        strm->param.dir = fastDevChangeInfo->dir;
        strm->param.rec_id = fastDevChangeInfo->rec_id;
        strm->param.play_id = fastDevChangeInfo->play_id;
        webrtc_voe_stream_start(s);
        if(strm->call_med) {
            strm->sf->rtp_rtcp->SetLocalSSRC(strm->chan, strm->call_med->ssrc);
            strm->sf->base1->StartSend(strm->chan);
        }
        return PJ_SUCCESS;
    }
    else if(cap == PJMEDIA_AUD_DEV_CAP_CONFIG_OPTIONS)
    {
        bjn_sky::AudioConfigOptions* options = (bjn_sky::AudioConfigOptions*) pval;
        if (options)
        {
            bjn_sky::AudioConfigOptions::const_iterator it;
            for (it = options->begin(); it != options->end(); ++it)
            {
                std::string name = it->first;
                int value = it->second;
                webrtc_voe_stream_set_config_cap(s, name, value);
            }
        }
        return PJ_SUCCESS;
    }
    else if(cap == PJMEDIA_AUD_DEV_CAP_MIC_MONITOR)
    {
        pj_status_t status = PJ_SUCCESS;
        if(pval)
        {
            bool startLocalMicMonitor = *(static_cast<const bool*>(pval));
            webrtc_voe_start_local_monitor(strm, startLocalMicMonitor);
        }
        else
        {
            status = PJ_EINVAL;
        }
        return status;
    }
    return PJMEDIA_EAUD_INVCAP;
}

/* API: Start stream. */
static pj_status_t webrtc_voe_stream_start(pjmedia_aud_stream *strm)
{
    struct webrtc_voe_audio_stream *stream = (struct webrtc_voe_audio_stream*)strm;
    char device_name[128];
    Config_Parser_Handler::DevicePropertiesMap mic_device_props, spk_device_props;
    typedef Config_Parser_Handler::DevicePropertiesMap::iterator device_props_iter;
    int rec_devices = 0, play_devices = 0;
    bool mic_aec_disabled=false, spkr_aec_disabled=false;
    bool ensureNonZeroSpeakerVolume = false;
    webrtc_audio_dsp_config dsp_config;
    std::string device_prop_str;
    unsigned spk_vol = -1;
    bool spk_mute = false;

    PJ_LOG(4, (THIS_FILE, "In function %s", __FUNCTION__));

    stream->sf->hardware->GetNumOfRecordingDevices(rec_devices);
    stream->sf->hardware->GetNumOfPlayoutDevices(play_devices);

    /* Create player stream here */
    if (stream->param.dir & PJMEDIA_DIR_PLAYBACK) {
        PJ_LOG(4, (THIS_FILE, "Setting playback index %d %d", stream->param.play_id, rec_devices));
        stream->sf->hardware->SetPlayoutDevice((stream->param.play_id - rec_devices));
    }

    /* Create capture stream here */
    if (stream->param.dir & PJMEDIA_DIR_CAPTURE) {
        PJ_LOG(4, (THIS_FILE, "Setting capture index %d", stream->param.rec_id));
        stream->sf->hardware->SetRecordingDevice(stream->param.rec_id);
    }

    stream->sf->codec->SetVADStatus(stream->chan, true, kVadAggressiveLow, true); //setting dtx mode off
    
    if (stream->param.dir & PJMEDIA_DIR_CAPTURE)
    {
        std::string device_key;
        device_name[0] = 0;
        stream->sf->hardware->GetRecordingDeviceName(stream->param.rec_id, device_name, stream->cap_device_id);
        PJ_ASSERT_RETURN(stream->sf->cph_instance, PJ_EUNKNOWN);
        GetDeviceConfigProperties(DeviceConfigTypeMic, device_name, stream->cap_device_id, mic_device_props, device_key, *stream->sf->cph_instance);
        strncpy(stream->cap_device_id, device_key.c_str(), 128);

        unsigned mic_lvl;
        // retrieve input device's volume here
        if(stream->sf->volume->GetMicVolume(mic_lvl) != -1){
            PJ_LOG(4, (THIS_FILE, "Original mic level %u",mic_lvl));
            stream->original_mic_level = mic_lvl;
        }
        if(mic_device_props.size() > 0) 
        {
            PJ_LOG(4, (THIS_FILE, "Parsing Device Properties Map for Micrphone"));
            for(device_props_iter iter = mic_device_props.begin(); iter != mic_device_props.end(); iter++) {
                PJ_LOG(4, (THIS_FILE, "Device: %s Property: %s Value: %d", 
                    device_name, iter->first.c_str(), iter->second));
                if(iter->first == "aec_enabled")
                {
                    (iter->second==0) ? (mic_aec_disabled=true):(mic_aec_disabled=false);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: aec_enabled = %s", 
                        mic_aec_disabled ? "false":"true"));
                    continue;
                }
                if(iter->first == "aec_tail_length_ms")
                {
                    dsp_config.aec_tail_length_ms = std::min(120, std::max(20, iter->second));
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: aec_tail_length_ms = %d", 
                        dsp_config.aec_tail_length_ms));
                    continue;
                }
                if(iter->first == "aec_mode")
                {
                    switch(iter->second)
                    {
                        case 0:
                            dsp_config.aec_mode = kEcAec;
                            break;
                        case 1:
                            dsp_config.aec_mode = kEcConference;
                            break;
                        case 2:
                            dsp_config.aec_mode = kEcAecm;
                            break;
                        case 5 :
                            if(stream->enable_custom_aec)
                                dsp_config.aec_mode = kEcBjn;
                            break;
                        default:
                            dsp_config.aec_mode = kEcConference;
                            break;
                    }
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: aec_mode = %d", 
                        dsp_config.aec_mode));
                    continue;
                }
                if(iter->first == "aec_delay_mode")
                {
                    switch(iter->second)
                    {
                        case 0:
                            dsp_config.aec_delay_mode = kEcReportedLatency;
                            break;
                        case 1:
                            dsp_config.aec_delay_mode = kEcStaticLatency;
                            break;
                        case 2:
                            dsp_config.aec_delay_mode = kEcAlgorithmicLatency;
                            break;
                        default:
                            dsp_config.aec_delay_mode = kEcReportedLatency;
                            break;
                    };
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: aec_delay_mode = %d", 
                        dsp_config.aec_delay_mode));
                    continue;
                }
                if(iter->first == "aec_static_delay")
                {
                    dsp_config.aec_static_delay = iter->second;
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: aec_static_delay = %d",
                                dsp_config.aec_static_delay));
                }

                if(iter->first == "ns_enabled")
                {
                    (iter->second>0)?(dsp_config.ns_enabled=true):(dsp_config.ns_enabled=false);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: ns_enabled = %s", 
                        dsp_config.ns_enabled ? "true":"false"));
                    continue;
                }
                if(iter->first == "ns_mode")
                {
                    switch(iter->second)
                    {
                        case 0:
                            dsp_config.ns_mode = kNsLowSuppression;
                            break;
                        case 1:
                            dsp_config.ns_mode = kNsModerateSuppression;
                            break;
                        case 2:
                            dsp_config.ns_mode = kNsHighSuppression;
                            break;
                        case 3:
                            dsp_config.ns_mode = kNsVeryHighSuppression;
                            break;
                        default:
                            dsp_config.ns_mode = kNsHighSuppression;
                            break;
                    }
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: ns_mode = %d", 
                        dsp_config.ns_mode));
                    continue;
                }
                if(iter->first == "keynoise_enabled")
                {
                    (iter->second>0)?(dsp_config.keynoise_enabled=true):(dsp_config.keynoise_enabled=false);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: keynoise_enabled = %s", 
                        dsp_config.keynoise_enabled ? "true":"false"));
                    continue;
                }
                if(iter->first == "keynoise_mode")
                {
                    switch(iter->second)
                    { 
                        case 0:
                            dsp_config.keynoise_mode = kKeynoiseLow;
                            break;
                        case 1:
                            dsp_config.keynoise_mode = kKeynoiseModerate;
                            break;
                        case 2:
                            dsp_config.keynoise_mode = kKeynoiseHigh;
                            break;
                        case 3:
                            dsp_config.keynoise_mode = kKeynoiseAutomatic;
                            break;
                        default:
                            dsp_config.keynoise_mode = kKeynoiseHigh;
                            break;
                    }
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: keynoise_mode = %d", 
                        dsp_config.keynoise_mode));
                    continue;
                }
                if(iter->first == "agc_enabled")
                {
                    (iter->second > 0)?(dsp_config.agc_enabled=true):(dsp_config.agc_enabled=false);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: agc_enabled = %s", 
                        dsp_config.agc_enabled ? "true":"false"));
                    continue;
                }
                if(iter->first == "agc_const_gain_enabled")
                {
                    if(iter->second > 0)
                    {
                        dsp_config.agc_mode = kAgcFixedDigital;
                        dsp_config.agc_const_gain_enabled = true;
                    }
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: agc_const_gain_enabled = %s", 
                        dsp_config.agc_const_gain_enabled ? "true":"false"));
                    continue;
                }
                if(iter->first == "agc_max_gain_db")
                {
                    dsp_config.agc_max_gain_db = std::max(0, std::min(iter->second, 24));
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: agc_max_gain_db = %d", 
                        dsp_config.agc_max_gain_db));
                    continue;
                }
                if(iter->first == "audio_delay_offset_ms")
                {
                    dsp_config.audio_delay_offset_ms = iter->second;
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: audio_delay_offset_ms = %d", 
                        dsp_config.audio_delay_offset_ms));
                    continue;
                }
                if(iter->first == "esm_mode")
                {
                    webrtc::EsModes esmMode = MapEsmMode( iter->second );
                    if( esmMode > dsp_config.esm_mode )
                        dsp_config.esm_mode = esmMode;
                    std::string esmModeStr = MapEsmModeToString(dsp_config.esm_mode);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: esm_mode = %s", 
                            esmModeStr.c_str() ) );
                    continue;
                }
                if(iter->first == "esm_level") {
                    webrtc::EsSuppression esmLevel = MapEsmLevel( iter->second );
                    if( esmLevel > dsp_config.esm_level ) {
                        dsp_config.esm_level = esmLevel;
                    }
                    std::string esmLevelStr = MapEsmLevelToString(dsp_config.esm_level);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: esm_level = %s", 
                            esmLevelStr.c_str() ) );
                    continue;
                }
                if(iter->first == "delay_estimator_enabled")
                {
                    (iter->second > 0)?(dsp_config.delay_estimator_enabled=true):(dsp_config.delay_estimator_enabled=false);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: delay_estimator_enabled = %s", 
                        dsp_config.delay_estimator_enabled ? "true":"false"));
                    continue;
                }
                if(iter->first == "cng_mode")
                {
                    dsp_config.cng_mode = MapCngMode(iter->second);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: cng_mode = %d", 
                        dsp_config.cng_mode));
                    continue;
                }
                if(iter->first == "mic_level")
                {
                    dsp_config.mic_level = std::max(std::min(100, iter->second),0);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: mic_level = %d", 
                            dsp_config.mic_level));
                }
                if(iter->first == "agc_digital_gain_db")
                {
                    dsp_config.agc_digital_gain_db = iter->second;
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: agc_digital_gain_db = %d",
                               dsp_config.agc_digital_gain_db));
                }
                if(iter->first == "aec_nlp_delay_filter")
                {
                    dsp_config.aec_nlp_delay_filter |= (iter->second != 0);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: aec_nlp_delay_filter = %s",
                        dsp_config.aec_nlp_delay_filter ? "true":"false"));
                    continue;
                }
                if(iter->first == "bjn_aec_tail_length_ms")
                {
                    dsp_config.bjn_aec_tail_length_ms = iter->second;
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: bjn_aec_tail_length_ms = %d",
                        dsp_config.bjn_aec_tail_length_ms));
                    continue;
                }
                if(iter->first == "bjn_esm_mode")
                {
                    webrtc::EsModes esmMode = MapEsmMode( iter->second );
                    if( esmMode > dsp_config.bjn_esm_mode )
                        dsp_config.bjn_esm_mode = esmMode;
                    std::string esmModeStr = MapEsmModeToString(dsp_config.bjn_esm_mode);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: bjn_esm_mode = %s",
                            esmModeStr.c_str() ) );
                    continue;
                }
                if(iter->first == "bjn_esm_level") {
                    webrtc::EsSuppression esmLevel = MapEsmLevel( iter->second );
                    if( esmLevel > dsp_config.bjn_esm_level ) {
                        dsp_config.bjn_esm_level = esmLevel;
                    }
                    std::string esmLevelStr = MapEsmLevelToString(dsp_config.bjn_esm_level);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: bjn_esm_level = %s",
                            esmLevelStr.c_str() ) );
                    continue;
                }
            }
        }
    }


    if (stream->param.dir & PJMEDIA_DIR_PLAYBACK)
    {
        std::string device_key;
        device_name[0] = 0;
        stream->original_spk_level = -1;
        stream->original_spk_mute_state = UNKNOWN;
        stream->restore_spk_state = false;
        stream->sf->hardware->GetPlayoutDeviceName((stream->param.play_id - rec_devices), device_name, stream->play_device_id);
        PJ_ASSERT_RETURN(stream->sf->cph_instance, PJ_EUNKNOWN);
        GetDeviceConfigProperties(DeviceConfigTypeSpeaker, device_name, stream->play_device_id, spk_device_props, device_key, *stream->sf->cph_instance);
        strncpy(stream->play_device_id, device_key.c_str(), 128);
        if(spk_device_props.size() > 0) {
            PJ_LOG(4, (THIS_FILE, "Parsing Device Properties Map for Speaker"));
            for(device_props_iter iter = spk_device_props.begin(); iter != spk_device_props.end(); iter++) {
                PJ_LOG(4, (THIS_FILE, "Device: %s Property: %s Value: %d", device_name, iter->first.c_str(), iter->second));
                if(iter->first == "aec_enabled")
                {
                    (iter->second==0) ? (spkr_aec_disabled=true):(spkr_aec_disabled=false);
                    PJ_LOG(4, (THIS_FILE, "XML Spkr Property: aec_enabled = %s", 
                        spkr_aec_disabled ? "false":"true"));
                    continue;
                }
                if(iter->first == "aec_tail_length_ms")
                {
                    // Should use the larger of two taillengths defined. Speaker param can only increase taillength.
                    dsp_config.aec_tail_length_ms = std::max(dsp_config.aec_tail_length_ms, iter->second);
                    dsp_config.aec_tail_length_ms = std::min(dsp_config.aec_tail_length_ms, 120);
                    PJ_LOG(4, (THIS_FILE, "XML Spkr Property: aec_tail_length_ms = %d", 
                        dsp_config.aec_tail_length_ms));
                    continue;
                }
                if(iter->first == "aec_delay_mode")
                {
                    switch(iter->second)
                    {
                        case 0:
                            dsp_config.aec_delay_mode = 
                                std::max(dsp_config.aec_delay_mode, kEcReportedLatency);
                            break;
                        case 1:
                            dsp_config.aec_delay_mode = 
                                std::max(dsp_config.aec_delay_mode, kEcStaticLatency);
                            break;
                        case 2:
                            dsp_config.aec_delay_mode = 
                                std::max(dsp_config.aec_delay_mode, kEcAlgorithmicLatency);
                            break;
                        default:
                            dsp_config.aec_delay_mode = 
                                std::max(dsp_config.aec_delay_mode, kEcReportedLatency);
                            break;
                    };
                    PJ_LOG(4, (THIS_FILE, "XML Spkr Property: aec_delay_mode = %d", 
                        dsp_config.aec_delay_mode));
                    continue;
                }
                if(iter->first == "aec_static_delay")
                {
                    dsp_config.aec_static_delay = iter->second;
                    PJ_LOG(4, (THIS_FILE, "XML Spkr Property: aec_static_delay = %d",
                                dsp_config.aec_static_delay));
                }
                if(iter->first == "audio_delay_offset_ms")
                {
                    dsp_config.audio_delay_offset_ms += iter->second;
                    PJ_LOG(4, (THIS_FILE, "XML Spkr Property: audio_delay_offset_ms = %d", 
                        dsp_config.audio_delay_offset_ms));
                    continue;
                }
                if(iter->first == "delay_estimator_enabled")
                {
                    if(!dsp_config.delay_estimator_enabled)
                    {
                        (iter->second > 0)?(dsp_config.delay_estimator_enabled=true):(dsp_config.delay_estimator_enabled=false);
                    }
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: audio_delay_offset_ms = %s", 
                        dsp_config.delay_estimator_enabled ? "true":"false"));
                    continue;
                }
                if(iter->first == "esm_mode")
                {
                    webrtc::EsModes esmMode = MapEsmMode(iter->second);
                    if(esmMode > dsp_config.esm_mode)
                        dsp_config.esm_mode = esmMode;
                    std::string esmModeStr = MapEsmModeToString(dsp_config.esm_mode);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: esm_mode = %s", 
                            esmModeStr.c_str() ) );
                    continue;
                }
                if(iter->first == "esm_level")
                {
                    webrtc::EsSuppression esmLevel = MapEsmLevel(iter->second);
                    if(esmLevel > dsp_config.esm_level)
                        dsp_config.esm_level = esmLevel;
                    std::string esmLevelStr = MapEsmLevelToString(dsp_config.esm_level);
                    PJ_LOG(4, (THIS_FILE, "XML Mic Property: esm_level = %s", 
                            esmLevelStr.c_str() ) );
                    continue;
                }
                if(iter->first == "aec_nlp_delay_filter")
                {
                    dsp_config.aec_nlp_delay_filter |= (iter->second != 0);
                    PJ_LOG(4, (THIS_FILE, "XML Speaker Property: aec_nlp_delay_filter = %s",
                        dsp_config.aec_nlp_delay_filter ? "true":"false"));
                    continue;
                }
                if(iter->first == "bjn_aec_tail_length_ms")
                {
                    dsp_config.bjn_aec_tail_length_ms = iter->second;
                    PJ_LOG(4, (THIS_FILE, "XML Speaker Property: bjn_aec_tail_length_ms = %d",
                        dsp_config.bjn_aec_tail_length_ms));
                    continue;
                }
                if(iter->first == "bjn_esm_mode")
                {
                    webrtc::EsModes esmMode = MapEsmMode( iter->second );
                    if( esmMode > dsp_config.bjn_esm_mode )
                        dsp_config.bjn_esm_mode = esmMode;
                    std::string esmModeStr = MapEsmModeToString(dsp_config.bjn_esm_mode);
                    PJ_LOG(4, (THIS_FILE, "XML Spkr Property: bjn_esm_mode = %s",
                            esmModeStr.c_str() ) );
                    continue;
                }
                if(iter->first == "bjn_esm_level") {
                    webrtc::EsSuppression esmLevel = MapEsmLevel( iter->second );
                    if( esmLevel > dsp_config.bjn_esm_level ) {
                        dsp_config.bjn_esm_level = esmLevel;
                    }
                    std::string esmLevelStr = MapEsmLevelToString(dsp_config.bjn_esm_level);
                    PJ_LOG(4, (THIS_FILE, "XML Spkr Property: bjn_esm_level = %s",
                            esmLevelStr.c_str() ) );
                    continue;
                }

                if(iter->first == "spk_auto_system_unmute") {
                    if(bjn_sky::getAudioFeatures()->mSpeakerAutoUnmute
                            && iter->second != 0)
                    {
                        // Call ensureVolume API after stream has been set
                        // so that we can query current volume.
                        ensureNonZeroSpeakerVolume = true;
                    }
                }
            }
        }
    }
    (spkr_aec_disabled && mic_aec_disabled) ? (dsp_config.aec_enabled = false):(dsp_config.aec_enabled = true);
    PJ_LOG(4, (THIS_FILE, "XML Audio Property: aec_enabled = %s",
                        dsp_config.aec_enabled ? "true":"false"));

    /* Now that configuration is complete, call the WebRTC API to setup the DSP stack. */
    if(stream->sf->volume->SetChannelOutputVolumeScaling(stream->chan, 2.0f) != 0) {
        PJ_LOG(4, (THIS_FILE, "Failed to set output volume scaling."));
        return PJ_EBUG;
    }
    if (stream->sf->apm->EnableHighPassFilter(true) == -1){
        PJ_LOG(4, (THIS_FILE, "Failed to enable HPF"));
        return PJ_EBUG;
    }

    if(dsp_config.aec_enabled && dsp_config.aec_mode == kEcAecm) {
        if (stream->sf->apm->SetAecmMode(dsp_config.aecm_mode, true) == -1) {
            PJ_LOG(4, (THIS_FILE, "Failed to set AECM mode"));
            return PJ_EBUG;
        }
        if (stream->sf->apm->SetEcStatus(dsp_config.aec_enabled, dsp_config.aec_mode) == -1) {
            PJ_LOG(4, (THIS_FILE, "Failed to enable AEC"));
            return PJ_EBUG;
        }
    } else if(dsp_config.aec_enabled) {
        // This enablement kicks in only if the OS specific enablement is set and the external enablement is
        // set.
        if(stream->enable_custom_aec_external && stream->enable_custom_aec) {
            dsp_config.aec_mode = kEcBjn;
            PJ_LOG(4, (THIS_FILE, "BJN AEC is enabled for internal and external devices."));
        }
        if (stream->sf->apm->SetEcStatus(dsp_config.aec_enabled, dsp_config.aec_mode) == -1) {
            PJ_LOG(4, (THIS_FILE, "Failed to enable AEC"));
            return PJ_EBUG;
        }
        if (stream->sf->apm->SetEcMetricsStatus(true) == -1) {
            PJ_LOG(4, (THIS_FILE, "Failed to set AEC metrics"));
            return PJ_EBUG;
        }
        if(dsp_config.aec_mode == kEcBjn) {
            if (stream->sf->apm->SetEcTailLengthMs(dsp_config.bjn_aec_tail_length_ms) == -1) {
                PJ_LOG(4, (THIS_FILE, "Possible error setting the AEC tail length to %d ms", dsp_config.aec_tail_length_ms));
            }
            if (stream->sf->apm->SetEcDelayMode(dsp_config.aec_delay_mode, dsp_config.aec_static_delay)) {
                 PJ_LOG(4, (THIS_FILE, "Possible error setting the AEC Delay Mode %d  with %d ms delay", 
                             dsp_config.aec_delay_mode,
                             dsp_config.audio_delay_offset_ms));
            }
        } else {
            if (stream->sf->apm->SetEcTailLengthMs(dsp_config.aec_tail_length_ms) == -1) {
                PJ_LOG(4, (THIS_FILE, "Possible error setting the AEC tail length to %d ms", dsp_config.aec_tail_length_ms));
            }
        }
        if (stream->sf->apm->SetEcNlpDelayFiltering(dsp_config.aec_nlp_delay_filter) == -1) {
            PJ_LOG(4, (THIS_FILE, "Possible error setting the AEC NLP Delay Filtering %s ms",
                        dsp_config.aec_nlp_delay_filter ? "true":"false" ));
        }
        stream->sf->apm->EnableDriftCompensation(true);

        if(bjn_sky::getAudioFeatures()->mAecDelayLogging) {
            PJ_LOG(4, (THIS_FILE, "AEC Delay Logging Enabled"));
            if(stream->sf->apm->SetEcDelayLogging(true, BJN::getBjnLogfilePath(true)) != 0){
                PJ_LOG(4, (THIS_FILE, "Failed to enable AEC delay logging"));
            }
        }

        if (bjn_sky::getAudioFeatures()->mAecModifiedBuffer) {
            PJ_LOG(4, (THIS_FILE, "AEC Modified Buffer Logic Enabled"));
            if (stream->sf->apm->SetEcModifiedBufferLogic(true)  != 0){
                PJ_LOG(4, (THIS_FILE, "Failed to enable modified AEC buffer logic"));
            }
        }

        PJ_LOG(4, (THIS_FILE, "AEC Settings mode = %d delay_mode = %d delay_ms = %d tail = %d ms nlp_filtering = %s drift = %s",
                dsp_config.aec_mode,
                dsp_config.aec_delay_mode,
                dsp_config.aec_static_delay,
                (dsp_config.aec_mode == kEcBjn) ? dsp_config.bjn_aec_tail_length_ms:dsp_config.aec_tail_length_ms,
                dsp_config.aec_nlp_delay_filter ? "true":"false",
                "true" ) );
    }
    if (stream->sf->apm->SetEsmDelayEstimatorStatus(dsp_config.delay_estimator_enabled) == -1){
        PJ_LOG(4, (THIS_FILE, "Failed to set Delay Estimator"));
        return PJ_EBUG;
    }
    if( dsp_config.aec_mode == kEcBjn ) {
        dsp_config.esm_mode = dsp_config.bjn_esm_mode;
        dsp_config.esm_level = dsp_config.bjn_esm_level;
    }
    if (stream->sf->apm->SetEsmStatus(dsp_config.esm_enabled) == -1)
    {
        PJ_LOG(4, (THIS_FILE, "Failed to set Echo State Machine"));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetEsmMode(dsp_config.esm_mode) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set Echo State Machine Mode"));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetEsmLevel(dsp_config.esm_level) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set Echo State Machine Level"));
        return PJ_EBUG;
    }
    stream->sf->apm->SetDelayOffsetMs(dsp_config.audio_delay_offset_ms);

#if FB_X11
    if (bjn_sky::getAudioFeatures()->mDigitalAgcLinux) {
        dsp_config.mic_level = 100;
        dsp_config.agc_mode = kAgcAdaptiveDigital;
    }
#endif

    if (stream->sf->apm->SetAgcStatus(dsp_config.agc_enabled, dsp_config.agc_mode) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to enable AGC"));
        return PJ_EBUG;
    }
    if(stream->sf->apm->SetCngMode(kCngModeStatic) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set CNG mode"));
        return PJ_EBUG;
    }
    if(dsp_config.agc_enabled)
    {
        /* Change the default target level. Other parameters remain unchanged. */
        AgcConfig skinnyAgc;
        // strange implementation in webrtc uses compressor ratio as gain in fixed mode
        if(dsp_config.agc_const_gain_enabled)
        {
            skinnyAgc.digitalCompressionGaindB = dsp_config.agc_max_gain_db; 
        }
        else
        {
            skinnyAgc.digitalCompressionGaindB = 9; 
        }
        skinnyAgc.targetLeveldBOv = 6;
        skinnyAgc.limiterEnable = true;
        if (stream->sf->apm->SetAgcConfig(skinnyAgc) == -1){
            PJ_LOG(4, (THIS_FILE, "Failed to configure AGC"));
            return PJ_EBUG;
        }
        if (stream->sf->apm->SetAgcDigitalGainDb(dsp_config.agc_digital_gain_db)==-1)
        {
            PJ_LOG(4, (THIS_FILE, "Failed to set Agc Digital Gain."));
        }
    }
    if (stream->sf->apm->SetNsStatus(dsp_config.ns_enabled,  dsp_config.ns_mode) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set NS"));
        return PJ_EBUG;
    }

    if (stream->sf->apm->SetTypingDetectionStatus(stream->typing_detection_enabled &&
        dsp_config.keynoise_enabled) == -1) 
    {
        PJ_LOG(4, (THIS_FILE, "Failed to enable Typing Detection"));
    }
    if(stream->sf->apm->SetTypingDetectionParameters(5, 40, 10, 2, 1) == -1)
    {
        PJ_LOG(4, (THIS_FILE, "Failed to set Typing Detection Parameters"));
    }
    if (stream->sf->apm->EnableKeyNoiseSupp(stream->typing_detection_enabled && 
        dsp_config.keynoise_enabled, dsp_config.keynoise_mode)==-1)
    {
        PJ_LOG(4, (THIS_FILE, "Failed to enable Keyboard Noise Suppression"));
    }

    stream->sf->base1->StartPlayout(stream->chan);
    webrtc_voe_start_local_monitor(stream, true);

    // Temporary hack to address SKY-1639. The root cause of the issue is that
    // the preferece pannel mute the input when dragging the slider all the way
    // to 0. Some other applications probably do to.
    // A nicer fix would be to provide UI feedback and a way to unmute the system
    // setting.
    // Appending to this hack, another hack ... SKY-3532: We need to reduce
    // the microphone level for some mac models, as they are subject to 
    // saturation during far-talk situations, resulting in some gnarly echo.
#if FB_MACOSX
    if(stream->sf->volume->SetMicVolume((dsp_config.mic_level * 255)/100)) {
        PJ_LOG(4, (THIS_FILE, "Failed to set the Microphone Level"));
    }
    stream->sf->volume->SetSystemInputMute(false);
#endif

#if FB_X11
    if (bjn_sky::getAudioFeatures()->mDigitalAgcLinux) {
        if(stream->sf->volume->SetMicVolume((dsp_config.mic_level * 255)/100)) {
            PJ_LOG(4, (THIS_FILE, "Failed to set the Microphone Level"));
        }
    }
#endif

    if(stream->param.dir & PJMEDIA_DIR_PLAYBACK)
    {
        if(stream->sf->volume->GetSpeakerVolume(spk_vol) == 0){
            PJ_LOG(4, (THIS_FILE, "Original spk level %u",spk_vol));
                stream->original_spk_level = spk_vol;
        }

        if(stream->sf->volume->GetSystemOutputMute(spk_mute) == 0){
            PJ_LOG(4, (THIS_FILE, "Original spk mute status %d",spk_mute));
                stream->original_spk_mute_state = spk_mute ? MUTED : UNMUTED;
        }

        if(ensureNonZeroSpeakerVolume == true)
        {
			// ???
            // stream->sf->volume->EnsureNonZeroSpeakerVolume();
            // stream->restore_spk_state = true;
        }
    }

    PushEcMode(stream,dsp_config.aec_mode);
    return PJ_SUCCESS;
}

/* API: Stop stream. */
static pj_status_t webrtc_voe_stream_stop(pjmedia_aud_stream *strm)
{
    struct webrtc_voe_audio_stream *stream = (struct webrtc_voe_audio_stream*)strm;
    PJ_LOG(4, (THIS_FILE, "In function %s", __FUNCTION__));

    if(stream->original_mic_level != -1){
        unsigned mic_lvl=-1;
        stream->sf->volume->GetMicVolume(mic_lvl); // retrieve input device's volume here
        PJ_LOG(4, (THIS_FILE, "Mic level before/after stream stop %u/%d",mic_lvl,stream->original_mic_level));
        stream->sf->volume->SetMicVolume(stream->original_mic_level);
    }

    if(stream->restore_spk_state)
    {
        unsigned spk_lvl = -1;
        device_mute_state spk_mute_state = UNKNOWN;
        bool spk_mute_state_bool = false;
        if(stream->sf->volume->GetSystemOutputMute(spk_mute_state_bool) == 0)
        {
            spk_mute_state = spk_mute_state_bool ? MUTED : UNMUTED;
        }
        stream->sf->volume->GetSpeakerVolume(spk_lvl); // retrieve output device's volume here

        if(stream->original_spk_mute_state != UNKNOWN &&
           spk_mute_state != stream->original_spk_mute_state)
        {
            stream->sf->volume->SetSystemOutputMute((stream->original_spk_mute_state == MUTED) ? true : false);
        }

        if(stream->original_spk_level != -1 &&
           spk_lvl != stream->original_spk_level)
        {
            stream->sf->volume->SetSpeakerVolume(stream->original_spk_level);
        }

        PJ_LOG(4, (THIS_FILE, "Restoring speaker state to initial state if known."
                   " Before/after stream stop, "
                   "speaker mute state=%s/%s; volume level=  %u/%u",
                   getMuteStateString(spk_mute_state).c_str(),
                   getMuteStateString(stream->original_spk_mute_state).c_str(),
                   spk_lvl,
                   stream->original_spk_level));
    }

    if (stream->sf->apm->EnableHighPassFilter(false) == -1){
        PJ_LOG(4, (THIS_FILE, "Failed to disable high pass filter."));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetEcStatus(false) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set echo value"));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetEsmDelayEstimatorStatus(false) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to clear Delay Estimator"));
        return PJ_EBUG;
    } 
    if (stream->sf->apm->SetEsmStatus(false) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set Echo State Machine"));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetEcMetricsStatus(false) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set echo metrics"));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetAgcStatus(false) == -1) {
        PJ_LOG(4, (THIS_FILE, "Failed to set agc"));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetNsStatus(false) == -1) 
    {
        PJ_LOG(4, (THIS_FILE, "Failed to set ns status"));
        return PJ_EBUG;
    }
    if (stream->sf->apm->SetTypingDetectionStatus(false) == -1) 
    {
        PJ_LOG(4, (THIS_FILE, "Failed to set typing status"));
    }
    if (stream->sf->apm->EnableKeyNoiseSupp(false, kKeynoiseModerate) == -1) {
         PJ_LOG(4, (THIS_FILE, "Failed to set keyboard noise suppression status"));
    }

    webrtc_voe_start_local_monitor(stream, false);
    stream->sf->base1->StopSend(stream->chan);
    stream->sf->base1->StopPlayout(stream->chan);
    return PJ_SUCCESS;
}


/* API: Destroy stream. */
static pj_status_t webrtc_voe_stream_destroy(pjmedia_aud_stream *strm)
{
    struct webrtc_voe_audio_stream *stream = (struct webrtc_voe_audio_stream*)strm;
    PJ_LOG(4, (THIS_FILE, "In function %s", __FUNCTION__));
    PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);
    stream->sf->netw->DeRegisterExternalTransport(stream->chan);
    webrtc_voe_stream_stop(strm);
    stream->sf->base1->DeleteChannel(stream->chan);
    if(stream->bjnTransport) {
        delete stream->bjnTransport;
        stream->bjnTransport = NULL;
    }

    if(stream->bjnEsmObserver) {
        delete stream->bjnEsmObserver;
        stream->bjnEsmObserver = NULL;
    }
    if(stream->bjnVadObserver) {
        delete stream->bjnVadObserver;
        stream->bjnVadObserver = NULL;
    }
    if(stream->bjnEcObserver) {
        delete stream->bjnEcObserver;
        stream->bjnEcObserver = NULL;
    }
    if(stream->bjnAudDeviceObserver) {
        delete stream->bjnAudDeviceObserver;
        stream->bjnAudDeviceObserver = NULL;
    }
    if(stream->bjnAudNotificationObserver) {
        delete stream->bjnAudNotificationObserver;
        stream->bjnAudNotificationObserver = NULL;
    }
    if(stream->bjnAudioStatsObserver) {
        delete stream->bjnAudioStatsObserver;
        stream->bjnAudioStatsObserver = NULL;
    }
    if(stream->bjnAudioMicAnimationObserver) {
        delete stream->bjnAudioMicAnimationObserver;
        stream->bjnAudioMicAnimationObserver = NULL;
    }
    pj_pool_release(stream->pool);

    return PJ_SUCCESS;
}

static void webrtc_voe_stream_rtp_cb(pjmedia_aud_stream *strm, void *usr_data,
                                      void*pkt, pj_ssize_t len)
{
    struct webrtc_voe_audio_stream *stream = (struct webrtc_voe_audio_stream*)strm;
    pjmedia_endpt_bwmgr_update(pjsua_var.med_endpt, AUDIO_MED_IDX, pkt, len, PJMEDIA_NORMAL_RTP);
    stream->sf->netw->ReceivedRTPPacket(stream->chan, pkt, len);
    //PJ_LOG(4, (THIS_FILE, "Rx rtp data: %p %p %d", usr_data, pkt, len));
}
static void webrtc_voe_stream_rtcp_cb(pjmedia_aud_stream *strm, void *usr_data,
                                       void*pkt, pj_ssize_t len)
{
    struct webrtc_voe_audio_stream *stream = (struct webrtc_voe_audio_stream*)strm;
    stream->sf->netw->ReceivedRTCPPacket(stream->chan, pkt, len);
    pj_gettimeofday(&stream->lastRXRtcp);
    stream->rtcp_rx_cnt++;
    //PJ_LOG(4, (THIS_FILE, "Rx rtcp data: %p %p %d", usr_data, pkt, len));
}
