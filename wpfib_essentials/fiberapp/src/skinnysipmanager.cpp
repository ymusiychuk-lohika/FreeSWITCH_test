#include "skinnysipmanager.h"

#include <boost/scoped_ptr.hpp>
#include <boost/thread/thread.hpp>
//#include "global/config.h"
//#include "PeerConnectionAPI.h"
//#include "openlog.h"
#include "webrtc_capture_dev.h"
#include "bjn_render_dev.h"
#include "webrtc_voe_dev.h"
#include "bjndevices.h"
#ifdef FB_MACOSX
#include "talk/base/macutils.h"
#include "Mac/macUtils.h"
#elif FB_X11
#include "talk/base/linux.h"
#include "X11/linuxUtils.h"
#elif FB_WIN
#include "Win/winUtils.h"
#include <Audiopolicy.h>
#endif

#define FBSTRING_PLUGIN_VERSION "fiberapp"
#include "networkmonitorfactory.h"

#define ESM_STATS_USE_SIP_INFO 0
#define ESM_STATS_USE_RTCP_APP 1

#define ACCESS_DENIED		5
#define REG_KEY_NOT_FOUND	2
#define REG_KEY_MIC	"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Capture\\"
#define REG_KEY_SPK	"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\MMDevices\\Audio\\Render\\"
#define REG_KEY_FXPROP	"\\FxProperties"
#define REG_VALUE	"{1da5d803-d492-4edd-8c23-e0c0ffee7f0e},5"
#define CAPTURE_AUDIO_DEVICE    0
#define RENDER_AUDIO_DEVICE     1
#define MAC_RESOURCES_DIR "Contents/Resources/"

#define WAV_FILE_NAME "speakerTest.wav"
#define WAV_FILE_DURATION 4000
#define UNIMPLEMENTED(func)	LOG(LS_WARNING) << "*** Call to unimplemented function ***" << #func
#define UNIMPLEMENTED_CASE(case_) LOG(LS_WARNING) << "*** Unimplemented switch case ***" << #case_

#define LOW_CPU_BW_MAX 900000

#define DEFAULT_SOUND_VOLUME 255
#define DEFAULT_SOUND_LEVEL 9
#define DEFAULT_SOUND_DECAY_TIMER 0
#define DEFAULT_PENDING_REICE_MSEC 2000


using namespace std;

namespace
{
#if ESM_STATS_USE_SIP_INFO
const char INFO_XML_SYSTEM_INFO_ESMSTATUS[]       = "esmstatus";
const char INFO_XML_SYSTEM_INFO_ESMXCORR[]        = "esmxcorr";
const char INFO_XML_SYSTEM_INFO_ESMECHOPRESENT[]  = "esmechopresent";
const char INFO_XML_SYSTEM_INFO_ESMECHOREMOVED[]  = "esmechoremoved";
const char INFO_XML_SYSTEM_INFO_ESMECHODETECTED[] = "esmechodetected";
const char INFO_XML_SYSTEM_INFO_ECBS[]            = "buffer_starvation";
const char INFO_XML_SYSTEM_INFO_ECBD[]            = "buffer_delay";
#endif
const char INFO_XML_AUDIO_ENGINE_STATE[]          = "audio_engine_state";
}

static int pjsua_initialized_;
boost::mutex pjsua_mutex;

namespace bjn_sky {
AudioFeatureEnablements skinnyAudioFeatures;
AudioFeatureEnablements* getAudioFeatures(void) { return &skinnyAudioFeatures; };
};

bjn_sky::skinnySipManager::skinnySipManager(talk_base::Thread* main_thread,
                                            talk_base::MessageHandler *browser_msg_hndler,
                                            talk_base::Thread *browser_thread,
                                            const SipManagerConfig& sipManagerConfig)
    : SipManager(main_thread, browser_msg_hndler, browser_thread, &bjn_sky::skinnySipManager::platformPrintLog)
    , logCPUParams(true)
    , device_manager_(cricket::DeviceManagerFactory::Create())
    , window_picker_(talk_base::WindowPickerFactory::CreateWindowPicker())
    , auto_level_check_count_(0)
    , level_determined_(0)
    , speakerPlay(false)
    , spk_vol_(DEFAULT_SOUND_VOLUME)
    , spk_mute_(false)
    , spk_lvl_(DEFAULT_SOUND_LEVEL)
    , spk_decay_timer_(DEFAULT_SOUND_DECAY_TIMER)
    , mic_mute_(false)
    , mic_vol_(DEFAULT_SOUND_VOLUME)
    , mic_lvl_(DEFAULT_SOUND_LEVEL)
    , mic_decay_timer_(DEFAULT_SOUND_DECAY_TIMER)
#ifdef WIN32
    , hr(S_OK)
    , typeDev(eRender)
    , deviceCount(0)
    , deviceIndex(0)
    , device(NULL)
    , deviceEnumerator(NULL)
    , deviceCollection(NULL)
    , pwszID(NULL)
#endif
    , mDeviceController(NULL)
    , mSipManagerConfig(sipManagerConfig)
    , mSingleStream(false)
    , mIsHuddleModeEnabled(false)
    , mNetworkMonitor(NULL)
{
    LOG(LS_INFO) << "skinnySipManager begin construction";
#ifdef WIN32
    if(0 == isXP()) // If windows not XP
    {
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if(S_OK == hr)
        {
            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&deviceEnumerator));
            assert(NULL != deviceEnumerator);
            if (deviceEnumerator) disableDucking();
        }
    }
#endif
    dual_stream_ = true;
    mReportDeviceIds = true;

#ifdef WIN32
    if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
        cricket::Win32DeviceManager* winDeviceManager = dynamic_cast< cricket::Win32DeviceManager* >(device_manager_);
        if (NULL != winDeviceManager){
            winDeviceManager->EnableAudioNotifyMechanism(true);
        }
    }
#endif

    device_manager_->Init();
    device_manager_->SignalDevicesChange.connect(this, &bjn_sky::skinnySipManager::OnDevicesChange);
#ifdef WIN32
    if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
        device_manager_->SignalVolumeChanges.connect(this, &bjn_sky::skinnySipManager::OnVolumeChange);
        bool bAbleToGetStats = GetCurrentVolStats(m_volStats);
        if (false == bAbleToGetStats){
            LOG(LS_ERROR) << "Not able to get the initial volume stats";
        }
    }
#endif
    postGetDevices();

    window_picker_->Init();
    window_picker_->SignalDesktopsChanged.connect(this, &bjn_sky::skinnySipManager::OnDesktopsChange);

    LOG(LS_INFO) << "skinnySipManager construction complete";
}

bjn_sky::skinnySipManager::~skinnySipManager() {
    if(mDeviceController)
    {
        delete mDeviceController;
        mDeviceController = NULL;
    }
#ifdef WIN32
    if(0 == isXP()) // If windows not XP
    {
        if(NULL != deviceEnumerator)
        {
            deviceEnumerator->Release();
        }
        CoUninitialize();
    }
#endif
    if(pool) pj_pool_release(pool);
    delete device_manager_;
    delete window_picker_;
    DeinitPJSip();
}

void bjn_sky::skinnySipManager::startNetworkMonitor() {
    LOG(LS_INFO) << "Check and start network monitoring if enabled";
    uint16_t flag  = 0;

    setReIceInterval();

    if(mMeetingFeatures.mWinNetworkMonitor) flag |= WIN_ENABLE_NW_MONITOR;
    if(mMeetingFeatures.mMacNetworkMonitor) flag |= MAC_ENABLE_NW_MONITOR;
    if(mMeetingFeatures.mX11NetworkMonitor) flag |= X11_ENABLE_NW_MONITOR;

    mNetworkMonitor = NetworkMonitorFactory::CreateNetworkMonitor(client_thread_,this,flag);
    if (mNetworkMonitor) {
        mNetworkMonitor->start();
    }
}

void skinnySipManager::setReIceInterval(){
#ifdef FB_WIN
    if(mMeetingFeatures.mWinNetworkMonitor) {
        mReIceInterval = AGGRESSIVE_REICE_INTERVAL;
    }
    else 
#elif  FB_MACOSX
    if(mMeetingFeatures.mMacNetworkMonitor) {
        mReIceInterval = AGGRESSIVE_REICE_INTERVAL;
    }
    else
#elif FB_X11
    if(mMeetingFeatures.mX11NetworkMonitor) {
        mReIceInterval = AGGRESSIVE_REICE_INTERVAL;
    }
    else
#endif
    {
      mReIceInterval = DEFAULT_MAX_REICE_INTERVAL;
    }
}


void bjn_sky::skinnySipManager::postHidEvents(uint16_t usage, bool state)
{
    std::pair<uint16_t, bool> deviceState;
    deviceState = std::pair<uint16_t, bool>(usage, state);
    browser_thread_->Post(browser_msg_hndler_, APP_MSG_NOTIFY_DEVICE_STATE,
        new talk_base::TypedMessageData<std::pair<uint16_t, bool> >(deviceState));
    return;
}

bool bjn_sky::skinnySipManager::IsValidDevicesInfo(const std::vector<std::string>& devicesInfo)
{
    LOG(LS_INFO) << __FUNCTION__ ;

    // Video Capture Devices
    bool isValid = false;
    size_t num_devices = 0;
    if (devicesInfo[VIDEO_DEVICE_NAME] != SCREEN_SHARING_FRIENDLY_NAME_T)
    {
        for (size_t i = 0; i < videoCaptureDevices_.size(); i++)
        {
            // Find number of video capture devices
            if (videoCaptureDevices_[i].name != SCREEN_SHARING_FRIENDLY_NAME_T)
                ++num_devices;
        }
        if (!num_devices && (devicesInfo[VIDEO_DEVICE_ID] == BJN_EMPTY_STR))
        {
            // No video capture devices present and hence empty strings are sent to plugin
            isValid = true;
        }
        else
        {
            for (size_t i = 0; i < videoCaptureDevices_.size(); i++)
            {
                if (devicesInfo[VIDEO_DEVICE_ID] == videoCaptureDevices_[i].id)
                {
                    isValid = true;
                    break;
                }
            }
        }
    }
    if (!isValid)
    {
        LOG(LS_ERROR) << "Invalid Video Capture device/id sent to plugin";
        return false;
    }

    // Audio Capture Devices
    isValid = false;
    num_devices = audioCaptureDevices_.size();
    if (!num_devices && (devicesInfo[MIC_DEVICE_ID] == BJN_EMPTY_STR))
    {
        // No audio capture devices present and hence empty strings are sent to plugin
        isValid = true;
    }
    else
    {
        for (int i = 0; i < audioCaptureDevices_.size(); i++)
        {
            if (devicesInfo[MIC_DEVICE_ID] == audioCaptureDevices_[i].id)
            {
                isValid = true;
                break;
            }
        }
    }
    if (!isValid)
    {
        LOG(LS_ERROR) << "Invalid Audio Capture device/id sent to plugin";
        return false;
    }

    // Audio Playout Devices
    isValid = false;
    num_devices = audioPlayoutDevices_.size();
    if (!num_devices && (devicesInfo[SPEAKER_DEVICE_ID] == BJN_EMPTY_STR))
    {
        // No audio playout devices present and hence empty strings are sent to plugin
        isValid = true;
    }
    else
    {
        for (int i = 0; i < audioPlayoutDevices_.size(); i++)
        {
            if (devicesInfo[SPEAKER_DEVICE_ID] == audioPlayoutDevices_[i].id)
            {
                isValid = true;
                break;
            }
        }
    }
    if (!isValid)
    {
        LOG(LS_ERROR) << "Invalid Audio Playout device/id sent to plugin";
        return false;
    }

    LOG(LS_INFO) << "Device Info list validation is success";
    return isValid;
}

void bjn_sky::skinnySipManager::DeinitPJSip()
{
    boost::lock_guard<boost::mutex> lock(pjsua_mutex);
    pjsua_initialized_--;
    if(cph_instance) {
        delete cph_instance;
        cph_instance = NULL;
    }
    if(pjsua_initialized_ == 0) {
        //need to check as where to destroy the factory
        Subscription_Factory->DeleteFactory(pjsua_get_pjsip_endpt());
        pjsua_destroy2(PJSUA_DESTROY_NO_NETWORK);
    }
    if(mNetworkMonitor) {
        mNetworkMonitor->stop();
        delete mNetworkMonitor;
        mNetworkMonitor = NULL;
    }
}

void bjn_sky::skinnySipManager::updateSipmanagerConfig(const SipManagerConfig& sipManagerConfig)
{
    mSipManagerConfig = sipManagerConfig;
}

bool bjn_sky::skinnySipManager::platformInit()
{
    boost::lock_guard<boost::mutex> lock(pjsua_mutex);
    if(pjsua_initialized_++) {
        registerMainThread();
        return false;
    }
    return true;
}

void bjn_sky::skinnySipManager::platformParseCustomConfigXML()
{
    cph_instance->parseConfigXML(const_cast<char *> (mSipManagerConfig.mCustomConfigXML.c_str()), mSipManagerConfig.mCustomConfigXML.length());
}

int bjn_sky::skinnySipManager::getMeetingInstances()
{
    boost::lock_guard<boost::mutex> lock(pjsua_mutex);
    int ret = pjsua_initialized_;
    return ret;
}

void bjn_sky::skinnySipManager::initializePlatformVideoCodecParams() {
    int encBitrate = 0;
    int decBitrate = 0;
    int configBitrateBps = mSipManagerConfig.mMaxBandwidth * 1000;
    for (unsigned i=0; i < cph_instance->vendor().size(); i++)
    {
        mCpuMonitor.addCpuParam(cph_instance->machineModel().at(i),
                                cph_instance->vendor().at(i),
                                (int)(cph_instance->family().at(i)),
                                (int)(cph_instance->model().at(i)),
                                (int)(cph_instance->stepping().at(i)),
                                (int)(cph_instance->vcores().at(i)),
                                (int)(cph_instance->frequency().at(i)),
                                (int)(cph_instance->encwidth().at(i)),
                                (int)(cph_instance->encheight().at(i)),
                                (int)(cph_instance->encfps().at(i)),
                                (int)(cph_instance->decwidth().at(i)),
                                (int)(cph_instance->decheight().at(i)),
                                (int)(cph_instance->decfps().at(i)),
                                (bool)(cph_instance->dualstream().at(i)));
    }

    mCpuMonitor.setMaxFreqBasedResCapFlag(mMeetingFeatures.mMaxFreqBasedResCap);
    if (mMeetingFeatures.mGpuLogWin){
        mCpuMonitor.InitGPUInfo();
    }
    if(mMeetingFeatures.mH264CapPub){
        mCpuMonitor.setH264CapableFlag();
    }
#ifdef WIN32
    if (mMeetingFeatures.mWinBaseBoardManufacturerModel) {
        mCpuMonitor.searchWithBaseBoardData();
    }
#endif
    CPU_Monitor::CPU_Params cpuParams   = mCpuMonitor.getCpuParams();
    if(logCPUParams)
    {
        mCpuMonitor.logCPUParams(cpuParams);
        logCPUParams = false;
    }

    if(mMeetingFeatures.mLogFanSpeed)
    {
        mCpuMonitor.InitSMCSampler();
    }

    video_codec_params_.enc.width       = cpuParams.vparams.encWidth;
    video_codec_params_.enc.height      = cpuParams.vparams.encHeight;
    video_codec_params_.enc.fps         = cpuParams.vparams.encFps;

    if(cpuParams.vparams.encHeight > 480)
    {
        encBitrate = configBitrateBps > 0 ? std::min(configBitrateBps, MAX_VID_BITRATE_BPS): DEFAULT_VID_BITRATE_BPS;
    }
    else
    {
        encBitrate = configBitrateBps > 0 ? std::min(configBitrateBps, LOW_CPU_BW_MAX): LOW_CPU_BW_MAX;
    }

    video_codec_params_.enc.bitrate = encBitrate;

    decBitrate = configBitrateBps > 0 ? std::min(configBitrateBps, MAX_VID_BITRATE_BPS): DEFAULT_VID_BITRATE_BPS;

    video_codec_params_.dec.width       = cpuParams.vparams.decWidth;
    video_codec_params_.dec.height      = cpuParams.vparams.decHeight;
    video_codec_params_.dec.fps         = cpuParams.vparams.decFps;
    video_codec_params_.dec.bitrate     = decBitrate;

    LOG(LS_INFO)<<" Encode bitrate set to: "<<video_codec_params_.enc.bitrate<<" and Decode bitrate set to: "<<video_codec_params_.dec.bitrate;
}

void bjn_sky::skinnySipManager::getPlatformLogfilePathWithGuid(std::string& path)
{
    path = BJN::getBjnLogfilePath(true);
}

void bjn_sky::skinnySipManager::getConfigPath(std::string& path)
{
     path = BJN::getBjnConfigFile();
}

std::string bjn_sky::skinnySipManager::getPcapPath()
{
     return BJN::getBjnPcapPath();
}

void bjn_sky::skinnySipManager::getUserAgent(std::string& useragent){
    useragent_.assign("BlueJeans-Browser/");
    useragent_.append(FBSTRING_PLUGIN_VERSION);
#ifdef FB_WIN
    useragent_.append("/Windows ");
#elif  FB_MACOSX
    useragent_.append("/Mac ");
#elif FB_X11
    useragent_.append("/Linux ");
#else
    useragent_.append("/Unknown OS ");
#endif
    useragent = useragent_;
}

void bjn_sky::skinnySipManager::setConfigPath(std::string configPath){
    config_path_ = configPath;
}

void bjn_sky::skinnySipManager::setUserAgent(std::string useragent){
}

void bjn_sky::skinnySipManager::DeviceCpy(std::vector<Device> &Loc_Dev, std::vector<cricket::Device> &cric_Dev)
{
    Device tmp;
    for (std::vector<cricket::Device>::iterator it = cric_Dev.begin() ; it != cric_Dev.end(); ++it) {
        tmp.name = it->name;
        tmp.id= it->id;
        tmp.builtIn = it->builtIn;
        Loc_Dev.push_back(tmp);
    }
}

#ifdef WIN32
//This function will fetch current volume stats
bool bjn_sky::skinnySipManager::GetCurrentVolStats(cricket::VolumeStats &volStats){
    cricket::Win32DeviceManager* winDeviceManager = dynamic_cast< cricket::Win32DeviceManager* >( device_manager_ );
    if (NULL != winDeviceManager){
        return  winDeviceManager->GetCurrentVolStats(volStats);
    }
    LOG(LS_ERROR) << "Not able to get the Win32DeviceManager pointer";
    return false;
}
#endif
void bjn_sky::skinnySipManager::platformGetVolumeMuteState(AudioDspMetrics& metrics){

#ifdef WIN32
    //In case of windows xp default values will be sent
    //In case "win_core_audio_devnotify" is disabled,
    //then also default values would be sent with all
    //other DSP stats.
    //Default value for volume is -1 and for mute state 0
    metrics.mSpkrVolume = m_volStats.m_systemVolume;
    metrics.mSpkrMuteState = m_volStats.m_systemMuteState;
    metrics.mMicVolume = m_volStats.m_micVolume;
    metrics.mMicMuteState = m_volStats.m_micMuteState;
#else
    //Already updated in webrtc_voe_dev.cc for Mac and Linux
    return;
#endif

}

void bjn_sky::skinnySipManager::GetPlatformDevices() {
    std::vector<cricket::Device> audioCaptureDevices;
    std::vector<cricket::Device> audioPlayoutDevices;
    std::vector<cricket::Device> videoCaptureDevices;
    device_manager_->GetAudioInputDevices(&audioCaptureDevices);
    LOG(LS_INFO) << "Audio capture Devices: ";
    DeviceCpy(audioCaptureDevices_,audioCaptureDevices);
    PrintDevices(audioCaptureDevices_);
    device_manager_->GetAudioOutputDevices(&audioPlayoutDevices);
    LOG(LS_INFO) << "Audio playout Devices: ";
    DeviceCpy(audioPlayoutDevices_,audioPlayoutDevices);
    PrintDevices(audioPlayoutDevices_);
    device_manager_->GetVideoCaptureDevices(&videoCaptureDevices);
    LOG(LS_INFO) << "Video capture Devices: ";
    DeviceCpy(videoCaptureDevices_,videoCaptureDevices);
    PrintDevices(videoCaptureDevices_);

    LocalDevices msg_param = {audioCaptureDevices, audioPlayoutDevices, videoCaptureDevices};
    browser_thread_->Post(browser_msg_hndler_, APP_MSG_NOTIFY_LOCAL_DEVICES,
                        new talk_base::TypedMessageData<LocalDevices>(msg_param));

    if(mMeetingFeatures.mDepreferUsbOnThunderbolt)
    {
        updateUsbOnThunderboltMap(audioCaptureDevices_, true, true);
        updateUsbOnThunderboltMap(audioPlayoutDevices_, true, false);
    }
}

void* bjn_sky::skinnySipManager::getLocalWindow(unsigned videoCapId ){
    return NULL;
}

void* bjn_sky::skinnySipManager::getRemoteWindow(){
    return NULL;
}

void bjn_sky::skinnySipManager::enterBackground(){
    return;
}

bool bjn_sky::skinnySipManager::isScreenSharingDevice(const std::string& screenId){
    return BJN::DevicePropertyManager::isScreenSharingDevice(screenId);
}

void bjn_sky::skinnySipManager::initializePlatformAudioCodecs()
{
    LOG(LS_INFO) << "In function " << __FUNCTION__;
    pjmedia_aud_dev_factory *factory =
        pjmedia_webrtc_voe_audio_factory(pjmedia_aud_subsys_get_pool_factory(),
                                         this);
    pjmedia_aud_register_factory(NULL,factory);
}

void bjn_sky::skinnySipManager::initializePlatformVideoCodecs()
{
    LOG(LS_INFO) << "In function " << __FUNCTION__;
}

void bjn_sky::skinnySipManager::initializePlatformVideoDevices() {
    LOG(LS_INFO) << "In function " << __FUNCTION__;

    pjmedia_vid_register_factory(NULL,
                                 pjmedia_bjn_vid_render_factory(
                                 pjmedia_vid_dev_subsys_get_pool_factory(), this));
    pjmedia_vid_register_factory(NULL,
                                 pjmedia_webrtc_vid_capture_factory(
                                 pjmedia_vid_dev_subsys_get_pool_factory(),this));
}

void bjn_sky::skinnySipManager::platformPrintLog(int level, const char *data, int len){
    //TODO: Map PJSIP log level to xmpp logging level
    std::string str(data, len);
    LOG(LS_INFO) << str;
}

void bjn_sky::skinnySipManager::platformMakeCall(pj_str_t* uri,
                                                 pjsua_call_setting* callSettings,
                                                 bool encryptionSupport,
                                                 std::string proxyInfo)
{
    pjsua_msg_data msg_data;
    pj_str_t hname, hvalue;
    int major, minor, build;
    std::string uiInfo;
    std::stringstream buffer;
    std::string new_uri;
    std::string cpuParams;
    pj_str_t call_uri;
    pj_str_t transport_str, proxy_str;

    uiInfo.assign("BlueJeans-Browser/");
    uiInfo.append(FBSTRING_PLUGIN_VERSION);
#ifdef FB_WIN
    uiInfo.append("/Windows ");
#elif  FB_MACOSX
    uiInfo.append("/Mac ");
#elif FB_X11
    uiInfo.append("/Linux ");
#else
    uiInfo.append("/Unknown OS ");
#endif

#ifdef FB_X11
    buffer << talk_base::ReadLinuxLsbRelease() << "/";
#else
    talk_base::GetOsVersion(&major, &minor, &build);
    buffer << major << "." << minor << "." << build << "/";
#endif
    uiInfo.append(buffer.str());
    uiInfo.append(uiInfo_);

    pjsua_msg_data_init(&msg_data);
    pjsip_generic_string_hdr hmsgUA, hmsgCP, hmsgPI, hmsgPT;

    pj_cstr(&hname, "X-User-Agent");
    pj_cstr(&hvalue, uiInfo.c_str());
    pjsip_generic_string_hdr_init2(&hmsgUA, &hname, &hvalue);
    pj_list_push_back(&msg_data.hdr_list, &hmsgUA);

    pj_cstr(&hname, "X-Cpu-Params");
    mCpuMonitor.curCPUParams(cpuParams);
    pj_cstr(&hvalue, cpuParams.c_str());
    pjsip_generic_string_hdr_init2(&hmsgCP, &hname, &hvalue);
    pj_list_push_back(&msg_data.hdr_list, &hmsgCP);

    if(mSingleStream) {
        dual_stream_ = mCpuMonitor.getCpuParams().vparams.dualStream;
    }

    if(!proxyInfo.empty() && proxyInfo.find_first_not_of(' ') != std::string::npos)
    {
        pj_cstr(&hname, "X-Proxy-Info");
        proxyInfo.erase(proxyInfo.end()-1, proxyInfo.end());
        pj_cstr(&hvalue, proxyInfo.c_str());
        pjsip_generic_string_hdr_init2(&hmsgPI, &hname, &hvalue);
        pj_list_push_back(&msg_data.hdr_list, &hmsgPI);

        pj_cstr(&hname, "X-Proxy-Transport");
        encryptionSupport ? pj_cstr(&hvalue, "true ") : pj_cstr(&hvalue, "false");
        pjsip_generic_string_hdr_init2(&hmsgPT, &hname, &hvalue);
        pj_list_push_back(&msg_data.hdr_list, &hmsgPT);
    }

    pj_cstr(&transport_str, ";transport");
    char *p = pj_stristr(uri, &transport_str);
    if(p) {
        if(encryptionSupport) {
            new_uri = "sips:";
            new_uri.append((uri->ptr + 4), p - (uri->ptr + 4));
            new_uri += ";transport=multiport";
        } else {
            new_uri = "sip:";
            char *port_start = (char *) memchr((uri->ptr + 4), ':', uri->slen - 4);
            char *port_end = (char *) memchr((uri->ptr + 4), ';', uri->slen - 4);
            if(port_start) {
                int port_len = port_end - port_start - 1;
                new_uri.append((uri->ptr + 4), p - (uri->ptr + 4) - port_len);
                new_uri += "5060";
            } else {
                new_uri.append((uri->ptr + 4), p - (uri->ptr + 4));
            }
            new_uri += ";transport=tcp";
        }

        if (mMeetingFeatures.mNat64Ipv6 && mMeetingFeatures.mPreferIpv6)
            new_uri += ";ipv6";

        pj_cstr(&proxy_str, ";httpproxy=");
        if ((p = pj_stristr(uri, &proxy_str)) != NULL)
        {
            new_uri.append(p, (pj_strlen(uri) - (pj_size_t)(p - pj_strbuf(uri))));
        }
        LOG(LS_INFO) << "New uri for call: " << new_uri;
        call_uri = pj_strdup3(pool, new_uri.c_str());
    } else {
        pj_strassign(&call_uri, uri);
    }

    if(!dual_stream_) {
        callSettings->vid_cnt = 1;
        browser_thread_->Post(browser_msg_hndler_, APP_MSG_NOTIFY_WARNING,
                              new talk_base::TypedMessageData<bjn_sky::errorCode>(ERROR_WARN_DUAL_STREAM_DISABLED));
    }

    pjsua_call_make_call(0, &call_uri, callSettings, this, &msg_data, &call_id_);
    return;
}

int bjn_sky::skinnySipManager::platformGetVideoBitrateForCpuLoad() {
    if(logCPUParams)
    {
        mCpuMonitor.logCPUParams(mCpuMonitor.getCpuParams());
        logCPUParams = false;
    }

    // Check CPU load to take action on video quality
    int strength = mCpuMonitor.getAdjustmentStrength();
    int cpuBandwidth = bw_mgr_.getMaxBitrateForCpuLoad() - strength * 50000;
    return cpuBandwidth;
}

/**
 * This method return the process load of the application on the OS
 */
 int bjn_sky::skinnySipManager::platformGetProcessLoad(){

    /**
     * Refreshing the cpu load here as this method is called once in 1000ms
     */
     pj_time_val currentTime;
     pj_gettimeofday(&currentTime);
     mCpuMonitor.refreshCpuLoad(currentTime);

     return mCpuMonitor.getProcessLoad();
}

int bjn_sky::skinnySipManager::platformGetSystemCpuLoad(){
    return mCpuMonitor.getSystemLoad();
}


int bjn_sky::skinnySipManager::platformGetCurrentCpuFreqUsage()
{
#if defined(WIN32) || defined(LINUX)
   return mCpuMonitor.getCurCPUFreqUsage();
#else
   return 0;
#endif
}

float bjn_sky::skinnySipManager::platformGetFanSpeed()
{
    return mCpuMonitor.getFanSpeed();
}

void bjn_sky::skinnySipManager::sendEsmStatus(const pjmedia_event_esm_notify_data &esm)
{
    if(call_id_ == PJSUA_INVALID_ID)
        return;

#if ESM_STATS_USE_RTCP_APP
    StatElement stat(SI_ESM_STATUS, (int)esm.esm_state);
    pjsua_update_bjn_stat(call_id_, &stat);
    stat = StatElement(SI_ESM_XCORR_METRIC, esm.esm_xcorr_metric);
    pjsua_update_bjn_stat(call_id_, &stat);
    stat = StatElement(SI_ESM_ECHO_PRESENT_METRIC, esm.esm_echo_present_metric);
    pjsua_update_bjn_stat(call_id_, &stat);
    stat = StatElement(SI_ESM_ECHO_REMOVED_METRIC, esm.esm_echo_removed_metric);
    pjsua_update_bjn_stat(call_id_, &stat);
    stat = StatElement(SI_ESM_ECHO_DETECTED_METRIC, esm.esm_echo_detected_metric);
    pjsua_update_bjn_stat(call_id_, &stat);
#endif

#if ESM_STATS_USE_SIP_INFO
    std::stringstream buffer;
    buffer << INFO_XML_SYSTEM_INFO_ESMSTATUS << " value='" << esm.esm_state << "'/>\r\n";
    buffer << "<" << INFO_XML_SYSTEM_INFO_ESMXCORR << " value='" << esm.esm_xcorr_metric << "'/>\r\n";
    buffer << "<" << INFO_XML_SYSTEM_INFO_ESMECHOPRESENT << " value='" << esm.esm_echo_present_metric << "'/>\r\n";
    buffer << "<" << INFO_XML_SYSTEM_INFO_ESMECHOREMOVED << " value='" << esm.esm_echo_removed_metric << "'/>\r\n";
    buffer << "<" << INFO_XML_SYSTEM_INFO_ESMECHODETECTED << " value='" << esm.esm_echo_detected_metric << "'";
    sendSystemInfoMessage(call_id_, buffer.str());
#endif
}

void bjn_sky::skinnySipManager::sendEchoCancellerBufferStarvation(const pjmedia_event_ecbs_data &ecbs)
{
    if (call_id_ == PJSUA_INVALID_ID)
        return;

#if ESM_STATS_USE_RTCP_APP
    StatElement stat(SI_ESM_ECHO_BUFFER_STARVATION, ecbs.buffered_data);
    pjsua_update_bjn_stat(call_id_, &stat);
    stat = StatElement(SI_ESM_ECHO_BUFFER_STARVATION_DELAY, ecbs.delay_ms);
    pjsua_update_bjn_stat(call_id_, &stat);
    if (ecbs.buffered_data < ECBS_BUFFER_STARVATION_THRESHOLD)
    {
        // Only post this when the buffer is low -- oneshot events will
        // expire out automatically.
        stat = StatElement(SI_ESM_ECHO_BUFER_STARVATION_EVENT, 1);
        pjsua_send_bjn_stat_oneshot(call_id_, &stat);
    }
#endif

#if ESM_STATS_USE_SIP_INFO
    std::stringstream buffer;
    buffer << INFO_XML_SYSTEM_INFO_ECBS << " value='" << ecbs.buffered_data << "'/>\r\n";
    buffer << "<" << INFO_XML_SYSTEM_INFO_ECBD << " value='" << ecbs.delay_ms << "'";
    sendSystemInfoMessage(call_id_, buffer.str());
#endif
}

pj_status_t bjn_sky::skinnySipManager::platformGetErrorStatus(){
    pj_status_t     internalStatus = 0;
    int status = 0;
    pj_status_t errorStatus = PJSIP_IS_STATUS_IN_CLASS(status,200)?internalStatus:status;
    return errorStatus;
}

void bjn_sky::skinnySipManager::initializePlatformAccount(){
    LOG(LS_INFO) << "In function " << __FUNCTION__;
    FindVideoDeviceIdx("", "RenderLocalView",
                       &rend_local_idx_, PJMEDIA_DIR_RENDER);

    FindVideoDeviceIdx("", "RenderRemoteView",
                       &rend_idx_, PJMEDIA_DIR_RENDER);

    LOG(LS_INFO) << "In function " << __FUNCTION__ << " complete";
    return;
}

static bool tryGetAddrInfo()
{
    const char *node = "bjn.vc", *service = "443";
    bool useGetAddrInfo = true;
    struct addrinfo *res;

    if (getaddrinfo( node, service, NULL, &res) == 0) {
        useGetAddrInfo = false;
        freeaddrinfo(res);
    }
    LOG(LS_INFO) << "Using " << ((useGetAddrInfo)?"GetAddrInfo":"PJSIP resolver")
                 << " for name resolution";
    return useGetAddrInfo;
}

void bjn_sky::skinnySipManager::getPlatformNameServer(pjsua_config *cfg)
{
    if (tryGetAddrInfo() == false) {
    int ret = BJN::getNameServer(name_server_);
    if(ret == PJ_SUCCESS) {
        for(std::vector<std::string>::iterator it = name_server_.begin();
                ((it != name_server_.end()) && (cfg->nameserver_count < 4));
                ++it)
        {
        LOG(LS_INFO) << "Nameserver is :" << *it;
        cfg->nameserver[cfg->nameserver_count++] = pj_str((char *)it->c_str());
        }
    } else {
        LOG(LS_INFO) << "Nameserver not found. getAddressinfo will be used.";
    }
    }
    return;
}

void bjn_sky::skinnySipManager::platformGetContentDevice(pjmedia_vid_dev_index &content_idx, pjmedia_vid_dev_index &rend_idx,
    const std::string& screenName, const std::string& screenId)
{
    FindVideoDeviceIdx(screenId.c_str(),
                       screenName.c_str(),
                       &content_idx,
                       PJMEDIA_DIR_CAPTURE);
    FindVideoDeviceIdx("", "RenderContentView",
                       &rend_idx, PJMEDIA_DIR_RENDER);
}

bool bjn_sky::skinnySipManager::isDeviceSame(Device savedDevice, Device newDevice)
{
    return(savedDevice.id == newDevice.id);
}

bool bjn_sky::skinnySipManager::findAndSetAddedDevice()
{
    std::vector<cricket::Device> audioCaptureDevices;
    std::vector<cricket::Device> audioPlayoutDevices;
    std::vector<cricket::Device> videoCaptureDevices;

    std::vector<Device>  taudioCaptureDevices;
    std::vector<Device>  taudioPlayoutDevices;
    std::vector<Device>  tvideoCaptureDevices;

    device_manager_->GetAudioInputDevices(&audioCaptureDevices);
    LOG(LS_INFO) << "Audio capture Devices: ";
    DeviceCpy(taudioCaptureDevices,audioCaptureDevices); 
    PrintDevices(taudioCaptureDevices);

    device_manager_->GetAudioOutputDevices(&audioPlayoutDevices);
    LOG(LS_INFO) << "Audio playout Devices: ";
    DeviceCpy(taudioPlayoutDevices,audioPlayoutDevices);
    PrintDevices(taudioPlayoutDevices);

    device_manager_->GetVideoCaptureDevices(&videoCaptureDevices);
    LOG(LS_INFO) << "Video capture Devices: ";
    DeviceCpy(tvideoCaptureDevices,videoCaptureDevices);
    PrintDevices(tvideoCaptureDevices);


    std::vector<Device>::iterator itNewList;
    std::vector<Device>::iterator itSavedList;

    bool gotAddedDevice=false;
    //for capture device
    for (itNewList = taudioCaptureDevices.begin(); itNewList != taudioCaptureDevices.end(); ++itNewList)
    {
        bool found=false;
        for (itSavedList = audioCaptureDevices_.begin(); itSavedList != audioCaptureDevices_.end(); ++itSavedList)
        {
            if(true == isDeviceSame(*itSavedList, *itNewList))
            {
                found=true;
                break;
            }
        }
        if(!found)
        {
            dev_.devAction=ADDED;
            dev_.devType = AUDIO_IN;
            dev_.devGuid = itNewList->id;
            dev_.devName = itNewList->name;
            audioCaptureDevices_.push_back(*itNewList);
            gotAddedDevice=true;
            break;
        }
    }
    if(false == gotAddedDevice)
    {
        //for playout device
        for (itNewList = taudioPlayoutDevices.begin(); itNewList != taudioPlayoutDevices.end(); ++itNewList)
        {
            bool found=false;
            for (itSavedList = audioPlayoutDevices_.begin(); itSavedList != audioPlayoutDevices_.end(); ++itSavedList)
            {
                if(isDeviceSame(*itSavedList, *itNewList))
                {
                    found=true;
                    break;
                }
            }
            if(!found)
            {
                dev_.devAction=ADDED;
                dev_.devType = AUDIO_OUT;
                dev_.devGuid = itNewList->id;
                dev_.devName = itNewList->name;
                audioPlayoutDevices_.push_back(*itNewList);
                gotAddedDevice=true;
                break;
            }
        }
    }

    if(false == gotAddedDevice)
    {
        //for video device
        for (itNewList = tvideoCaptureDevices.begin(); itNewList != tvideoCaptureDevices.end(); ++itNewList)
        {
            bool found=false;
            for (itSavedList = videoCaptureDevices_.begin(); itSavedList != videoCaptureDevices_.end(); ++itSavedList)
            {
                if(isDeviceSame(*itSavedList, *itNewList))
                {
                    found=true;
                    break;
                }
            }
            if(!found)
            {
                dev_.devAction=ADDED;
                dev_.devType = VIDEO;
                dev_.devGuid = itNewList->id;
                dev_.devName = itNewList->name;
                videoCaptureDevices_.push_back(*itNewList);
                gotAddedDevice=true;
                break;
            }
        }
    }
   
    return  gotAddedDevice;
}

bool bjn_sky::skinnySipManager::findAndSetRemovedDevice()
{
    std::vector<cricket::Device> audioCaptureDevices;
    std::vector<cricket::Device> audioPlayoutDevices;
    std::vector<cricket::Device> videoCaptureDevices;
    std::vector<Device>  taudioCaptureDevices;
    std::vector<Device>  taudioPlayoutDevices;
    std::vector<Device>  tvideoCaptureDevices;

    device_manager_->GetAudioInputDevices(&audioCaptureDevices);
    LOG(LS_INFO) << "Audio capture Devices: ";
    DeviceCpy(taudioCaptureDevices,audioCaptureDevices); 
    PrintDevices(taudioCaptureDevices);

    device_manager_->GetAudioOutputDevices(&audioPlayoutDevices);
    LOG(LS_INFO) << "Audio playout Devices: ";
    DeviceCpy(taudioPlayoutDevices,audioPlayoutDevices);
    PrintDevices(taudioPlayoutDevices);

    device_manager_->GetVideoCaptureDevices(&videoCaptureDevices);
    LOG(LS_INFO) << "Video capture Devices: ";
    DeviceCpy(tvideoCaptureDevices,videoCaptureDevices);
    PrintDevices(tvideoCaptureDevices);

    std::vector<Device>::iterator itNewList;
    std::vector<Device>::iterator itSavedList;

    bool gotRemovedDevice=false;
    //for capture device
    for (itSavedList = audioCaptureDevices_.begin(); itSavedList != audioCaptureDevices_.end(); ++itSavedList)
    {
        bool found=false;
        for (itNewList = taudioCaptureDevices.begin(); itNewList != taudioCaptureDevices.end(); ++itNewList)
        {
            if(true == isDeviceSame(*itSavedList, *itNewList))
            {
                found=true;
                break;
            }
        }
        if(!found)
        {
            dev_.devAction=REMOVED;
            dev_.devType = AUDIO_IN;
            dev_.devGuid = itSavedList->id;
            dev_.devName = itSavedList->name;
            audioCaptureDevices_.erase(itSavedList);
            gotRemovedDevice=true;
            break;
        }
    }
    if(false == gotRemovedDevice)
    {
        //for playout device
        for (itSavedList = audioPlayoutDevices_.begin(); itSavedList != audioPlayoutDevices_.end(); ++itSavedList)
        {
            bool found=false;
            for (itNewList = taudioPlayoutDevices.begin(); itNewList != taudioPlayoutDevices.end(); ++itNewList)
            {
                if(true == isDeviceSame(*itSavedList, *itNewList))
                {
                    found=true;
                    break;
                }
            }
            if(!found)
            {
                dev_.devAction=REMOVED;
                dev_.devType = AUDIO_OUT;
                dev_.devGuid = itSavedList->id;
                dev_.devName = itSavedList->name;
                audioPlayoutDevices_.erase(itSavedList);
                gotRemovedDevice=true;
                break;
            }
        }
    }

    if(false == gotRemovedDevice)
    {
        //for playout device
        for (itSavedList = videoCaptureDevices_.begin(); itSavedList != videoCaptureDevices_.end();++itSavedList)
        {
            bool found=false;
            for (itNewList = tvideoCaptureDevices.begin(); itNewList != tvideoCaptureDevices.end(); ++itNewList)
            {
                if(true == isDeviceSame(*itSavedList, *itNewList))
                {
                    found=true;
                    break;
                }
            }
            if(!found)
            {
                dev_.devAction=REMOVED;
                dev_.devType = VIDEO;
                dev_.devGuid = itSavedList->id;
                dev_.devName = itSavedList->name;
                videoCaptureDevices_.erase(itSavedList);
                gotRemovedDevice = true;
                break;
            }
        }
    }
    return gotRemovedDevice;  
}

void bjn_sky::skinnySipManager::postSetAudioOptions(int options)
{
  if(client_thread_) {
      client_thread_->Post(this, MSG_SET_AUDIO_OPTIONS, new talk_base::TypedMessageData<int>(options));
  }
}

void bjn_sky::skinnySipManager::postSetAudioConfigOptions(AudioConfigOptions& options)
{
    if(client_thread_) {
         client_thread_->Post(this, MSG_SET_AUDIO_CONFIG_OPTIONS,
                             new talk_base::TypedMessageData<AudioConfigOptions>(options));
    }
}

void bjn_sky::skinnySipManager::postStartSpeakerVolume()
{
    return;
}

void bjn_sky::skinnySipManager::postStopSpeakerVolume()
{
    spk_decay_timer_ = 0;
}

void bjn_sky::skinnySipManager::postGetSpeakerVolLvl(int *level)
{
    if(client_thread_ && !spk_decay_timer_) {
        spk_decay_timer_ = DECAY_POLL_INTERVAL;
        client_thread_->Post(this, MSG_START_SPK_MONITOR);
    }
    *level = spk_lvl_;
    return;
}

void bjn_sky::skinnySipManager::postStartMicVolume()
{
}

void bjn_sky::skinnySipManager::postStopMicVolume()
{
    mic_decay_timer_ = 0;
}

void bjn_sky::skinnySipManager::postGetMicVolLvl(int *level)
{
    if(client_thread_ && !mic_decay_timer_) {
        mic_decay_timer_ = DECAY_POLL_INTERVAL;
        client_thread_->Post(this, MSG_START_MIC_MONITOR);
    }
    *level = mic_lvl_;
    return;
}

void bjn_sky::skinnySipManager::postGetSpeakerMute(bool* s_mstatus)
{
#ifdef WIN32
    if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
        //No need to post message to monitor speaker
        //Changes in speaker volume will be notified
        //via OnVolumeChange callback
        if (m_volStats.m_systemMuteState || m_volStats.m_sessionMuteState
            || (m_volStats.m_sessionVolume == 0.0) || (m_volStats.m_systemVolume == 0.0)){
            *s_mstatus = true;
        }
        else{
            *s_mstatus = false;
        }
        spk_mute_ = *s_mstatus;
    }
    else {
#endif
        if (client_thread_ && !spk_decay_timer_) {
            spk_decay_timer_ = DECAY_POLL_INTERVAL;
            client_thread_->Post(this, MSG_START_SPK_MONITOR);
        }
        *s_mstatus = spk_mute_;
#ifdef WIN32
    }
#endif
}

void bjn_sky::skinnySipManager::postCheckSpeaker(LocalMediaInfo &selectedDevices)
{
    if(client_thread_) {
        client_thread_->Post(this, MSG_CHECK_SPEAKER,
                             new talk_base::TypedMessageData<LocalMediaInfo &>(selectedDevices));
    }
}

void bjn_sky::skinnySipManager::disableDeviceEnhancements(std::string micId, std::string micName, int devType)
{
#ifdef FB_WIN
    DWORD   value, Type = REG_DWORD;
    DWORD   buffersize = sizeof(value);
    string  regSubkey, guid;
    long    retVal;
    HKEY    hKey;
    bool    flag = 0;
    int     pos = micId.find( ".{", 0);

    guid.assign(micId, pos+1, 50);
    if ( devType == CAPTURE_AUDIO_DEVICE )
        regSubkey.assign( REG_KEY_MIC );
    else if( devType == RENDER_AUDIO_DEVICE )
        regSubkey.assign( REG_KEY_SPK );
    regSubkey.append( &guid[0] );
    regSubkey.append( REG_KEY_FXPROP );

    LOG(LS_INFO) << "Setting Enhancements for reg key " << regSubkey;

    retVal = RegOpenKeyExA (HKEY_LOCAL_MACHINE, (LPCSTR)regSubkey.c_str(), NULL,
        KEY_QUERY_VALUE|KEY_WOW64_64KEY|KEY_SET_VALUE, &hKey);
    if( retVal == ERROR_SUCCESS )
    {
        value = 1;
        retVal = RegSetValueExA(hKey,(LPCSTR) REG_VALUE, NULL, Type,(const BYTE *)&value, buffersize);
        if( retVal == ERROR_SUCCESS )
            LOG(LS_INFO) << "Enhancements set successfully for " << micName;
        RegCloseKey(hKey);
    }

    // no enhancements available for selected device
    else if( retVal == REG_KEY_NOT_FOUND )
        LOG(LS_INFO) << "No enhancements available for " << micName;

    // non-admin user
    else if( retVal == ACCESS_DENIED )
    {
        LOG(LS_INFO) << "Access denied for setting enhancements";
        if( RegOpenKeyExA (HKEY_LOCAL_MACHINE, (LPCSTR)regSubkey.c_str(), NULL,
            KEY_QUERY_VALUE|KEY_WOW64_64KEY, &hKey) == ERROR_SUCCESS )
        {
            retVal = RegQueryValueExA(hKey,(LPCSTR) REG_VALUE, NULL, &Type, (LPBYTE)&value, &buffersize);
            if( retVal == ERROR_SUCCESS ) {
                flag = !value;
            }
            RegCloseKey(hKey);
        }
        else
            flag = 1;
    }

    if( flag )
    {
        LOG(LS_INFO) << "Enhancements are not disabled for " << micName;
        if( devType == CAPTURE_AUDIO_DEVICE )
            browser_thread_->Post(browser_msg_hndler_, APP_MSG_DEVICE_ENHANCEMENTS,
                                  new talk_base::TypedMessageData<int>(ERROR_MIC_ENHANCEMENTS_SET_FAILED));
        else if( devType == RENDER_AUDIO_DEVICE )
            browser_thread_->Post(browser_msg_hndler_, APP_MSG_DEVICE_ENHANCEMENTS,
                                  new talk_base::TypedMessageData<int>(ERROR_SPK_ENHANCEMENTS_SET_FAILED));
    } else {
        LOG(LS_INFO) << "Enhancements are already disabled for " << micName;
    }
#endif
}

void bjn_sky::skinnySipManager::SetLocalDevicesOptions(LocalMediaInfo &selectedDeviceoptions)
{
#ifdef FB_WIN
    if(talk_base::IsWindowsVistaOrLater()) 
    {
        if( selectedDeviceoptions.get_Microphone() != BJN_EMPTY_STR )
            disableDeviceEnhancements(selectedDeviceoptions.get_MicrophoneID(),
                                      selectedDeviceoptions.get_Microphone(), CAPTURE_AUDIO_DEVICE);
        if( selectedDeviceoptions.get_Speaker() != BJN_EMPTY_STR )
            disableDeviceEnhancements(selectedDeviceoptions.get_SpeakerID(),
                                      selectedDeviceoptions.get_Speaker(), RENDER_AUDIO_DEVICE);
    }
#endif

    if( selectedDeviceoptions_.get_SpeakerID() != selectedDeviceoptions.get_SpeakerID() )
    {
        if( speakerPlay )
        {
            client_thread_->Clear(this, MSG_STOP_TEST_SND);
            stopTestSound();
            checkSpeaker(selectedDeviceoptions);
        }
        resetSavedSpeakerState();
    }

    if(selectedDeviceoptions_.get_MicrophoneID() != selectedDeviceoptions.get_MicrophoneID())
    {
        resetSavedMicrophoneState();
    }
}

void bjn_sky::skinnySipManager::getSpeakerMute()
{
    bool mStatus;
    pj_status_t status;
#ifdef FB_WIN
     status = pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_OUTPUT_MUTE_SETTING, &mStatus);

    if(0 == bjn_sky::isXP()) // If windows not XP
    {
        bool devMuteStatus;
        std::string speakerGUID= getSelSpeakerGUID();
        status =  winGetSpeakerMute(speakerGUID, &devMuteStatus);
        if((true == devMuteStatus) || (true == mStatus))
        {
            mStatus = true;
        }
        else
        {
            mStatus = false;
        }
    }
#else
    status = pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_SPEAKER_MUTE, &mStatus);
#endif
    if (status == PJ_SUCCESS)
    {
        spk_mute_ = mStatus;
    }
    else
    {
        LOG(LS_INFO) << " Failed postGetSpeakerMute";
    }
}

void bjn_sky::skinnySipManager::postGetMicrophoneMute(bool* m_mstatus)
{
#ifdef WIN32
    //No need to post message to monitor mic
    //Changes in mic volume will be notified
    //via OnVolumeChange callback
    if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
        *m_mstatus = m_volStats.m_micMuteState;
        mic_mute_ = *m_mstatus;
    }
    else{
#endif
        if (client_thread_ && !mic_decay_timer_) {
            mic_decay_timer_ = DECAY_POLL_INTERVAL;
            client_thread_->Post(this, MSG_START_MIC_MONITOR);
        }
        *m_mstatus = mic_mute_;
#ifdef WIN32
    }
#endif
}

void bjn_sky::skinnySipManager::getMicrophoneMute()
{
    pj_status_t status;
    status = pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_MICROPHONE_MUTE, &mic_mute_);
    if (status != PJ_SUCCESS)
    {
        LOG(LS_INFO) << " Failed postGetSpeakerMute->pjsua_snd_get_setting";
    }
}

void bjn_sky::skinnySipManager::getSpeakerSettings()
{
    if(spk_decay_timer_ != 0) {
        if(!aud_dev_change_inprogress_) {
            pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_OUTPUT_SIGNAL_METER, &spk_lvl_);
            getSpeakerVol();
            getSpeakerMute();
        }

        client_thread_->PostDelayed(POLL_INTERVAL, this, MSG_START_SPK_MONITOR);
        spk_decay_timer_ -= POLL_INTERVAL;
    }
}

void bjn_sky::skinnySipManager::getMicSettings()
{
    if(mic_decay_timer_ != 0) {
        if(!aud_dev_change_inprogress_) {
            pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER, &mic_lvl_);
            getMicrophoneVol();
            getMicrophoneMute();
        }

        client_thread_->PostDelayed(POLL_INTERVAL, this, MSG_START_MIC_MONITOR);
        mic_decay_timer_ -= POLL_INTERVAL;
    }
}

void bjn_sky::skinnySipManager::monitorSpeakerMic()
{
    if(0 != mic_lvl_) {
        faulty_microphone_ = false;
    }
    if(0 != spk_lvl_) {
        faulty_speaker_ = false;
    }

    if(MAX_TIME_LEVEL_DETECTION == checkCount_)
    {
        if(client_thread_ && !spk_decay_timer_) {
            spk_decay_timer_ = DECAY_POLL_INTERVAL;
            getSpeakerSettings();
        }

        if(client_thread_ && !mic_decay_timer_) {
            mic_decay_timer_ = DECAY_POLL_INTERVAL;
            getMicSettings();
        }

        if((true == faulty_microphone_) && (false == mic_mute_))
        {
            LOG(LS_INFO) << "Notify UI that microphone is not working / muted";
            postFaultyMicrophoneNotification();
        }
        if((true == faulty_speaker_) && (false == spk_mute_))
        {
            LOG(LS_INFO) << "Notify UI that speaker is not working / muted";
            postFaultySpeakerNotification();
        }

        checkCount_ = 0;
    }
    else
    {
        checkCount_++;
        client_thread_->PostDelayed(1000, this, MSG_MONITOR_LEVEL);
    }
}

void bjn_sky::skinnySipManager::postGetMicrophoneVol(unsigned* mVol)
{
#ifdef WIN32
    //No need to post message to monitor mic
    //Changes in mic volume will be notified
    //via OnVolumeChange callback
    if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
        *mVol = m_volStats.m_micVolume;
        mic_vol_ = *mVol;
    }
    else{
#endif
        if (client_thread_ && !mic_decay_timer_) {
            mic_decay_timer_ = DECAY_POLL_INTERVAL;
            client_thread_->Post(this, MSG_START_MIC_MONITOR);
        }
        *mVol = mic_vol_;
#ifdef WIN32
    }
#endif
}

pj_status_t bjn_sky::skinnySipManager::getMicrophoneVol()
{
    pj_status_t status;
    status = pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_INPUT_VOLUME_SETTING, &mic_vol_);
    if (status != PJ_SUCCESS)
    {
        LOG(LS_INFO) << " Failed getMicrophoneVol->pjsua_snd_get_setting";
    }
    return status;
}

void bjn_sky::skinnySipManager::postGetSpeakerVol(unsigned* sVol)
{
#ifdef WIN32
    //No need to post message to monitor speaker
    //Changes in speaker volume will be notified
    //via OnVolumeChange callback
    if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
        *sVol = m_volStats.m_systemVolume;
        spk_vol_ = *sVol;
    }
    else{
#endif
        if (client_thread_ && !spk_decay_timer_) {
            spk_decay_timer_ = DECAY_POLL_INTERVAL;
            client_thread_->Post(this, MSG_START_SPK_MONITOR);
        }
        *sVol = spk_vol_;
#ifdef WIN32
    }
#endif
}

int bjn_sky::skinnySipManager::checkSpeaker(LocalMediaInfo &selectedDeviceoptions)
{
    std::string wavFilePath;
#ifdef FB_WIN
    wavFilePath = BJN::getBjnAppDataPath() + "\\" + CONFIG_DIRECTORY + "\\" + WAV_FILE_NAME;
#elif FB_MACOSX
    wavFilePath = BJN::getBjnAppDataPath() + "/" + MAC_RESOURCES_DIR  + WAV_FILE_NAME;
#elif FB_X11
    wavFilePath = BJN::getBjnAppDataPath() + "/" + CONFIG_DIRECTORY + "/" + WAV_FILE_NAME;
#endif

    if(!speakerPlay && isSipInitialized)
    {
        speakerPlay = true;
        pjsua_snd_set_setting(PJMEDIA_AUD_DEV_START_TEST_SOUND, wavFilePath.c_str(), false);
        client_thread_->PostDelayed(WAV_FILE_DURATION, this, MSG_STOP_TEST_SND);
    }
    return 0;
}

#ifdef FB_WIN
pj_status_t bjn_sky::skinnySipManager::winGetSpeakerVol(std::string id, float* vLevel)
{
    if(!deviceEnumerator)
    {
        LOG(LS_INFO) << "No COM object...";
        return -1;
    }

    hr = deviceEnumerator->EnumAudioEndpoints(typeDev, DEVICE_STATE_ACTIVE, &deviceCollection);
    if (FAILED(hr))
    {
        LOG(LS_INFO) <<  "EnumAudioEndpoints Failed";
        return -1;
    }
    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr))
    {
        LOG(LS_INFO) <<  "deviceCollection->GetCount Failed";
        deviceCollection->Release();
        return -1;
    }

    if(0 == (id.compare("-1")))
    {
        hr = deviceCollection->Item(0, &device);
        if (FAILED(hr))
        {
            LOG(LS_INFO) <<  "Default device deviceCollection Failed";
            deviceCollection->Release();
            return -1;
        }
        else
        {
            float mvLevel;
            // Query the speaker system volume.
            IAudioEndpointVolume* endpoint = NULL;
            hr = device->Activate(__uuidof(IAudioEndpointVolume),
                                  CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&endpoint));
            if(FAILED(hr)) {
                LOG(LS_ERROR) <<  "Failed to get audio endpoint:"<<hr;
                device->Release();
                deviceCollection->Release();
                return -1;
            }

            endpoint->GetMasterVolumeLevelScalar(&mvLevel);
            *vLevel = mvLevel;
            device->Release();
            endpoint->Release();
        }
    }
    else
    {
        for (UINT i = 0 ; i < deviceCount ; i += 1)
        {
            pwszID = NULL;
            hr = deviceCollection->Item(i, &device);
            if (FAILED(hr))
            {
                LOG(LS_INFO) <<  "DeviceCollection Failed";
                deviceCollection->Release();
                return -1;
            }
            else
            {
                hr = device->GetId(&pwszID);
                if (FAILED(hr))
                {
                    LOG(LS_INFO) <<  "DeviceCollection Failed";
                    device->Release();
                    deviceCollection->Release();
                    return -1;
                }

                oString.assign(pwszID);
                CoTaskMemFree(pwszID);
                string uid(oString.begin(), oString.end());
                uid.assign(oString.begin(), oString.end());
                oString.clear();

                if(0 == (id.compare(uid)))
                {
                    float mvLevel;
                    // Query the speaker system volume.
                    IAudioEndpointVolume* endpoint = NULL;
                    hr = device->Activate(__uuidof(IAudioEndpointVolume),
                        CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&endpoint));

                    if(FAILED(hr)) {
                        LOG(LS_ERROR) <<  "Failed to get audio endpoint:"<<hr;
                        device->Release();
                        deviceCollection->Release();
                        return -1;
                    }

                    endpoint->GetMasterVolumeLevelScalar(&mvLevel);
                    *vLevel = mvLevel;
                    endpoint->Release();
                    device->Release();
                    break;
                }
                else {
                    device->Release();
                }
            }
        }
    }
    deviceCollection->Release();
    return PJ_SUCCESS;
}

//Taken from http://msdn.microsoft.com/en-us/library/windows/desktop/dd940391.aspx
HRESULT bjn_sky::skinnySipManager::disableDucking()
{
    HRESULT hr = S_OK;

    IMMDevice* pEndpoint = NULL;
    IAudioSessionManager2* pSessionManager2 = NULL;
    IAudioSessionControl* pSessionControl = NULL;
    IAudioSessionControl2* pSessionControl2 = NULL;

    hr = deviceEnumerator->EnumAudioEndpoints(typeDev, DEVICE_STATE_ACTIVE, &deviceCollection);
    if (FAILED(hr))
    {
        LOG(LS_INFO) <<  "EnumAudioEndpoints Failed";
        return hr;
    }
    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr))
    {
        LOG(LS_INFO) <<  "deviceCollection->GetCount Failed";
        deviceCollection->Release();
        return hr;
    }

    for (UINT i = 0 ; i < deviceCount ; i += 1)
    {
        pwszID = NULL;
        hr = deviceCollection->Item(i, &pEndpoint);
        if (FAILED(hr))
        {
            LOG(LS_INFO) <<  "DeviceCollection Failed";
            deviceCollection->Release();
            return hr;
        }
        // Activate session manager.
        if (SUCCEEDED(hr))
        {
            hr = pEndpoint->Activate(__uuidof(IAudioSessionManager2),
                                     CLSCTX_INPROC_SERVER,
                                     NULL,
                                     reinterpret_cast<void **>(&pSessionManager2));
            pEndpoint->Release();
            pEndpoint = NULL;
        }
        if (SUCCEEDED(hr))
        {
            hr = pSessionManager2->GetAudioSessionControl(NULL, 0, &pSessionControl);
            pSessionManager2->Release();
            pSessionManager2 = NULL;
        }

        if (SUCCEEDED(hr))
        {
            hr = pSessionControl->QueryInterface(
                      __uuidof(IAudioSessionControl2),
                      (void**)&pSessionControl2);
            pSessionControl->Release();
            pSessionControl = NULL;
        }

        if (SUCCEEDED(hr))
        {
            hr = pSessionControl2->SetDuckingPreference(TRUE);
            LOG(LS_INFO) << "Ducking preference disabled with hr " << hr;
            pSessionControl2->Release();
            pSessionControl2 = NULL;
        }
    }
    deviceCollection->Release();
    return hr;
}
#endif


pj_status_t bjn_sky::skinnySipManager::getSpeakerVol()
{
    int vLevel;
    pj_status_t status;
#ifdef FB_WIN
    status = pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING, &vLevel);
    if(0 == bjn_sky::isXP()) // If windows not XP
    {
        float mvLevel;
        std::string speakerGUID= getSelSpeakerGUID();
        status =  winGetSpeakerVol(speakerGUID, &mvLevel);
        if(0.0 == mvLevel)
        {
            vLevel = 0;
        }
    }
#else
    status = pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING, &vLevel);
#endif
    if (status == PJ_SUCCESS)
    {
        spk_vol_ = vLevel;
    }
    else
    {
        LOG(LS_INFO) << " Failed getSpeakerVol->pjsua_snd_get_setting";
    }
    return status;
}

void bjn_sky::skinnySipManager::postSetSpeakerVol(unsigned* vol)
{
    if(client_thread_) {
        client_thread_->Post(this, MSG_SET_SPK_VOLUME, new talk_base::TypedMessageData<unsigned>(*vol));
    }
}

#ifdef WIN32

//This api sets the session/application volume not the system vol
//via core audio api's,session volume is specific to application

bool bjn_sky::skinnySipManager::SetSpeakerVolumeViaCoreAudio(float spkVol){
    cricket::Win32DeviceManager* winDeviceManager = dynamic_cast< cricket::Win32DeviceManager* >(device_manager_);
    if (NULL != winDeviceManager){
        return winDeviceManager->PostSetSessionVolume(spkVol);
    }
    else{
        LOG(LS_ERROR) << "Device manager pointer is null, "
            "not able to set speaker session volume";
    }
    return false;
}

#endif

pj_status_t bjn_sky::skinnySipManager::setSpeakerVol(unsigned* spkVol)
{
    pj_status_t status = PJ_EUNKNOWN;
#ifdef WIN32
    if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
        float level = 0.0f;
        level = (*((unsigned int*)spkVol)) / 100.0f;
        if (SetSpeakerVolumeViaCoreAudio(level)){
            status = PJ_SUCCESS;
        }
        else{
            LOG(LS_ERROR) << "Not able to set speaker vol via core audio";
            status = PJ_EUNKNOWN;
        }
    }
    else{
#endif
        status = pjsua_snd_set_setting(PJMEDIA_AUD_DEV_CAP_OUTPUT_VOLUME_SETTING, spkVol, false);
        if (status != PJ_SUCCESS)
        {
            LOG(LS_INFO) << " Failed setSpeakerVol->pjsua_snd_set_setting";
        }
#ifdef WIN32
    }
#endif
    return status;
}

#ifdef FB_WIN
pj_status_t bjn_sky::skinnySipManager::winGetSpeakerMute(std::string id, bool* muteStatus)
{
    if(!deviceEnumerator)
    {
        LOG(LS_INFO) << "No COM object...";
        return -1;
    }

    hr = deviceEnumerator->EnumAudioEndpoints(typeDev, DEVICE_STATE_ACTIVE, &deviceCollection);
    if (FAILED(hr))
    {
        LOG(LS_INFO) <<  "EnumAudioEndpoints Failed";
        return -1;
    }
    hr = deviceCollection->GetCount(&deviceCount);
    if (FAILED(hr))
    {
        LOG(LS_INFO) <<  "deviceCollection->GetCount Failed";
        deviceCollection->Release();
        return -1;
    }

    if(0 == (id.compare("-1")))
    {
        hr = deviceCollection->Item(0, &device);
        if (FAILED(hr))
        {
            LOG(LS_INFO) <<  "Default device deviceCollection Failed";
            deviceCollection->Release();
            return -1;
        }
        else
        {
            BOOL mute;
            // Query the speaker system mute state.
            IAudioEndpointVolume* endpoint = NULL;
            hr = device->Activate(__uuidof(IAudioEndpointVolume),
                                  CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&endpoint));
            if(FAILED(hr)) {
                LOG(LS_ERROR) <<  "Failed to get audio endpoint:"<<hr;
                device->Release();
                deviceCollection->Release();
                return -1;
            }

            endpoint->GetMute(&mute);
            *muteStatus = (mute!=0);
            endpoint->Release();
            device->Release();
        }
    }
    else
    {
        for (UINT i = 0 ; i < deviceCount ; i += 1)
        {
            pwszID = NULL;
            hr = deviceCollection->Item(i, &device);
            if (FAILED(hr))
            {
                LOG(LS_INFO) <<  "DeviceCollection Failed";
                deviceCollection->Release();
                return -1;
            }
            else
            {
                hr = device->GetId(&pwszID);
                if (FAILED(hr))
                {
                    LOG(LS_INFO) <<  "DeviceCollection Failed";
                    device->Release();
                    deviceCollection->Release();
                    return -1;
                }

                oString.assign(pwszID);
                CoTaskMemFree(pwszID);
                string uid(oString.begin(), oString.end());
                uid.assign(oString.begin(), oString.end());
                oString.clear();

                if(0 == (id.compare(uid)))
                {
                    BOOL mute;
                    // Query the speaker system mute state.
                    IAudioEndpointVolume* endpoint = NULL;
                    hr = device->Activate(__uuidof(IAudioEndpointVolume),
                                          CLSCTX_ALL, NULL,  reinterpret_cast<void**>(&endpoint));

                    if(FAILED(hr)) {
                        LOG(LS_ERROR) <<  "Failed to get audio endpoint:"<<hr;
                        device->Release();
                        deviceCollection->Release();
                        return -1;
                    }
                    endpoint->GetMute(&mute);
                    *muteStatus = (mute!=0);
                    endpoint->Release();
                    device->Release();
                    break;
                }
                else {
                    device->Release();
                }
            }
        }
    }
    deviceCollection->Release();
    return PJ_SUCCESS;
}
#endif

void bjn_sky::skinnySipManager::stopTestSound()
{
    pjsua_snd_set_setting(PJMEDIA_AUD_DEV_START_TEST_SOUND, NULL, false);
    speakerPlay = false;
}

void bjn_sky::skinnySipManager::OnDevicesChange(bool dAction)
{
    LOG(LS_INFO) << "Devices changed.";
    if(client_thread_) {
        if(true == dAction)
        {
            dev_.devAction = ADDED;
        }
        else
        {
            dev_.devAction = REMOVED;
        }
      client_thread_->PostDelayed(150,this, MSG_DEVICES_CHANGED, new talk_base::TypedMessageData<bool>(true));
    }
}

#ifdef WIN32
//This function will get the volume stats from audio listener
void bjn_sky::skinnySipManager::OnVolumeChange(const cricket::VolumeStats& volstats)
{
    if (client_thread_){
        client_thread_->Post(this, MSG_ENDPOINT_VOLUME_CHANGED, new talk_base::TypedMessageData<cricket::VolumeStats>(volstats));
    }
}
#endif

void bjn_sky::skinnySipManager::OnDesktopsChange(bool layoutChanged, bool endSharing)
{
    LOG(LS_INFO) << "Desktops changed. layoutChanged = " << layoutChanged << ", endSharing = " << endSharing;
    if(client_thread_) {
      // post this delayed as changing monitor layout takes while
      ScreenDeviceConfigChange changed(layoutChanged, endSharing);
      client_thread_->PostDelayed(500, this, MSG_DESKTOPS_CHANGED,
          new talk_base::TypedMessageData<ScreenDeviceConfigChange>(changed));
    }
}

void bjn_sky::skinnySipManager::postInitializeMediaEngine(LocalMediaInfo &selectedDevices)
{
  if(client_thread_) {
      client_thread_->Post(this, MSG_INIT_MEDIA,
                           new talk_base::TypedMessageData<LocalMediaInfo>(selectedDevices));
  }
}

void bjn_sky::skinnySipManager::postEnumerateScreenDevices(int width, int height)
{
  if(client_thread_) {
      std::pair<int, int> thumbnailSize;
      thumbnailSize = std::pair<int, int>(width, height);
      client_thread_->Post(this, MSG_ENUMERATE_MONITORS,
                           new talk_base::TypedMessageData<std::pair<int, int> >(thumbnailSize));
  }
}

void bjn_sky::skinnySipManager::postSetDeviceMute(IDeviceController::muteTypes usage, bool isMute)
{
    if(client_thread_) {
        std::pair<IDeviceController::muteTypes, bool> deviceMuteState;
        deviceMuteState = std::pair<IDeviceController::muteTypes, bool>(usage, isMute);
        postToClient(MSG_SET_DEVICE_MUTE, deviceMuteState);
    }
}

void bjn_sky::skinnySipManager::postSetDeviceState(uint16_t usage, bool isMute)
{
    if(client_thread_) {
        std::pair<uint16_t, bool> deviceState;
        deviceState = std::pair<uint16_t, bool>(usage, isMute);
        postToClient(MSG_SET_DEVICE_STATE, deviceState);
    }
}

void bjn_sky::skinnySipManager::postEnumerateApplications(int width, int height)
{
  if(client_thread_) {
      std::pair<int, int> thumbnailSize;
      thumbnailSize = std::pair<int, int>(width, height);
      client_thread_->Post(this, MSG_ENUMERATE_APPLICATIONS,
                           new talk_base::TypedMessageData<std::pair<int, int> >(thumbnailSize));
  }
}

bool bjn_sky::skinnySipManager::isAppSharingSupported()
{
    return window_picker_->IsApplicationSharingSupported();
}

void bjn_sky::skinnySipManager::platformHandleMessage(talk_base::Message *msg)
{
    switch (msg->message_id) {

        case MSG_SET_AUDIO_CONFIG_OPTIONS: {
            talk_base::TypedMessageData<AudioConfigOptions>* data =
                static_cast<talk_base::TypedMessageData<AudioConfigOptions> *>(msg->pdata);
            AudioConfigOptions options = data->data();
            if(mIsHuddleModeEnabled)
            {
                //if this case is triggered by moderator mute/Unmute, set device state accordingly.
                std::map<std::string, int>::iterator it;
                it = options.find("dnm_mute");
                if (it != options.end())
                {
                    postSetDeviceMute(IDeviceController::remoteMute, options["dnm_mute"]);
                }
            }
            pjsua_snd_set_setting(PJMEDIA_AUD_DEV_CAP_CONFIG_OPTIONS,
                               &options,
                               false);
            delete data;
            break;
        }
        case MSG_SET_AUDIO_OPTIONS: {
            UNIMPLEMENTED_CASE(MSG_SET_AUDIO_OPTIONS);
            break;
        }
        case MSG_START_SPK_MONITOR: {
            getSpeakerSettings();
            break;
        }
        case MSG_START_MIC_MONITOR: {
            getMicSettings();
            break;
        }
        case MSG_MONITOR_LEVEL: {
            monitorSpeakerMic();
            break;
        }
        case MSG_CHECK_SPEAKER: {
            talk_base::TypedMessageData<LocalMediaInfo&> *data =
                static_cast<talk_base::TypedMessageData<LocalMediaInfo&> *>(msg->pdata);
            LocalMediaInfo& selectedDevices = data->data();
            if( checkSpeaker(selectedDevices) )
            {
                browser_thread_->Post(browser_msg_hndler_, APP_MSG_SPK_FAIL,
                                      new talk_base::TypedMessageData<int>(ERROR_SPK_FAILED));
            }
            delete data;
            break;
        }
        case MSG_SET_SPK_VOLUME: {
            talk_base::TypedMessageData<unsigned> *data =
                static_cast<talk_base::TypedMessageData<unsigned> *>(msg->pdata);
            unsigned spkVol = data->data();
            setSpeakerVol(&spkVol);
            delete data;
            break;
        }
        case MSG_STOP_TEST_SND: {
            stopTestSound();
            break;
        }
#ifdef WIN32
        case MSG_ENDPOINT_VOLUME_CHANGED:{
            talk_base::TypedMessageData<cricket::VolumeStats> *data = static_cast<talk_base::TypedMessageData<cricket::VolumeStats> *>(msg->pdata);
            m_volStats = data->data();
            delete data;
            break;
        }
#endif
        case MSG_DEVICES_CHANGED: {
            talk_base::TypedMessageData<bool> *data =
                static_cast<talk_base::TypedMessageData<bool> *>(msg->pdata);
            bool retry = data->data();
            ChangeDevInfoList changedDevices;
            if(ADDED == dev_.devAction)
            {
                if(false == findAndSetAddedDevice())
                {
                    if(retry) {
                        LOG(LS_INFO) << "No new added devices found.Retrying once.";
                        client_thread_->PostDelayed(1000, this, MSG_DEVICES_CHANGED, new talk_base::TypedMessageData<bool>(false));
                        delete data;
                        return;
                    } else {
                        LOG(LS_INFO) << "No new added devices found.";
                        delete data;
                        return;
                    }
                }
                OnDevicesChange(true);
            }
            else
            {
                if(false == findAndSetRemovedDevice())
                {
                    if(retry) {
                        LOG(LS_INFO) << "No device is removed.Retrying once.";
                        client_thread_->PostDelayed(1000, this, MSG_DEVICES_CHANGED, new talk_base::TypedMessageData<bool>(false));
                        delete data;
                        return;
                    } else {
                        LOG(LS_INFO) << "No device is removed";
                        delete data;
                        return;
                    }
                }
                OnDevicesChange(false);
            }

            if(VIDEO != dev_.devType)
            {
                pjmedia_aud_dev_refresh();
                if(mMeetingFeatures.mDepreferUsbOnThunderbolt) {
                    updateUsbOnThunderboltMap(dev_.devName,
                                              dev_.devGuid,
                                              (dev_.devAction == ADDED),
                                              (dev_.devType == AUDIO_IN));
                }
            }
            else
            {
                updateVideoDevice();
            }

            if(mIsHuddleModeEnabled && (VIDEO == dev_.devType))
            {
                //if huddle mode is enabled and there is change in VIDEO device, we reenumerate SRK supported devices devices
                //as the changed device could be the conference cam device.
                postToClientDelayed(mDeviceController->getDeviceEnumerateWaitTime(), MSG_REENUMERATE_HUDDLE_DEVICES, (ADDED == dev_.devAction));
            }

            changedDevices.push_back(dev_);
            browser_thread_->Post(browser_msg_hndler_, APP_MSG_NOTIFY_LOCAL_DEVICES_CHANGE,
                new talk_base::TypedMessageData<ChangeDevInfoList>(changedDevices));

            delete data;
            break;
        }
        case MSG_INIT_MEDIA: {
            mAutoSpkMic.clear();
            talk_base::TypedMessageData<LocalMediaInfo> *data =
                static_cast<talk_base::TypedMessageData<LocalMediaInfo> *>(msg->pdata);
            LocalMediaInfo& selectedDevices = data->data();
            std::string spkId, spkName;
            std::string micId, micName;
            std::string vendorId;
            std::string productId;
            std::string classId;
            std::string deviceId;
            AudioDeviceSelection    micSelection;
            AudioDeviceSelection    spkrSelection;

            if(AutomaticDeviceGuid == selectedDevices.get_MicrophoneID() &&
               AutomaticDeviceGuid == selectedDevices.get_SpeakerID()) {
                getAutomaticMic(micId, micName);
                GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic, micName, micId, vendorId, productId, classId, deviceId, false);
                LOG(LS_INFO) << "Microphone id: " << deviceId;

                micSelection = AUDIO_DEVICE_SELECT_AUTO;
                if(!getMatchingDevice(deviceId, DeviceConfigTypeSpeaker, audioPlayoutDevices_, spkName, spkId)) {
                    LOG(LS_INFO) << "No matching device found.";
                    getAutomaticSpk(spkId, spkName);
                    if(GetAudioHardwareIdFromDeviceId(DeviceConfigTypeSpeaker, spkName, spkId, vendorId, productId, classId, deviceId, false))
                    {
                        if(getMatchingDevice(deviceId, DeviceConfigTypeMic, audioCaptureDevices_, micName, micId)) {
                            client_thread_->Clear(this, MSG_AUTO_MIC_LEVEL_CHECK);
                            micSelection = AUDIO_DEVICE_SELECT_PAIRED;
                        }
                    }
                    spkrSelection = AUDIO_DEVICE_SELECT_AUTO;
                } else {
                    spkrSelection = AUDIO_DEVICE_SELECT_PAIRED;
                }

                selectedDevices.setAudiocapDeviceID(micId);
                selectedDevices.setAudioCaptureName(micName);

                selectedDevices.setAudioplayoutDeviceID(spkId);
                selectedDevices.setAudioPlayoutName(spkName);
            }
            else if(AutomaticDeviceGuid != selectedDevices.get_MicrophoneID() &&
                    AutomaticDeviceGuid == selectedDevices.get_SpeakerID()) {
                micId = selectedDevices.get_MicrophoneID();
                micName = selectedDevices.get_Microphone();
                GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic, micName, micId, vendorId, productId, classId, deviceId, false);
                LOG(LS_INFO) << "Microphone id: " << deviceId << " manually selected";

                micSelection = AUDIO_DEVICE_SELECT_MANUAL;

                if(!getMatchingDevice(deviceId, DeviceConfigTypeSpeaker, audioPlayoutDevices_, spkName, spkId)) {
                    LOG(LS_INFO) << "No matching device found. Using automatic speaker " << deviceId;
                    getAutomaticSpk(spkId, spkName);
                    spkrSelection = AUDIO_DEVICE_SELECT_AUTO;
                } else {
                    LOG(LS_INFO) << "Using paired speaker " << deviceId;
                    spkrSelection = AUDIO_DEVICE_SELECT_PAIRED;
                }
                selectedDevices.setAudioplayoutDeviceID(spkId);
                selectedDevices.setAudioPlayoutName(spkName);
            }
            else if(AutomaticDeviceGuid == selectedDevices.get_MicrophoneID() &&
                    AutomaticDeviceGuid != selectedDevices.get_SpeakerID()) {
                spkId = selectedDevices.get_SpeakerID();
                spkName = selectedDevices.get_Speaker();
                GetAudioHardwareIdFromDeviceId(DeviceConfigTypeSpeaker, spkName, spkId, vendorId, productId, classId, deviceId, false);
                LOG(LS_INFO) << "Speaker id: " << deviceId;

                spkrSelection = AUDIO_DEVICE_SELECT_MANUAL;
                if(!getMatchingDevice(deviceId, DeviceConfigTypeMic, audioCaptureDevices_, micName, micId)) {
                    LOG(LS_INFO) << "No matching device found Using automatic mic " << deviceId;
                    getAutomaticMic(micId, micName);
                    micSelection = AUDIO_DEVICE_SELECT_AUTO;
                } else {
                    LOG(LS_INFO) << "Using paired mic " << deviceId;
                    micSelection = AUDIO_DEVICE_SELECT_PAIRED;
                }
                selectedDevices.setAudiocapDeviceID(micId);
                selectedDevices.setAudioCaptureName(micName);
            } else {
                micSelection = AUDIO_DEVICE_SELECT_MANUAL;
                spkrSelection = AUDIO_DEVICE_SELECT_MANUAL;
                micId = selectedDevices.get_MicrophoneID();
                micName = selectedDevices.get_Microphone();
                spkId = selectedDevices.get_SpeakerID();
                spkName = selectedDevices.get_Speaker();
                LOG(LS_INFO) << "Using manually selected mic and speakers mic = " << micId <<
                    " spkr = " << spkId;
            }
            selectedDevices.setAudioCaptureSelection(micSelection);
            selectedDevices.setAudioPlaybackSelection(spkrSelection);

            SetLocalDevicesOptions(selectedDevices);
            ChangeDevices(selectedDevices);
            // update the device ids that we report to Indigo

            // If we are unable to query the device hardware IDs
            // The device is probably invalid in which case we will continue to use
            // older device. However set the dev_id_ to the error code for PJMEDIA_AUD_INVALID_DEV
            // so that such events are trackable in Indigo. For invalid devices, the name reported
            // in Indigo will be of the device being used.

            if(GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic,
                                              selectedDevices.get_Microphone(),
                                              selectedDevices.get_MicrophoneID(),
                                              vendorId,
                                              productId,
                                              classId,
                                              deviceId,
                                              false)) {
                cap_dev_id_ = deviceId;
            } else {
                cap_dev_id_ = boost::to_string(static_cast<int>(PJMEDIA_AUD_INVALID_DEV));
            }

            if(GetAudioHardwareIdFromDeviceId(DeviceConfigTypeSpeaker,
                                              selectedDevices.get_Speaker(),
                                              selectedDevices.get_SpeakerID(),
                                              vendorId,
                                              productId,
                                              classId,
                                              deviceId,
                                              false)) {
                play_dev_id_ = deviceId;
            } else {
                play_dev_id_ = boost::to_string(static_cast<int>(PJMEDIA_AUD_INVALID_DEV));
            }

            // Update automatic device label name and notify the JavaScript layer
            if (mMeetingFeatures.mLabelAutomaticDevice) {
                updateAutomaticDeviceLabel(micSelection, spkrSelection, micId, micName, spkId, spkName);
            }

#ifdef WIN32
            if (bjn_sky::getAudioFeatures()->mCoreAudioDevNotify){
                cricket::Win32DeviceManager* winDeviceManager = dynamic_cast<cricket::Win32DeviceManager*>(device_manager_);
                if (NULL != winDeviceManager){
                    bool bnewSpeakerGUIDSet = winDeviceManager->PostSetSpeakerGUID(getSelSpeakerGUID());
                    if (bnewSpeakerGUIDSet == false){
                        LOG(LS_ERROR) << "Error while setting guid for the current selected speaker";
                    }
                    bool bnewMicGUIDSet = winDeviceManager->PostSetMicGUID(selectedDevices.get_MicrophoneID());
                    if (bnewMicGUIDSet == false){
                        LOG(LS_ERROR) << "Error while setting guid for the current selected mic";
                    }
                }
                else{
                    LOG(LS_ERROR) << "Not able to get pointer to device manager to set the speaker guid";
                }
            }
#endif
            delete data;
            break;
        }
        case MSG_AUTO_MIC_LEVEL_CHECK: {
            if(auto_level_check_count_) {
                pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER, &mic_lvl_);
                LOG(LS_INFO) << "Microphone level is: " << mic_lvl_;
                --auto_level_check_count_;
                if(mic_lvl_) ++level_determined_;
                client_thread_->PostDelayed(100, this, MSG_AUTO_MIC_LEVEL_CHECK);
            } else {
                cricket::Device defaultPlayDevice = ((cricket::DeviceManager *) device_manager_)->getDefaultPlayDevice();
                cricket::Device defaultCapDevice = ((cricket::DeviceManager *) device_manager_)->getDefaultCapDevice();

                if(level_determined_) {
                    LOG(LS_INFO) << "Microphone level determined " << level_determined_ << " times";
                } else {
                    LOG(LS_INFO) << "No level determined. Switching Devices " << defaultCapDevice.name << " " << defaultCapDevice.id;
                    LOG(LS_INFO) << "Also changing speakers to " << defaultPlayDevice.name << " " << defaultPlayDevice.id;

                    LocalMediaInfo *selectedDevices = new LocalMediaInfo(selectedDeviceoptions_);
                    selectedDevices->setAudioCaptureName(defaultCapDevice.name);
                    selectedDevices->setAudiocapDeviceID(defaultCapDevice.id);
                    selectedDevices->setAudioPlayoutName(defaultPlayDevice.name);
                    selectedDevices->setAudioplayoutDeviceID(defaultPlayDevice.id);

                    client_thread_->Post(this, MSG_INIT_MEDIA,
                                         new talk_base::TypedMessageData<LocalMediaInfo>(*selectedDevices));
                }
            }
            break;
        }
        case MSG_ENUMERATE_MONITORS: {
            talk_base::TypedMessageData<std::pair<int, int> > *data =
                static_cast<talk_base::TypedMessageData<std::pair<int, int> > *>(msg->pdata);
            std::pair<int, int> thumbnailSize = data->data();
            int thumbnailWidth = thumbnailSize.first;
            int thumbnailHeight = thumbnailSize.second;
            delete data;

            bool generateThumbnails = true;
            // if width or height are zero or less don't return thumbnails
            if ((thumbnailWidth <= 0) || (thumbnailHeight <= 0))
            {
                generateThumbnails = false;
            }
            SharingDescriptionList screenList;
            talk_base::DesktopDescriptionList desktopList;
            window_picker_->GetDesktopList(&desktopList);
            // grab thumbnail image of each screen from the window picker
            float thumbnailAspect = (float) thumbnailWidth / (float) thumbnailHeight;
            for (size_t i = 0; i < desktopList.size(); i++)
            {
                std::string png;
                int width, height;
                talk_base::DesktopDescription desc = desktopList[i];
                int screenIndex = desc.id().index();
                if (generateThumbnails)
                {
                    window_picker_->GetDesktopDimensions(desc.id(), &width, &height);
                    // modify the thumbnail size to maintain the screen aspect ratio
                    float desktopAspect = (float) width / (float) height;
                    int finalWidth = thumbnailWidth;
                    int finalHeight = thumbnailHeight;
                    if (thumbnailAspect > desktopAspect)
                    {
                        // adjust the width
                        finalWidth = (int) (thumbnailHeight * desktopAspect);
                    }
                    else
                    {
                        // adjust the height
                        finalHeight = (int) (thumbnailWidth / desktopAspect);
                    }
                    uint8_t* thumbData = NULL;
                    thumbData = window_picker_->GetDesktopThumbnail(desc.id(), finalWidth, finalHeight);
                    if (thumbData)
                    {
                        png = BJN::ConvertImageToPNGBase64(thumbData, finalWidth, finalHeight);
                        window_picker_->FreeThumbnail(thumbData);
                    }
                }
                SharingDescription screenDesc;
                screenDesc.mId = screenIndex;
                screenDesc.mScreenName = BJN::DevicePropertyManager::getScreenSharingDeviceFriendlyName(screenIndex);
                screenDesc.mScreenId = BJN::DevicePropertyManager::getScreenSharingDeviceUniqueID(screenIndex, 0);
                screenDesc.mThumbnailPNG = png;
                screenList.push_back(screenDesc);
            }
            browser_thread_->Post(browser_msg_hndler_, APP_MSG_SCREEN_CAPTURE_LIST_UPDATE,
                new talk_base::TypedMessageData<SharingDescriptionList>(screenList));
            break;
        }
        case MSG_DESKTOPS_CHANGED: {
            talk_base::TypedMessageData<ScreenDeviceConfigChange> *data =
                static_cast<talk_base::TypedMessageData<ScreenDeviceConfigChange> *>(msg->pdata);
            ScreenDeviceConfigChange screenConfig = data->data();
            delete data;

            if (screenConfig.mLayoutChanged)
            {
                // make sure the capture device list is current, the real video capture device
                // hasn't changed but it's index might have so we need to reset it
                updateVideoDevice();
            }

            // notify the browser that they might have to stop screen sharing or refresh the thumbnail list
            browser_thread_->Post(browser_msg_hndler_, APP_MSG_NOTIFY_SCREEN_CAPTURE_DEVICES_CHANGE,
                new talk_base::TypedMessageData<ScreenDeviceConfigChange>(screenConfig));
            break;
        }
        case MSG_SET_DEVICE_STATE: {
            talk_base::TypedMessageData<std::pair<uint16_t, bool> > *data =
                static_cast<talk_base::TypedMessageData<std::pair<uint16_t, bool> > *>(msg->pdata);

            std::pair<uint16_t, bool> deviceState = data->data();
            uint16_t usage = deviceState.first;
            bool state = deviceState.second;
            bool ret = false;
            if(mDeviceController)
            {
                ret = mDeviceController->setDeviceState(usage, state);
                if(!ret)
                {
                    LOG(LS_ERROR) << "LogitechSRK: device operation " << usage << " Failed";
                }


                deviceOperationStatus devStatus;
                devStatus.usage = deviceState.first;
                devStatus.usageOn = deviceState.second;
                devStatus.success = ret;

                browser_thread_->Post(browser_msg_hndler_, APP_MSG_NOTIFY_DEVICE_OPERATION_STATUS, new talk_base::TypedMessageData<deviceOperationStatus>(devStatus));
            }
            delete data;
            break;
        }
        case MSG_SET_DEVICE_MUTE: {
            talk_base::TypedMessageData<std::pair<IDeviceController::muteTypes, bool> > *data =
                static_cast<talk_base::TypedMessageData<std::pair<IDeviceController::muteTypes, bool> > *>(msg->pdata);

            std::pair<IDeviceController::muteTypes, bool> deviceMuteState = data->data();
            bool ret = false;
            if(mDeviceController)
            {
                ret = mDeviceController->setDeviceMute(deviceMuteState.first, deviceMuteState.second);
                if(!ret)
                {
                    LOG(LS_ERROR) << "LogitechSRK: device Mute Failed";
                }

                deviceOperationStatus devStatus;
                devStatus.usage = IDeviceController::DeviceMute;
                devStatus.usageOn = deviceMuteState.second;
                devStatus.success = ret;

                browser_thread_->Post(browser_msg_hndler_, APP_MSG_NOTIFY_DEVICE_OPERATION_STATUS, new talk_base::TypedMessageData<deviceOperationStatus>(devStatus));
            }
            delete data;
            break;
        }
        //case MSG_SETUP_HUDDLE_MODE: {
        //    if(!mDeviceController)
        //    {
        //        mDeviceController = IDeviceController::createDeviceController();
        //        if(mDeviceController)
        //        {
        //            if(!mDeviceController->initialize())
        //            {
        //                LOG(LS_ERROR) << "LogitechSRK: mDeviceController initialization failed";
        //                break;
        //            }
        //            mDeviceController->setEventHandler(this);
        //            mIsHuddleModeEnabled = true;
        //        }
        //        else
        //        {
        //            LOG(LS_ERROR) << "LogitechSRK: mDeviceController creation failed";
        //        }
        //    }
        //    break;
        //}
        case MSG_REENUMERATE_HUDDLE_DEVICES: {
            talk_base::TypedMessageData<bool> *data =
                static_cast<talk_base::TypedMessageData<bool> *>(msg->pdata);
            bool deviceAdded = data->data();

            if(mDeviceController)
            {
                mDeviceController->processDeviceChange(deviceAdded);
            }
            delete data;
            break;
        }
        case MSG_ENUMERATE_APPLICATIONS: {
            talk_base::TypedMessageData<std::pair<int, int> > *data =
                static_cast<talk_base::TypedMessageData< std::pair<int, int> > *>(msg->pdata);
            std::pair<int, int> thumbnailSize = data->data();
            int thumbnailWidth = thumbnailSize.first;
            int thumbnailHeight = thumbnailSize.second;
            delete data;

            bool generateThumbnails = true;
            // if width or height are zero or less don't return thumbnails
            if ((thumbnailWidth <= 0) || (thumbnailHeight <= 0))
            {
                generateThumbnails = false;
            }
            SharingDescriptionList screenList;
            // on systems where application sharing isn't supported or
            // implemented an empty list will be returned
            talk_base::ApplicationDescriptionList appList;
            window_picker_->GetApplicationList(&appList);
            for (size_t i = 0; i < appList.size(); i++)
            {
                talk_base::ApplicationDescription desc = appList[i];
                SharingDescription screenDesc;
                screenDesc.mScreenName = BJN::DevicePropertyManager::getScreenSharingDeviceFriendlyName(0);
                screenDesc.mScreenId = BJN::DevicePropertyManager::getScreenSharingDeviceUniqueID(0, desc.id());
                screenDesc.mName = desc.name();
                screenDesc.mFileDescription = desc.file_description();
                screenDesc.mProductName = desc.product_name();
                screenDesc.mWindowList = desc.get_windows();
                screenDesc.mIntegrityLevel = desc.integrity_level();
                if (generateThumbnails)
                {
                    uint8_t* thumbData = NULL;
                    thumbData = window_picker_->GetApplicationThumbnail(desc, thumbnailWidth, thumbnailHeight);
                    if (thumbData)
                    {
                        std::string png = BJN::ConvertImageToPNGBase64(thumbData, thumbnailWidth, thumbnailHeight);
                        screenDesc.mThumbnailPNG = png;
                        window_picker_->FreeThumbnail(thumbData);
                    }
                }
                screenList.push_back(screenDesc);
            }
            browser_thread_->Post(browser_msg_hndler_, APP_MSG_APPLICATION_LIST_UPDATE,
                new talk_base::TypedMessageData<SharingDescriptionList>(screenList));
            break;
        }
        case MSG_SEND_ESM_STATUS: {
            boost::scoped_ptr<talk_base::TypedMessageData<pjmedia_event_esm_notify_data> > data(
                static_cast<talk_base::TypedMessageData<pjmedia_event_esm_notify_data> *>(msg->pdata));
            sendEsmStatus(data->data());
            break;
        }
        case MSG_SEND_ECBS_STATUS: {
            boost::scoped_ptr<talk_base::TypedMessageData<pjmedia_event_ecbs_data> > data(
                static_cast<talk_base::TypedMessageData<pjmedia_event_ecbs_data> *>(msg->pdata));
            sendEchoCancellerBufferStarvation(data->data());
            break;
        }
        case MSG_SET_FOCUS_APP: {
            talk_base::TypedMessageData<uint32_t> *data =
                static_cast<talk_base::TypedMessageData<uint32_t> *>(msg->pdata);
            uint32_t appID = data->data();
            setFocusApp(appID);
            delete data;
            break;
        }
        case MSG_NETWORK_INTERFACE_UP: {
            handleNetworkUpEvent();
            break;
        }
        case MSG_NETWORK_INTERFACE_DOWN: {
            handleNetworkDownEvent();
            break;
        }
        case MSG_AUD_EVENT_MIC_ANIMATION:{
            const int mic_animation_delay_ms = 250;
            talk_base::TypedMessageData<int> *data =
                static_cast<talk_base::TypedMessageData<int> *>(msg->pdata);
            browser_thread_->Clear(browser_msg_hndler_, APP_MSG_MIC_ANIMATION_STATE);
            browser_thread_->PostDelayed(mic_animation_delay_ms,
                browser_msg_hndler_,
                APP_MSG_MIC_ANIMATION_STATE,
                new talk_base::TypedMessageData<int>(data->data()));
            delete data;
            break;
        }
        case MSG_ENABLE_AUDIO_ENGINE: {
            talk_base::TypedMessageData<bool> *data =
                static_cast<talk_base::TypedMessageData<bool> *>(msg->pdata);
            enableAudioEngine(data->data());
            delete data;
            break;
        }
    }
}

void bjn_sky::skinnySipManager::handleNetworkUpEvent() {
    LOG(LS_INFO) << "In function " << __FUNCTION__;

#ifdef FB_X11
    pj_sockaddr addr[16];
    unsigned cnt = PJ_ARRAY_SIZE(addr);

    //We have a network up event..lets double check if we have valid IP,if so reset the n/wdown flag.
    if (mNetworkDown && (pj_enum_ip_interface(pj_AF_INET(), &cnt, addr) == PJ_SUCCESS)) {
       mNetworkDown = false;
       postToBrowser(APP_MSG_FIBER_EVENT,FBR_EV_NW_INTERFACE_ADDED);
    }else if(mNetworkDown && (pj_has_ipv6_interface() == PJ_SUCCESS) && mMeetingFeatures.mNat64Ipv6) {
         mNetworkDown = false;
         postToBrowser(APP_MSG_FIBER_EVENT,FBR_EV_NW_INTERFACE_ADDED);
    }
#else
    // For windows and osx, the default delay in re-ice by 2 secs is sufficient and
    // no further check needs to be done.
    if (mNetworkDown)
    {
        mNetworkDown = false;
        postToBrowser(APP_MSG_FIBER_EVENT,FBR_EV_NW_INTERFACE_ADDED);
    }
#endif

    if(mReIcePending && !mNetworkDown) {
      LOG(LS_INFO) << "Trigger Pending Re-Ice ";
      postNetworkEvent(true, true, DEFAULT_PENDING_REICE_MSEC);
      mReIcePending = false;
    }
}

void bjn_sky::skinnySipManager::handleNetworkDownEvent() {
    LOG(LS_INFO) << "In function " << __FUNCTION__;
    pj_sockaddr addr[16];
    unsigned cnt = PJ_ARRAY_SIZE(addr);
    if (!mNetworkDown && (pj_enum_ip_interface(pj_AF_INET(), &cnt, addr) != PJ_SUCCESS)) {
       mNetworkDown = true;
       if((pj_enum_ip_interface(pj_AF_INET6(), &cnt, addr) == PJ_SUCCESS) && mMeetingFeatures.mNat64Ipv6) {
              mNetworkDown = false;
       }
       if (mNetworkDown) {
         LOG(LS_ERROR) << "No Network!!";
         postToBrowser(APP_MSG_FIBER_EVENT,FBR_EV_NW_INTERFACE_REMOVED);
       }
    }
}

#ifdef FB_MACOSX
void bjn_sky::skinnySipManager::turnOffAllVideoStreams()
{
    int i = 0;
    LOG(LS_INFO) << "In function " << __FUNCTION__;
    setCaptureDevOff();
    for(i = 1; i < MAX_MED_IDX; i++) {
        pjsua_stop_video_stream(call_id_, startDirection(i), i);
    }
}
#endif

int bjn_sky::isXP()
{
#ifdef WIN32
    OSVERSIONINFO info = {0};
    info.dwOSVersionInfoSize = sizeof(info);
    GetVersionEx(&info);
    char str[30];
    wcstombs(str, info.szCSDVersion, 30);
    if( info.dwMajorVersion == XP_MAJOR_VERSION && info.dwMinorVersion == XP_MINOR_VERSION )
    {
        if( !strcmp(str,"Service Pack 1") || !strcmp(str,"Service Pack 2") )
            return -1;      //XP version not supported
        else if( !strcmp(str,"Service Pack 3") )
            return 1;       //XP version supported
    }
#endif
    return 0;               //other windows os
}

void bjn_sky::skinnySipManager::platformPreDeviceOperation()
{
    client_thread_->Clear(this, MSG_START_MIC_MONITOR);
    mic_decay_timer_ = 0;
    client_thread_->Clear(this, MSG_START_SPK_MONITOR);
    spk_decay_timer_ = 0;
    client_thread_->Clear(this, MSG_MONITOR_LEVEL);
    aud_dev_change_inprogress_ = true;
}

void bjn_sky::skinnySipManager::platformPostDeviceOperation()
{
#ifdef WIN32
    unsigned int speakerVolMax = 100;
    setSpeakerVol(&speakerVolMax);
#endif
    faulty_microphone_ = true;
    faulty_speaker_ = true;
    checkCount_ = 0;
    aud_dev_change_inprogress_ = false;
    monitorSpeakerMic();
}

pj_bool_t bjn_sky::skinnySipManager::platformHangUp(pj_status_t status)
{
    if( speakerPlay )
    {
        stopTestSound();
    }
    return PJ_TRUE;
}

bool bjn_sky::skinnySipManager::platformPresentationEnable(bool enable,
                                const std::string& screenName,
                                const std::string& screenId,
                                uint32_t appId,
                                uint64_t winId)
{
    if (enable)
    {
        int deviceIndex = BJN::DevicePropertyManager::getScreenSharingDeviceIndex(screenId);
        window_picker_->RegisterAppTerminateNotify(appId, deviceIndex);
        if (winId != 0)
        {
            window_picker_->MoveToFront(talk_base::WindowId::Cast(winId));
        }
    }
    else
    {
        window_picker_->UnregisterAppTerminateNotify(appId, -1);
    }
    return true;
}

void bjn_sky::skinnySipManager::platformPresentationConfigure(pj_uint32_t appId)
{
    pjmedia_vid_dev_index content_idx = PJMEDIA_VID_INVALID_DEV;
    pjmedia_vid_dev_index rend_idx = PJMEDIA_VID_INVALID_DEV;
    platformGetContentDevice(content_idx, rend_idx, mCurrScreenName, mCurrScreenId);

    pjsua_call_vid_strm_op_param param = { 0 };
    pjsua_call_vid_strm_op_param_default(&param);
    pjsua_cap_dev_view view_config = { 0 };
    view_config.screen_index = 0;
    view_config.app_pid = appId;
    view_config.win_screen_capture_using_dd = mMeetingFeatures.mWinScreenShareDD;
    view_config.mac_screen_capture_deferred = mMeetingFeatures.mMacDeferredScreenCapture;
    view_config.win_capture_using_dd_complete = mMeetingFeatures.mWinDDCaptureComplete;
    view_config.win_app_capture_global_hooks = mMeetingFeatures.mWinAppShareGlobalHooks;
    view_config.mac_screen_capture_delta = mMeetingFeatures.mMacDeltaScreenCapture;
    // content share capture device
    param.cap_dev = content_idx;
    param.med_idx = CONTENT_MED_IDX;
    param.data = static_cast<void*>(&view_config);
    pjsua_call_set_vid_strm(call_id_, PJSUA_CALL_VID_SCREEN_SHARE_SET_VIEW, &param);
}

bool bjn_sky::skinnySipManager::platformOnCallMediaEvent(
    pjsua_call_id call_id,
    unsigned med_idx,
    pjmedia_event *event)
{
    if (call_id != call_id_) // it *should* match...
        return false;

    switch (event->type)
    {
    case PJMEDIA_EVENT_AUDIO:
        {
            const int mic_animation_delay_ms = 250;
            switch (event->aud_subtype)
            {
            case PJMEDIA_AUD_EVENT_ESM_NOTIFY:
                postToClient(MSG_SEND_ESM_STATUS, event->data.esm_notify);
                return true;
            case PJMEDIA_AUD_EVENT_ECBS_NOTIFY:
                postToClient(MSG_SEND_ECBS_STATUS, event->data.ecbs_notify);
                return true;
            case PJMEDIA_AUD_EVENT_MIC_ANIMATION:
                browser_thread_->Clear(browser_msg_hndler_, APP_MSG_MIC_ANIMATION_STATE);
                browser_thread_->PostDelayed(mic_animation_delay_ms,
                        browser_msg_hndler_,
                        APP_MSG_MIC_ANIMATION_STATE,
                        new talk_base::TypedMessageData<int>(event->data.aud_mic_animation.mic_state));
                return true;
            case PJMEDIA_AUD_EVENT_CONFIG_CHANGE: {
                AudioConfigOptions *options =
                    static_cast<AudioConfigOptions*>(event->data.ptr);
                postSetAudioConfigOptions(*options);
                return true;
            }
            default:
                break;
            }
        }
    default:
        break;
    }

    // not handled
    return false;
}

void bjn_sky::skinnySipManager::platformAddSdpAttributes(pjsua_call_id call_id,
                                                         pjmedia_sdp_session *sdp,
                                                         pj_pool_t *pool)
{
    // currently RDC is a Mac and Windows only feature, and it need to be feature enabled
#if defined(FB_WIN) || defined(FB_MACOSX)
    if (mMeetingFeatures.mRemoteDesktopControl)
    {
        pjmedia_sdp_attr *a;
        a = pjmedia_sdp_attr_create(pool, "X-RDC:version=1.0; controller=true; controllee=true", NULL);
        pjmedia_sdp_attr_add(&sdp->attr_count, sdp->attr, a);
    }
#endif
}

bool bjn_sky::skinnySipManager::getDefaultDeviceId(
    bool capture,
    std::string &name,
    std::string &id)
{
    bool ret = false;
    unsigned count = MAX_AUDIO_DEVS;
    pjmedia_aud_dev_info info[MAX_AUDIO_DEVS];
    pjsua_enum_aud_devs(info, &count);
    for(unsigned i = 0; i < count ; i++) {
        if((capture && info[i].input_count) ||
          (!capture && info[i].output_count)) {
            name = info[i].name;
            id = info[i].driver;
            ret = true;
            break;
        }
    }
    return ret;
}

// Check whether the given microphone ID uses USB bus
// and has a matching speaker.
// Return true if both conditions are true.
bool bjn_sky::skinnySipManager::isMicUSBPair(std::string deviceId,
                                             const std::string& classId)
{
    std::string matchingDeviceName;
    std::string matchingDeviceId;
    std::string deviceTypePrefix = DeviceTypeString(DeviceConfigTypeMic) + ":";

    // Remove device type prefix from the start of device key
    if(deviceId.compare(0, deviceTypePrefix.length(), deviceTypePrefix) == 0)
    {
        deviceId = deviceId.substr(deviceTypePrefix.length());
    }

    return (classId == "USB" &&
            getMatchingDevice(deviceId,
                              DeviceConfigTypeSpeaker,
                              audioPlayoutDevices_,
                              matchingDeviceName,
                              matchingDeviceId));
}

bool bjn_sky::skinnySipManager::getPreferredDeviceId(
    bool capture,
    std::string &selectedDeviceName,
    std::string &selectedDeviceId)
{
    bool isSelectedDeviceUSBPair = false;
    bool isCandidateDeviceUSBPair = false;
    bool isSelectedDeviceBlacklisted = true; // worst possible assumption
    bool isCandidateDeviceBlacklisted = true;
    bool preferMatchingUSBDevice = false; // During Mic selection, we prefer a
                                          // USB mic having a matching speaker
    std::string deviceIdKey;
    std::string vendorId;
    std::string productId;
    std::string classId;
    std::vector<Device>* deviceList;
    std::vector<Device>::iterator it;
    std::vector<Device>::iterator selectedDevice;
    DeviceConfigType deviceConfigType;
    bool ret = false;

    if(capture)
    {
        deviceList = &audioCaptureDevices_;
        preferMatchingUSBDevice = true;
        deviceConfigType = DeviceConfigTypeMic;
    }
    else
    {
        deviceList = &audioPlayoutDevices_;
        deviceConfigType = DeviceConfigTypeSpeaker;
    }

    if(deviceList->size() == 0)
    {
        LOG(LS_ERROR) << "No "<< (capture ? "audio input" : "audio output")
                        << "devices to choose from! Aborting " << __FUNCTION__;
        return false;
    }

    ret = GetAudioHardwareIdFromDeviceId(deviceConfigType,
                                         selectedDeviceName,
                                         selectedDeviceId,
                                         vendorId,
                                         productId,
                                         classId,
                                         deviceIdKey,
                                         true);

    if(ret)
    {
        isSelectedDeviceBlacklisted =
            isAudioDeviceBlacklistedForAutomaticSelection(deviceConfigType,
                                                          selectedDeviceId,
                                                          productId,
                                                          vendorId,
                                                          classId,
                                                          deviceIdKey,
                                                          *cph_instance);
        // With device type prepended
        if (preferMatchingUSBDevice)
        {
            if(mMeetingFeatures.mDepreferUsbOnThunderbolt &&
               isUsbDeviceInThnMap(selectedDeviceId))
            {
                LOG(LS_INFO) << "USB device is on Thunderbolt. Hence, not"
                " treating as USB pair";
                isSelectedDeviceUSBPair = false;
            }
            else
            {
                isSelectedDeviceUSBPair = isMicUSBPair(deviceIdKey, classId);
            }
        }

        LOG(LS_INFO) << __FUNCTION__ <<". Default device=" << deviceIdKey
                     << ", isDefaultDeviceBlacklisted=" << isSelectedDeviceBlacklisted
                     << ", isDefaultDeviceUSBPair=" << isSelectedDeviceUSBPair;
    }
    // See if there is a better device than default
    // Iterate through all devices.
    // For speakers :
    //     - switch to the first not blacklisted device we find, and break
    // For mic:
    //     - If at any iteration, switch to new device if:
    //        - we would be moving from blacklisted to nonblacklisted device
    //        - we would be moving from non-USB pair to USB pair, but
    //          not from nonblacklisted to blacklisted device.

    for(it = deviceList->begin(), selectedDevice = deviceList->end(); it != deviceList->end(); it++)
    {
        // Skip automatic device entry
        if(it->id == AutomaticDeviceGuid)
            continue;

        if ((!preferMatchingUSBDevice || isSelectedDeviceUSBPair)
            && !isSelectedDeviceBlacklisted)
        {
            // The selected device is not blacklisted.
            // Also, if this is a mic, it uses USB and has a matching speaker
            // This is already the best combination we can find!
            // no need to check further
            break;
        }

        ret = GetAudioHardwareIdFromDeviceId(deviceConfigType,
                                             it->name,
                                             it->id,
                                             vendorId,
                                             productId,
                                             classId,
                                             deviceIdKey,
                                             true);

        if(ret)
        {
            isCandidateDeviceBlacklisted =
                isAudioDeviceBlacklistedForAutomaticSelection(deviceConfigType,
                                                              it->id,
                                                              productId,
                                                              vendorId,
                                                              classId,
                                                              deviceIdKey,
                                                              *cph_instance);

            if (preferMatchingUSBDevice)
            {
                if(mMeetingFeatures.mDepreferUsbOnThunderbolt &&
                   isUsbDeviceInThnMap(it->id))
                {
                    LOG(LS_INFO) << "USB device is on Thunderbolt. Hence, not"
                    " treating as USB pair";
                    isCandidateDeviceUSBPair = false;
                }
                else
                {
                    isCandidateDeviceUSBPair =
                        isMicUSBPair(deviceIdKey, classId);
                }
            }

            LOG(LS_INFO) << "Checking device=" << deviceIdKey
            << ", isCandidateDeviceBlacklisted=" << isCandidateDeviceBlacklisted
            << ", isCandidateDeviceUSBPair=" << isCandidateDeviceUSBPair;

            if(!isSelectedDeviceBlacklisted && isCandidateDeviceBlacklisted)
            {
                continue; //Never move from nonblacklisted to blacklisted device
            }
            else if((isSelectedDeviceBlacklisted && !isCandidateDeviceBlacklisted) ||
                    (preferMatchingUSBDevice && !isSelectedDeviceUSBPair && isCandidateDeviceUSBPair))
            {
                selectedDevice = it;
                isSelectedDeviceBlacklisted = isCandidateDeviceBlacklisted;
                isSelectedDeviceUSBPair = isCandidateDeviceUSBPair;
                LOG(LS_INFO) << "Switched to above device";
            }
        }
    }

    if (selectedDevice != deviceList->end())
    {
        // We found a better device than the default in the loop above.
        // and updated the selectedDeviceIndex.
        // Now update the selectedDevice name and id as well.
        selectedDeviceName = selectedDevice->name;
        selectedDeviceId = selectedDevice->id;
    }
	else {
		LOG(LS_INFO) << __FUNCTION__ << ":No device selected!";
	}

    LOG(LS_INFO) << __FUNCTION__ << " : For capture=" << capture
                 << ", Chosen device is=" <<    selectedDeviceName
                 << "(" << selectedDeviceId << "), isBlacklisted="
                 << isSelectedDeviceBlacklisted << ", isUSBPair="
                 << isSelectedDeviceUSBPair;
    return true;
}

bool bjn_sky::skinnySipManager::getAutomaticMic(std::string &micId, std::string &micName, bool bPostMicCheck /*= true*/)
{
    bool automatic_mic = false;
    std::string vendorId;
    std::string productId;
    std::string classId;
    std::string deviceId;

#ifdef WIN32
    device_manager_->GetDefaultAudioInputDevices();
    cricket::Device defaultCommDevice = ((cricket::DeviceManager *) device_manager_)->getDefaultCapCommDevice();
    micId = defaultCommDevice.id;
    micName = defaultCommDevice.name;
    if(isDeviceBuiltIn(audioCaptureDevices_, micId)) {
        automatic_mic = true;
    }
    //TODO: Should we add the following logic: When mBiasedAudioDeviceAutoSelection is enabled, choose the defaultDevice instead of DefaultCommDevice, if DefaultCommDevice is blacklisted but defaultDevice is not. Do not enable microphone level detection in this case.
    LOG(LS_INFO) << "Setting automatic microphone to device: "
                    << defaultCommDevice.name << " " << defaultCommDevice.id;
    if(automatic_mic) {
        pjsua_snd_get_setting(PJMEDIA_AUD_DEV_CAP_INPUT_SIGNAL_METER, &mic_lvl_);
        level_determined_ = 0;
        auto_level_check_count_ = 20;
        LOG(LS_INFO) << "Selected microphone is Automatic Device. Starting checking level";
        if (bPostMicCheck) {
            client_thread_->PostDelayed(100, this, MSG_AUTO_MIC_LEVEL_CHECK);
        }
    }
#else
    getDefaultDeviceId(true, micName, micId);
#endif

    if(mMeetingFeatures.mBiasedAudioDeviceAutoSelection)
    {
        LOG(LS_INFO) << "mBiasedAudioDeviceAutoSelection enabled";
        getPreferredDeviceId(true, micName, micId);
#ifdef WIN32
        if(automatic_mic && !isDeviceBuiltIn(audioCaptureDevices_, micId)) {
            //new selected device is not built-in. Clear mic level check msg
            if (bPostMicCheck) {
                client_thread_->Clear(this, MSG_AUTO_MIC_LEVEL_CHECK);
            }
        }
#endif
    }
    else
    {
        //If system default device is USB for both audio in/out then use it other wise first external usb device having both audio in/out.
        if(GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic, micName, micId,
                                          vendorId, productId, classId, deviceId, false) && classId == "USB") {
            if(!(mMeetingFeatures.mDepreferUsbOnThunderbolt && isUsbDeviceInThnMap(micId))
               && getMatchingDevice(deviceId, DeviceConfigTypeSpeaker, audioPlayoutDevices_, vendorId, deviceId)) {
                if (bPostMicCheck) {
                    client_thread_->Clear(this, MSG_AUTO_MIC_LEVEL_CHECK);
                }
                return true;
            }
        }
        for(unsigned i = 0; i < audioCaptureDevices_.size(); i++) {
            if(GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic, audioCaptureDevices_[i].name, audioCaptureDevices_[i].id,
                                              vendorId, productId, classId, deviceId, false) && classId == "USB") {
                if(!(mMeetingFeatures.mDepreferUsbOnThunderbolt && isUsbDeviceInThnMap(audioCaptureDevices_[i].id)) &&
                   getMatchingDevice(deviceId, DeviceConfigTypeSpeaker, audioPlayoutDevices_, vendorId, deviceId)) {
                    micName = audioCaptureDevices_[i].name;
                    micId = audioCaptureDevices_[i].id;
                    if (bPostMicCheck) {
                        client_thread_->Clear(this, MSG_AUTO_MIC_LEVEL_CHECK);
                    }
                    mAutoSpkMic.mMicId = micId;
                    mAutoSpkMic.mMicName = micName;
                    return true;
                }
            }
        }
    }
    mAutoSpkMic.mMicId = micId;
    mAutoSpkMic.mMicName = micName;
    mAutoSpkMic.mAutoMic = automatic_mic;
    return true;
}

bool bjn_sky::skinnySipManager::getAutomaticSpk(std::string &spkId, std::string &spkName)
{
#ifdef WIN32
    device_manager_->GetDefaultAudioOutputDevices();
    cricket::Device defaultCommDevice = ((cricket::DeviceManager *) device_manager_)->getDefaultPlayCommDevice();
    spkId = defaultCommDevice.id;
    spkName = defaultCommDevice.name;
    LOG(LS_INFO) << "Setting automatic speakers to default device: "
                 << defaultCommDevice.name << " " << defaultCommDevice.id;
#else
    getDefaultDeviceId(false, spkName, spkId);
#endif

    if(mMeetingFeatures.mBiasedAudioDeviceAutoSelection)
    {
        LOG(LS_INFO) << "mBiasedAudioDeviceAutoSelection enabled";
        getPreferredDeviceId(false, spkName, spkId);
    }
    mAutoSpkMic.mSpeakerId = spkId;
    mAutoSpkMic.mSpeakerName = spkName;
    return true;
}

bool bjn_sky::skinnySipManager::getMatchingDevice(
    std::string deviceId,
    DeviceConfigType devType,
    std::vector<Device> &fromDevices,
    std::string &name,
    std::string &id)
{
    std::string vendorId;
    std::string productId;
    std::string classId;
    std::string matchDeviceId;
    bool found = false;
    unsigned i;

    for(i = 0; i < fromDevices.size(); i++) {
        //TODO: builtIn is always true for Limux.
        if(!fromDevices[i].builtIn &&
           GetAudioHardwareIdFromDeviceId(devType, fromDevices[i].name, fromDevices[i].id,
                                          vendorId, productId, classId, matchDeviceId, false) &&
           matchDeviceId == deviceId) {
            name = fromDevices[i].name;
            id = fromDevices[i].id;
            LOG(LS_INFO) << "Macthing device name: " << fromDevices[i].name << " id:" << fromDevices[i].id;
            LOG(LS_INFO) << "Matching device id: " << matchDeviceId;
            found = true;
            break;
        }
    }
    return found;
}

NetworkInfo bjn_sky::skinnySipManager::platformGetNetworkInfo()
{
    return BJN::platformGetNetworkInfo();
}
void bjn_sky::skinnySipManager::updateVideoDevice()
{
    pjsua_vid_win_id local_view_rend_wid = pjsua_vid_preview_get_win(cap_idx_);
    pjmedia_vid_dev_refresh();
    if(FindVideoDeviceIdx(selectedDeviceoptions_.get_CameraID().c_str(),
                            selectedDeviceoptions_.get_Camera().c_str(),
                            &cap_idx_,
                            PJMEDIA_DIR_CAPTURE)) {
        if (local_view_rend_wid != PJSUA_INVALID_ID) {
            pjmedia_vid_dev_update_cap_id(local_view_rend_wid, cap_idx_);
        }
    } else {
        enableMedTx(false, VIDEO_MED_IDX);
        setCaptureDevOff();
        cap_idx_ = PJMEDIA_VID_INVALID_DEV;
    }
    LOG(LS_INFO) << "Updating capture device index to " << cap_idx_;
    pjsua_vid_channel_dev_init(call_id_, VIDEO_MED_IDX,
                                cap_idx_,
                                rend_local_idx_,
                                rend_idx_);
}

void bjn_sky::skinnySipManager::postMakeCallInternal()
{
    browser_thread_->Post(browser_msg_hndler_, APP_MSG_MAKE_CALL_INTERNAL);
}


void bjn_sky::skinnySipManager::postSetFocusApp(uint32_t app_id)
{
    if(client_thread_) {
         client_thread_->Post(this, MSG_SET_FOCUS_APP, new talk_base::TypedMessageData<uint32_t>(app_id));
    }
}

void bjn_sky::skinnySipManager::setFocusApp(uint32_t app_id)
{
    talk_base::ApplicationDescriptionList appList;
    window_picker_->GetApplicationList(&appList);
    for (size_t i = 0; i < appList.size(); i++)
    {
        talk_base::ApplicationDescription desc = appList[i];
        if(desc.id() == app_id)
        {
            talk_base::WindowDescriptionList windowList = desc.get_windows();
            for (size_t j = 0; j < windowList.size(); j++)
            {
                talk_base::WindowDescription wdesc = windowList[j];
                window_picker_->MoveToFront(wdesc.id());
            }
        }
    }
}

std::string bjn_sky::skinnySipManager::getPlatformGuid()
{
    return BJN::getGuid();
}

void bjn_sky::skinnySipManager::resetSavedSpeakerState() {
    spk_mute_ = false;
    spk_vol_ = DEFAULT_SOUND_VOLUME;
    spk_lvl_ = DEFAULT_SOUND_LEVEL;
    spk_decay_timer_ = DEFAULT_SOUND_DECAY_TIMER;
}

void bjn_sky::skinnySipManager::resetSavedMicrophoneState() {
    mic_mute_ = false;
    mic_vol_ = DEFAULT_SOUND_VOLUME;
    mic_lvl_ = DEFAULT_SOUND_LEVEL;
    mic_decay_timer_ = DEFAULT_SOUND_DECAY_TIMER;
}

void bjn_sky::skinnySipManager::updateAutomaticDeviceLabel(
    AudioDeviceSelection micSelection,
    AudioDeviceSelection spkrSelection,
    const std::string& micId,
    const std::string& micName,
    const std::string& spkId,
    const std::string& spkName)
{
    /*
       Automatic lable name logic is slightly complicated because we need to find out and publish the auto label by falsely attempting the
       current selected logic as auto! Good luck!
       Automatic device name label logic is broken into 2 phases:
          1) Auto labeling mechanism
          2) Algorithm to select the appropriate labeling mechanisms
       AUTO LABELING MECHANISM:
          a) Default - Use already existing laber i.e. passed from MSG_INIT_MEDIA
          b) Auto - Use output from getAutomaticMic or getAutomaticSpk API
          c) Matching - Mic: For selecting auto mic name, use currently selected Speaker name
                      - Speaker: For selecting auto speaker name , use currently selected Mic name
          d) SpeakerMatchingAutoMic - Select speaker name based upon auto mic name(i.e. output from getAutomaticMic)
                                    - if none matches then use auto mechanism
          e) MicMatchingAutoSpeaker - Select mic name based upon auto speaker name(i.e. output from getAutomaticSpk)
                                    - if none matches then use auto mechanism
       AUTO NAME SELECTION ALGORITHM:
          - Speaker and mic selection are categroised as auto - manual. If pairing or auto is used then selection is treated as auto
          ALGORITHM:
          a) if both mic and speaker selection are auto then use Default for both
          b) if both mic and speaker are manual then use Matching for both
          c) if mic is auto and speaker is manual then use Default for mic and SpeakerMatchingAutoMic for speaker
          d) if mic is manual and speaker is auto then use MicMatchingAutoSpeaker for mic and Default for speaker
          e) if all other cases use Auto for both mic and speaker
        Notes:
          - For efficiency mAutoSpkMic variable is introduced. So if auto logic was already called inside MSG_INIT_MEDIA then it is not called again
          - if auto logic is called, then there is a timer set inside to avoid that getAutomaticMic is called with last param false i.e. avoid built in mic check!
       */

    LOG(LS_INFO) << "updateAutomaticDeviceLabel inputs: mic selection:"
        << ((AUDIO_DEVICE_SELECT_AUTO == micSelection) ? "Auto" : ((AUDIO_DEVICE_SELECT_PAIRED == micSelection) ? "Paired" : "Manual"))
        << ";mic id: " << micId << ";mic name:" << micName  << ";speaker selection:"
        << ((AUDIO_DEVICE_SELECT_AUTO == spkrSelection) ? "Auto" : ((AUDIO_DEVICE_SELECT_PAIRED == spkrSelection) ? "Paired" : "Manual"))
        << ";spkr id:" << spkId << ";spkr name:" << spkName;

    if (selectedDeviceoptions_.get_MicrophoneID().empty() &&
        selectedDeviceoptions_.get_SpeakerID().empty()) {
        LOG(LS_INFO) << "No audio device is available in the system, so no automatic label processing is required";
        return;
    }

    // Both auto and pairing mechanism are treated as auto! otherwise manual
    bool bMicSelectionAuto = (AUDIO_DEVICE_SELECT_AUTO == micSelection || AUDIO_DEVICE_SELECT_PAIRED == micSelection) ? true : false;
    bool bSpkrSelectionAuto = (AUDIO_DEVICE_SELECT_AUTO == spkrSelection || AUDIO_DEVICE_SELECT_PAIRED == spkrSelection) ? true : false;

    // Matching mechanisms defined here
    enum InitType { Default, Auto, Matching, SpeakerMatchingAutoMic, MicMatchingAutoSpeaker };
    InitType micInitType = Default;
    InitType speakerInitType = Default;

    std::string sAutoMicId, sAutoMicName;
    std::string sAutoSpeakerId, sAutoSpeakerName;
    if (bMicSelectionAuto && bSpkrSelectionAuto) {
        // Both spkr and mic selection were automatic so use already initialised mic and speaker types
        micInitType = Default;
        speakerInitType = Default;
    }
    else if (!bMicSelectionAuto && !bSpkrSelectionAuto) {
        // Both are manual, so use Matching mechanism
        // speaker should match mic
        // mic should match speaker
        micInitType = speakerInitType = Matching;
    }
    else if (bMicSelectionAuto && !bSpkrSelectionAuto) {
        // For mic use default
        micInitType = Default;
        // For speaker match it against the auto mic
        speakerInitType = SpeakerMatchingAutoMic;
    }
    else if (!bMicSelectionAuto && bSpkrSelectionAuto) {
        // For mic , select auto mic and match it against speaker
        micInitType = MicMatchingAutoSpeaker;
        // For speaker use default
        speakerInitType = Default;
    }
    else {
        // This should be a bug! But fall to Auto
        micInitType = Auto;
        speakerInitType = Auto;
    }

    if (Default == micInitType) {
        sAutoMicId = micId;
        sAutoMicName = micName;
    }

    if (Default == speakerInitType) {
        sAutoSpeakerId = spkId;
        sAutoSpeakerName = spkName;
    }

    if (Matching == micInitType) {
        // Try to match with current selected speaker
        std::string vendorId;
        std::string productId;
        std::string classId;
        std::string deviceId;

        bool result = GetAudioHardwareIdFromDeviceId(DeviceConfigTypeSpeaker, spkName, spkId, vendorId, productId, classId, deviceId, false);

        if (!result || !getMatchingDevice(deviceId, DeviceConfigTypeMic, audioCaptureDevices_, sAutoMicName, sAutoMicId)) {
            micInitType = Auto;
        }
    }

    if (MicMatchingAutoSpeaker == micInitType) {
        // Match with auto speaker
        std::string vendorId;
        std::string productId;
        std::string classId;
        std::string deviceId;
        std::string localSpkId;
        std::string localSpkName;
        if (!mAutoSpkMic.isMicValid()) {
            getAutomaticMic(sAutoMicId, sAutoMicName, false);
        }
        else {
            sAutoMicId = mAutoSpkMic.mMicId;
            sAutoMicName = mAutoSpkMic.mMicName;
        }
        bool result = GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic, sAutoMicName, sAutoMicId, vendorId, productId, classId, deviceId, false);

        if (!result || !getMatchingDevice(deviceId, DeviceConfigTypeSpeaker, audioPlayoutDevices_, localSpkName, localSpkId)) {
            getAutomaticSpk(localSpkId, localSpkName);
            if (GetAudioHardwareIdFromDeviceId(DeviceConfigTypeSpeaker, spkName, spkId, vendorId, productId, classId, deviceId, false))
            {
                getMatchingDevice(deviceId, DeviceConfigTypeMic, audioCaptureDevices_, sAutoMicName, sAutoMicId);
            }
            // No else handling because auto mechanism is already initialised
        }
        // No else handling because auto mechanism is already initialised
    }

    // Select speaker matching auto mic
    if (SpeakerMatchingAutoMic == speakerInitType) {
        std::string sLocalAutoMicId, sLocalAutoMicName;
        if (!mAutoSpkMic.isMicValid()) {
            getAutomaticMic(sLocalAutoMicId, sLocalAutoMicName, false);
        }
        else {
            sLocalAutoMicId = mAutoSpkMic.mMicId;
            sLocalAutoMicName = mAutoSpkMic.mMicName;
        }

        std::string vendorId;
        std::string productId;
        std::string classId;
        std::string deviceId;

        bool result = GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic, sLocalAutoMicName, sLocalAutoMicId, vendorId, productId, classId, deviceId, false);

        if (!result || !getMatchingDevice(deviceId, DeviceConfigTypeSpeaker, audioPlayoutDevices_, sAutoSpeakerName, sAutoSpeakerId)) {
            // No matching speaker found so use Auto matching mechanism
            speakerInitType = Auto;
        }
    }

    if (Matching == speakerInitType) {
        // Matching with current selected mic
        std::string vendorId;
        std::string productId;
        std::string classId;
        std::string deviceId;

        bool result = GetAudioHardwareIdFromDeviceId(DeviceConfigTypeMic, micName, micId, vendorId, productId, classId, deviceId, false);

        if (!result || !getMatchingDevice(deviceId, DeviceConfigTypeSpeaker, audioPlayoutDevices_, sAutoSpeakerName, sAutoSpeakerId)) {
            // No matching device found, select default auto
            speakerInitType = Auto;
        }
    }

    if (Auto == micInitType) {
        // Automatic mic selection
        if (!mAutoSpkMic.isMicValid()) {
            getAutomaticMic(sAutoMicId, sAutoMicName, false);
        }
        else {
            sAutoMicId = mAutoSpkMic.mMicId;
            sAutoMicName = mAutoSpkMic.mMicName;
        }
    }

    if (Auto == speakerInitType) {
        // Automatic speaker selection
        if (!mAutoSpkMic.isSpkValid()) {
            getAutomaticSpk(sAutoSpeakerId, sAutoSpeakerName);
        }
        else {
            sAutoSpeakerId = mAutoSpkMic.mSpeakerId;
            sAutoSpeakerName = mAutoSpkMic.mSpeakerName;
        }
    }
    
#if defined FB_MACOSX
    // For Mac OS X "default" keyword is appened to auto mic and speaker name. This needs to be filtered.
    sAutoMicName = FixDefaultDeviceId(sAutoMicName);
    sAutoSpeakerName = FixDefaultDeviceId(sAutoSpeakerName);
#endif

    LOG(LS_INFO) << "Current selected device names, mic id:" << selectedDeviceoptions_.get_MicrophoneID() << ";mic name: " << selectedDeviceoptions_.get_Microphone()
        << ";spkr id:" << selectedDeviceoptions_.get_SpeakerID() << ";spkr name:" << selectedDeviceoptions_.get_Speaker();

    unsigned automaticMicIndex = 0;
    unsigned automaticSpkIndex = 0;
    ChangeDevInfoList updatedDevicesList;
    ChangeDevInfo changedDev;
    if (audioCaptureDevices_.size() > 0 &&
        audioCaptureDevices_[automaticMicIndex].name != sAutoMicName)
    {
        audioCaptureDevices_[automaticMicIndex].name = sAutoMicName;
        changedDev.devAction = UPDATED;
        changedDev.devGuid = audioCaptureDevices_[automaticMicIndex].id;
        changedDev.devName = audioCaptureDevices_[automaticMicIndex].name;
        changedDev.devType = AUDIO_IN;
        updatedDevicesList.push_back(changedDev);

        LOG(LS_INFO) << "Automatic mic id:" << changedDev.devGuid << ";mic name: " << changedDev.devName;
    }

    if (audioPlayoutDevices_.size() > 0 &&
        audioPlayoutDevices_[automaticSpkIndex].name != sAutoSpeakerName)
    {
        audioPlayoutDevices_[automaticSpkIndex].name = sAutoSpeakerName;
        changedDev.devAction = UPDATED;
        changedDev.devGuid = audioPlayoutDevices_[automaticSpkIndex].id;
        changedDev.devName = audioPlayoutDevices_[automaticSpkIndex].name;
        changedDev.devType = AUDIO_OUT;
        updatedDevicesList.push_back(changedDev);

        LOG(LS_INFO) << "Automatic spkr id:" << changedDev.devGuid << ";spkr name:" << changedDev.devName;
    }

    if (updatedDevicesList.size() > 0)
    {
        browser_thread_->Post(browser_msg_hndler_,
            APP_MSG_NOTIFY_LOCAL_DEVICES_CHANGE,
            new talk_base::TypedMessageData < ChangeDevInfoList >
            (updatedDevicesList));
    }
}

void bjn_sky::skinnySipManager::enableAudioEngine(bool enable)
{
    if(!mMeetingFeatures.mEnableDisableAudioEngine)
    {
        LOG(LS_INFO) << "Ignoring enableAudioEngine() call."
            << "\"enable_disable_audio_engine\" feature is disabled.";
        return;
    }

    if(mAudioEngineEnabled == enable)
    {
        LOG(LS_INFO) << "Audio Engine already "
            << (enable ? "enabled" : "disabled");
        return;
    }

    if(enable)
    {
        if(!pjsua_snd_is_active())
        {
            bool status = false;
            platformPreDeviceOperation();
            status = pjsua_set_snd_dev(mMicIdx, mSpkIdx);
            if(status == PJ_SUCCESS &&
               pjsua_snd_is_active()) // If we've opened a non-null device
            {
                platformPostDeviceOperation();
                // Note: We allow rivet to call enableMediaStreams
                // and begin sending/receiving audio, when it sees fit.
            }
            LOG(LS_INFO) << "Enabled Audio Engine";
        }
        else
        {
            LOG(LS_WARNING) << "Audio Engine was already active";
        }

        mAudioEngineEnabled = true;
    }
    else
    {
        // Audio Engine should be disabled before audio direction has
        // been set to inactive. If this is not the case, then it is
        // a bug and should be fixed. Still, if this situation arises
        // log a warning, and set audio direction to inactive now.
        if(mAudioDirection != ATTR_DIR_INACTIVE) {
            LOG(LS_WARNING) << "Audio engine disabled, when audio "
                "direction is not inactive. Setting audio direction "
                "to inactive to avoid AI underruns.";
            MediaStreamsDirection mediaDir = getCurrentMediaDirection();
            mediaDir.audioSend = false;
            mediaDir.audioRecv = false;
            callMediaDirection(mediaDir);
        }

        if(pjsua_snd_is_active()) {
            // Pause current webrtc stream, save the RTP sequence number
            // and time stamp, then destroy the webrtc engine by opening
            // NULL device.
            pjsua_pause_snd_dev();
            pjsua_save_seq_num_ts(call_id_, AUDIO_MED_IDX);
            pjsua_set_null_snd_dev();

            LOG(LS_INFO) << "Disabled Audio Engine";

            // TODO(Suyash): When Audio Engine is disabled, we need to ignore
            // any changes to audioDirection. Tracked by SKY-5305.
        }
        else {
            // This might not be an error if user had no devices connected.
            // Still log a warning.
            LOG(LS_WARNING) << "Audio Engine was already inactive";
        }

        mAudioEngineEnabled = false;
    }
    sendAudioEngineState();
}

void bjn_sky::skinnySipManager::sendAudioEngineState()
{
    if(call_id_ != PJSUA_INVALID_ID)
    {
        std::stringstream buffer;
        std::string audioEngineState = mAudioEngineEnabled ? "enabled"
            : "disabled";
        LOG(LS_INFO) << "Send info message for audio engine state: "
            << audioEngineState;

        buffer << INFO_XML_AUDIO_ENGINE_STATE << " value='"
            << audioEngineState << "'";
        sendSystemInfoMessage(call_id_, buffer.str());
    }
}

void bjn_sky::skinnySipManager::updateUsbOnThunderboltMap(const std::string& devName,
                                                          const std::string& devGuid,
                                                          bool added,
                                                          bool capture) {
    // TODO (Suyash): Fix the case when multiple devices with same GUID are connected.
    DeviceConfigType devConfigType = capture ? DeviceConfigTypeMic :
        DeviceConfigTypeSpeaker;
    std::map<std::string, bool>::iterator it = usbOnThnMap.find(devGuid);

    if(added) {
        if(it == usbOnThnMap.end()) {
            bool isOnThn = false;
            std::string classId;
            std::string vendorId;
            std::string productId;
            std::string deviceIdKey;

            if(GetAudioHardwareIdFromDeviceId(devConfigType,
                                              devName,
                                              devGuid,
                                              vendorId,
                                              productId,
                                              classId,
                                              deviceIdKey,
                                              false)) {
                isOnThn = isUSBDeviceOnThunderbolt(classId, vendorId, productId);
            }
            LOG(LS_INFO) << "Adding devGuid \"" << devGuid
                << "\" to USBOnThunderboltMap, with value "
                << (isOnThn ? "True" : "False");
            usbOnThnMap.insert(std::pair<std::string, bool>(devGuid, isOnThn));
        }
        else {
            LOG(LS_INFO) << "devGuid \"" << devGuid
                << "\" already part of USBOnThunderboltMap";
        }
    }
    else {
        if(it != usbOnThnMap.end()) {
            LOG(LS_INFO) << "Removing \"" << devGuid << "\" from USBOnThunderboltMap";
            usbOnThnMap.erase(it);
        }
    }
}

void bjn_sky::skinnySipManager::updateUsbOnThunderboltMap(const std::vector<Device> &devList,
                                                           bool added,
                                                           bool capture)
{
    std::vector<Device>::const_iterator dev;
    for(dev = devList.begin();
        dev != devList.end();
        dev++)
    {
        updateUsbOnThunderboltMap(dev->name, dev->id, added, capture);
    }
}


bool bjn_sky::skinnySipManager::isUsbDeviceInThnMap(const std::string& devGuid) const
{
    std::string fixedDevGuid;
#ifdef FB_MACOSX
    fixedDevGuid = FixDefaultDeviceId(devGuid);
#else
    fixedDevGuid = devGuid;
#endif
    std::map<std::string, bool>::const_iterator it = usbOnThnMap.find(fixedDevGuid);
    if(it != usbOnThnMap.end())
       return it->second;
    else
       return false;
}

