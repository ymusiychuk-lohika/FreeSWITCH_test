//
//  audiodevlistener.cpp
//  Skinny
//
//  Created by Tarun Chawla on 24 March 2016
//  Copyright (c) 2016 Bluejeans. All rights reserved.
//


#include "win32devicemanager.h"
#include "audiodevlistener.h"

#include <future>

namespace cricket {
    struct DeviceChangeInfo {
        // This structure encapsulates the data to be sent
        // to dipatcher thread in MSG_CHANGE_ENDPOINT_GUID_TIMEOUT
        // message.
        bool m_isMic;
        std::string m_guid;

        DeviceChangeInfo(bool isMic, const std::string& guid)
            : m_isMic(isMic)
            , m_guid(guid) {}

        DeviceChangeInfo()
            : m_isMic(false)
            , m_guid("") {}
    };

    struct VolumeUpdateInfo {
        // This structure encapsulates the data to be recevied by
        // calling thread from dispatcher thread after the latter
        // processes the message MSG_GET_VOLUME_STATS_TIMEOUT.
        // This will be wrapped in a promise template object
        // so that it can be returned.
        VolumeStats m_volStats;
        HRESULT m_result;

        VolumeUpdateInfo()
            : m_volStats()
            , m_result(E_FAIL) {}
    };

    // <R> is type of data to be returned by called thread and has to be specified.
    // <S> is type of data to be sent by caller thread, and may be unused.
    template <typename R, typename S = int>
    class PromiseMessageWrapper {
    public:
        std::future<R> getFutureObject() {
            return m_returnedData.get_future();
        }

        void notify(const R& returnData) {
            m_returnedData.set_value(returnData);
        }

        void setSendData(const S& sendData) {
            m_sendData = sendData;
        }

        const S& getSendDataRef() {
            return m_sendData;
        }

        PromiseMessageWrapper()
            : m_returnedData()
            , m_sendData() {}

    private:
        std::promise<R> m_returnedData;
        S               m_sendData;
    };

    AudioDeviceListenerWin::AudioDeviceListenerWin() :
        m_audioEndPointNotification(NULL),
        m_speakerEndPointVolumeNotification(NULL),
        m_micEndPointVolumeNotification(NULL),
        m_deviceWatcher(NULL),
        m_enumerator(NULL),
        m_bNeedCouninitialize(false),
        m_bErrorState(true)
        {
        HRESULT hr = S_OK;
        hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (SUCCEEDED(hr)) {
            m_bNeedCouninitialize = true;
        }
        else{
            LOG(LS_ERROR) << "Core Audio: AudioDeviceListenerWin: com"
                " Initialization failed " << std::hex << hr <<
                "GetLastError = " << GetLastError();
        }

        m_audioEndPointNotification = new(std::nothrow) AudioEndPointNotification;
        if (NULL == m_audioEndPointNotification){
            LOG(LS_ERROR) << "Core Audio: Not able to allocate memory "
                "for audio end point notification client";
        }

        m_speakerEndPointVolumeNotification = new(std::nothrow) SpeakerVolumeNotification;
        if (NULL == m_speakerEndPointVolumeNotification){
            LOG(LS_ERROR) << "Core Audio: Not able to allocate memory "
                "for speaker notification client";
        }
        m_micEndPointVolumeNotification = new(std::nothrow) MicVolumeNotification;
        if (NULL == m_micEndPointVolumeNotification){
            LOG(LS_ERROR) << "Core Audio: Not able to allocate memory "
                "for mic notification client";
        }
        m_dispatcherThread.Start();
    }

    AudioDeviceListenerWin::~AudioDeviceListenerWin(){
        m_dispatcherThread.Quit();
        Terminate();

        m_audioEndPointNotification = NULL;
        m_speakerEndPointVolumeNotification = NULL;
        m_micEndPointVolumeNotification = NULL;

        if (m_bNeedCouninitialize){
            CoUninitialize();
            m_bNeedCouninitialize = false;
        }
    }

    // ----------------------------------------------------------------------
    //  OnDeviceStateChanged()
    //
    //  Based on current audio endpoint state we will notify the Win32DeviceWatcher
    //  to enumerate device for removal or arrival
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::OnDeviceStateChanged(bool arrival) {
        if (NULL == m_deviceWatcher){
            LOG(LS_ERROR) << "Core Audio: Device Watcher pointer is null";
            return E_FAIL;
        }
        //Do not do any task in OS thread
        //If you are plannig to do some thing
        //then do in seperate thread as it can block
        //OS thread
        m_deviceWatcher->notify(arrival);
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  ChangeEndpoint()
    //
    //  This will change the audio endpoint device
    //  it will disconnect the connection from old point
    //  and attach to new default audio end point
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::ChangeEndpoint(const std::string& speakerGUID, const std::string& micGUID){
        return AttachWatcher(m_deviceWatcher, speakerGUID, micGUID);
    }

    // ----------------------------------------------------------------------
    //  AttachWatcher()
    //
    //  This function attach the Win32DeviceWatcher object to this class, watcher
    //  object will be notified for the arrival or removal of audio end point
    //  Any changes in Volume stats will also be notified
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::AttachWatcher(Win32DeviceWatcher *watcher, const std::string& speakerGUID, const std::string& micGUID){
        //Terminate if there is any listener present
        Terminate();
        LOG(LS_INFO) << "Core Audio: AudioDeviceListenerWin::AttachWatcher"
            " speakerGUID = " << speakerGUID.c_str()
            << " micGUID = " << micGUID.c_str();
        m_bErrorState = true;
        m_deviceWatcher = watcher;

        HRESULT hr = E_FAIL;
        do{
            hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_ALL,
                __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&m_enumerator));
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Error while creating IMMDeviceEnumerator ";
                break;
            }

            if (NULL == m_audioEndPointNotification){
                LOG(LS_ERROR) << "Core Audio:  Audio end point pointer is null";
                hr = E_FAIL;
                break;
            }

            hr = m_audioEndPointNotification->AttachDevListener(this, m_enumerator);

            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Not able to attach dev listener"
                    " for end point notification hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }

            if (NULL == m_speakerEndPointVolumeNotification){
                LOG(LS_ERROR) << "Core Audio: Speaker end point notification pointer is null";
                hr = E_FAIL;
                break;
            }

            hr = m_speakerEndPointVolumeNotification->AttachDevListener(this, m_enumerator, speakerGUID);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Not able to attach dev listener"
                    " for speaker volume notification hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }

            if (NULL == m_micEndPointVolumeNotification){
                LOG(LS_ERROR) << "Core Audio: Mic end point notification pointer is null";
                hr = E_FAIL;
                break;
            }

            hr = m_micEndPointVolumeNotification->AttachDevListener(this, m_enumerator, micGUID);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Not able to attach dev listener"
                    " for mic volume notification hr = "
                    << std::hex << hr << " GetLastError = " << GetLastError();
                break;
            }
        } while (0);


        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Not able to attach watcher";
            return hr;
        }
        m_bErrorState = false;
        return hr;
    }

    // ----------------------------------------------------------------------
    //  SetSpeakerGUID()
    //
    //  This function will set the speaker guid which we will be using
    //  it to get the audio device in GetDefaultAudioEndPoint()
    //  If new guid has come then we will initiate end point change
    //  This change of end point wil be asynchronous so non blocking
    //  for calling thread
    // ----------------------------------------------------------------------
    void AudioDeviceListenerWin::PostSetSpeakerGUID(const std::string& guid){
        LOG(LS_INFO) << "Core Audio: AudioDeviceListenerWin::PostSetSpeakerGUID = " << guid.c_str();
        m_dispatcherThread.Post(this, MSG_CHANGE_ENDPOINT_SPEAKER, new talk_base::TypedMessageData<std::string>(guid));
    }

    // ----------------------------------------------------------------------
    //  SetMicGUID()
    //
    //  This function will set the mic guid which we will be using
    //  it to get the audio device in GetDefaultAudioEndPoint()
    //  If new guid has come then we will initiate end point change
    //  This change of end point wil be asynchronous so non blocking
    //  for calling thread
    // ----------------------------------------------------------------------
    void AudioDeviceListenerWin::PostSetMicGUID(const std::string& guid){
        LOG(LS_INFO) << "Core Audio: AudioDeviceListenerWin::PostSetMicGUID = " << guid.c_str();
        m_dispatcherThread.Post(this, MSG_CHANGE_ENDPOINT_MIC, new talk_base::TypedMessageData<std::string>(guid));
    }

    // ----------------------------------------------------------------------
    //  Terminate()
    //  Unregister for all notification we have registered
    // ----------------------------------------------------------------------
    void AudioDeviceListenerWin::Terminate(){
        talk_base::CritScope cs(&m_CS);
        LOG(LS_INFO) << "Core Audio: Terminate";
        if (NULL != m_audioEndPointNotification){
            m_audioEndPointNotification->Terminate();
        }
        if (NULL != m_speakerEndPointVolumeNotification){
            m_speakerEndPointVolumeNotification->Terminate();
        }
        if (NULL != m_micEndPointVolumeNotification){
            m_micEndPointVolumeNotification->Terminate();
        }

        m_enumerator = NULL;
        m_bErrorState = false;

    }

    // ----------------------------------------------------------------------
    //  GetCurrentVolStats()
    //
    //  Get the current volume stats for system mic,system speaker and session
    //  System speaker vol stats corresponds to system speaker volume
    //  Session speaker vol stats corresponds to application
    //  System mic stats correponds to mic volume
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::GetCurrentVolStats(VolumeStats& volStats){
        talk_base::CritScope cs(&m_CS);

        HRESULT hr = E_FAIL;

	//getting session  and system volume stats
        hr = m_speakerEndPointVolumeNotification->GetSpeakerVolStats(m_volStat);
        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Not able to get speaker vol stats hr = "
                << std::hex << hr << " GetLastError = " << GetLastError();
        }

        //getting mic volume stats
        hr = m_micEndPointVolumeNotification->GetMicVolumeStats(m_volStat);
        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Not able to get mic vol stats hr = "
                << std::hex << hr << " GetLastError = " << GetLastError();
        }
        volStats = m_volStat;
        return hr;
    }

    bool AudioDeviceListenerWin::SetSessionVolume(float spkVol){
        talk_base::CritScope cs(&m_CS);
        if (NULL != m_speakerEndPointVolumeNotification){
            return m_speakerEndPointVolumeNotification->SetSessionVolume(spkVol);
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Error while setting speaker session volume";
            return false;
        }
    }

    bool AudioDeviceListenerWin::SetSpeakerEndpointVolume(float spkVol){

        talk_base::CritScope cs(&m_CS);
        if (NULL != m_speakerEndPointVolumeNotification){

            return m_speakerEndPointVolumeNotification->SetSpeakerEndpointVolume(spkVol);
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Error while setting speaker endpoint volume";
            return false;
        }
    }

    bool AudioDeviceListenerWin::SetSpeakerSessionMute(bool bMute) {

        talk_base::CritScope cs(&m_CS);
        if (NULL != m_speakerEndPointVolumeNotification){

            return m_speakerEndPointVolumeNotification->SetSpeakerSessionMute(bMute);
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Error while setting speaker session mute";
            return false;
        }
    }

    bool AudioDeviceListenerWin::SetSpeakerEndpointMute(bool bMute) {

        talk_base::CritScope cs(&m_CS);
        if (NULL != m_speakerEndPointVolumeNotification){
            return m_speakerEndPointVolumeNotification->SetSpeakerEndpointMute(bMute);
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Error while setting speaker endpoint mute";
            return false;
        }
    }
    // ----------------------------------------------------------------------
    //  OnNotifySpeakerVolume()
    //
    //  This is called by the audio core when system volume or
    //  mute state for the endpoint we are monitoring is changed
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::OnNotifySpeakerVolume(const VolumeStats& speakerStats){
        LOG(LS_INFO) << "Core Audio: New System Volume = " << m_volStat.m_systemVolume
            << " New System mute state = " << m_volStat.m_systemMuteState;
        m_volStat.m_systemMuteState = speakerStats.m_systemMuteState;
        m_volStat.m_systemVolume = speakerStats.m_systemVolume;
        m_dispatcherThread.Post(this, MSG_ENDPOINT_VOLUME_CHANGED, new talk_base::TypedMessageData<VolumeStats>(m_volStat));
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  OnNotifyMicVolume()
    //
    //  This is called by the audio core when system volume or
    //  mute state for the endpoint we are monitoring is changed
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::OnNotifyMicVolume(const VolumeStats& speakerStats){
        LOG(LS_INFO) << "Core Audio: New Mic Volume = " << m_volStat.m_micVolume
            << " New Mic mute state = " << m_volStat.m_micMuteState;
        m_volStat.m_micMuteState = speakerStats.m_micMuteState;
        m_volStat.m_micVolume = speakerStats.m_micVolume;
        m_dispatcherThread.Post(this, MSG_ENDPOINT_VOLUME_CHANGED, new talk_base::TypedMessageData<VolumeStats>(m_volStat));
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  OnSimpleVolumeChanged()
    //
    //  This is called by the audio core when session volume or
    //  mute state for the session we are monitoring is changed
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::OnSimpleVolumeChanged(const VolumeStats& sessionVolStats)
    {
        LOG(LS_INFO) << "Core Audio: New Session Volume = " << m_volStat.m_sessionVolume
            << " New Session mute state = " << m_volStat.m_sessionMuteState;
        m_volStat.m_sessionMuteState = sessionVolStats.m_sessionMuteState;
        m_volStat.m_sessionVolume = sessionVolStats.m_sessionVolume;
        m_dispatcherThread.Post(this, MSG_ENDPOINT_VOLUME_CHANGED, new talk_base::TypedMessageData<VolumeStats>(m_volStat));
        return S_OK;
    }

    // ----------------------------------------------------------------------
    //  DisableDucking()
    //
    //  This functions opts out our process out of default system ducking
    //  preference.
    //  http://msdn.microsoft.com/en-us/library/windows/desktop/dd940391.aspx
    // ----------------------------------------------------------------------
    bool AudioDeviceListenerWin::DisableDucking()
    {
        LOG(LS_INFO) << "Core Audio: DisableDucking";
        HRESULT hr = E_FAIL;

        if (NULL == m_deviceWatcher){
            LOG(LS_ERROR) << "Core Audio: Device Watcher pointer is null";
            return false;
        }

        CComPtr<IMMDevice> pEndpoint = NULL;
        CComPtr<IAudioSessionManager2> pSessionManager2 = NULL;
        CComPtr<IAudioSessionControl> pSessionControl = NULL;
        CComPtr<IAudioSessionControl2> pSessionControl2 = NULL;
        CComPtr<IMMDeviceCollection> deviceCollection = NULL;

        LPWSTR pwszID(NULL);
        UINT deviceCount;

        do{
            hr = m_enumerator->EnumAudioEndpoints(eRender, DEVICE_STATE_ACTIVE, &deviceCollection);
            if (FAILED(hr))
            {
                LOG(LS_ERROR) << "Core Audio: Enumeration of Audio endpoints "
                    "failed hr = " << std::hex << hr <<
                    " GetLastError = " << GetLastError();
                break;
            }
            hr = deviceCollection->GetCount(&deviceCount);
            if (FAILED(hr))
            {
                LOG(LS_ERROR) << "Core Audio: Getting number of audio devices "
                    "failed hr = " << std::hex << hr <<
                    " GetLastError = " << GetLastError();
                break;
            }

            for (UINT i = 0; i < deviceCount; i += 1)
            {
                pwszID = NULL;
                pSessionControl2 = NULL;
                pSessionControl = NULL;
                pSessionManager2 = NULL;
                pEndpoint = NULL;

                hr = deviceCollection->Item(i, &pEndpoint);
                if (FAILED(hr))
                {
                    LOG(LS_ERROR) << "Core Audio: Failed to get IMMDevice"
                        " object from IMMDeviceCollection object, index = "
                        << i << "hr = " << std::hex << hr <<
                        " GetLastError = " << GetLastError();
                    continue;
                }
                // Activate session manager.
                hr = pEndpoint->Activate(__uuidof(IAudioSessionManager2),
                    CLSCTX_INPROC_SERVER,
                    NULL,
                    reinterpret_cast<void **>(&pSessionManager2));

                if (FAILED(hr)){
                    LOG(LS_ERROR) << "Core Audio: Activating endpoint failed"
                        "for index = " << i << " hr = " << std::hex << hr <<
                        " GetLastError = " << GetLastError();
                    continue;
                }

                hr = pSessionManager2->GetAudioSessionControl(NULL, 0, &pSessionControl);
                if (FAILED(hr))
                {
                    LOG(LS_ERROR) << "Core Audio: Getting audio session "
                        "control failed for index = " << i << " hr = "
                        << std::hex << hr << " GetLastError = "
                        << GetLastError();
                    continue;
                }

                hr = pSessionControl->QueryInterface(
                    __uuidof(IAudioSessionControl2),
                    (void**)&pSessionControl2);

                if (FAILED(hr))
                {
                    LOG(LS_ERROR) << "Core Audio: Querying audio session "
                        " control interface failed for index = " << i <<
                        "hr = " << std::hex << hr <<
                        " GetLastError = " << GetLastError();
                    continue;
                }

                hr = pSessionControl2->SetDuckingPreference(TRUE);
                if (FAILED(hr))
                {
                    LOG(LS_ERROR) << "Core Audio: Disabling ducking preference"
                        " failed for index = " << i << " hr = " << std::hex << hr
                        << " GetLastError = " << GetLastError();
                    continue;
                }
                LOG(LS_INFO) << "Core Audio: Ducking preference disabled"
                    " for index = " << i;
            }
        } while (0);

        pSessionControl2 = NULL;
        pSessionControl = NULL;
        pSessionManager2 = NULL;
        pEndpoint = NULL;
        deviceCollection = NULL;

        if (FAILED(hr)){
            LOG(LS_ERROR) << "Core Audio: Disabling ducking preference failed"
                " for all render devices hr = " << std::hex << hr <<
                " GetLastError = " << GetLastError();
            return false;
        }
        return true;
    }

    void AudioDeviceListenerWin::HardwareKeyFromHardwareId(const std::string& hardwareId,
        unsigned int endpointType, std::string& vendorId, std::string& productId, std::string& classId)
    {
        // the input hardware Id look like this:
        // {1}.PCI\VEN_1102&DEV_000B&SUBSYS_00411102&REV_04\4&4F261B2&0&00E1
        // or this:
        // {1}.HDAUDIO\FUNC_01&VEN_10EC&DEV_0887&SUBSYS_102804AA&REV_1003\4&2718705D&0&0001
        // or this:
        // {1}.USB\VID_046D&PID_0821&MI_00\8&333DF4D6&0&0000
        // We need to extract 3 pieces of information that make up the key
        // 1. type PCI or HDAUDIO or USB (we add our own HDMI type)
        // 2. Vendor id: VEN_XXXX or VID_XXXX
        // 3. Product id: PID_XXXX or DEV_XXXX

        // type is always the text following "{1}." and preceeding the first '\'
        const char* startChar = strchr(hardwareId.c_str(), '.');
        if (startChar)
        {
            const char* endChar = strchr(hardwareId.c_str(), '\\');
            if (endChar)
            {
                classId.assign(startChar + 1, (endChar - startChar - 1));
            }
        }
        // we change the type here so we can identify HDMI devices
        if (endpointType == DigitalAudioDisplayDevice)
        {
            classId = "HDMI";
        }

        // get the vendor id
        char* vendorKey = "VEN_";
        if (classId == "USB")
        {
            vendorKey = "VID_";
        }
        startChar = strstr(hardwareId.c_str(), vendorKey);
        if (startChar)
        {
            vendorId.assign(startChar + 4, 4);
        }

        // get the product id
        char* productKey = "DEV_";
        if (classId == "USB")
        {
            productKey = "PID_";
        }
        startChar = strstr(hardwareId.c_str(), productKey);
        if (startChar)
        {
            productId.assign(startChar + 4, 4);
        }
    }

    std::string AudioDeviceListenerWin::StringToUpper(std::string& strToConvert)
    {
        std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(), ::toupper);
        return strToConvert;
    }

    bool AudioDeviceListenerWin::GetAudioHardwareDetailsFromDeviceId(EDataFlow typeDev,
        const std::string& deviceId,
        std::string& vendorId,
        std::string& productId,
        std::string& classId,
        std::string& deviceIdKey,
        bool idKeyWithType){

        LOG(LS_INFO) << "Core Audio: GetAudioHardwareDetailsFromDeviceId";

        HRESULT hr = E_FAIL;
        std::string deviceGUID;
        std::string deviceKey;

        const char* guidStart = strrchr(deviceId.c_str(), '{');
        if (guidStart)
        {
            deviceGUID = guidStart;
        }

        bool found = false;

        CComPtr<IMMDeviceCollection> devices = NULL;
        CComPtr<IMMDevice> device = NULL;
        CComPtr<IPropertyStore> props = NULL;

        do{
            hr = m_enumerator->EnumAudioEndpoints(typeDev, DEVICE_STATE_ACTIVE, &devices);
            if (FAILED(hr)){
                LOG(LS_ERROR) << "Core Audio: Enumeration of Audio endpoints "
                    "while getting hardware details failed hr = "
                    << std::hex << hr <<" GetLastError = " << GetLastError();
                break;
            }

            unsigned int devCount;
            hr = devices->GetCount(&devCount);
            if (FAILED(hr))
            {
                LOG(LS_ERROR) << "Core Audio: Getting number of audio devices "
                    "while getting hardware details failed hr = "
                    << std::hex << hr <<" GetLastError = " << GetLastError();
                break;
            }

            for (unsigned int i = 0; i < devCount; i++)
            {
                props = NULL;
                device = NULL;

                if (found)
                {
                    break;
                }

                // Get pointer to endpoint number i.
                hr = devices->Item(i, &device);
                if (FAILED(hr))
                {
                    LOG(LS_ERROR) << "Core Audio: Error while getting"
                        " IMMDevice reference from device enumerator"
                        " for device index = " << i << " hr =" << std::hex
                        << hr << " GetLastError = " << GetLastError();
                    continue;
                }

                HRESULT hr = device->OpenPropertyStore(STGM_READ, &props);
                if (FAILED(hr))
                {
                    LOG(LS_ERROR) << "Core Audio: Error while opening"
                        " property store for device index  = " << i
                        << " hr = " << std::hex << "GetLastError = "
                        << GetLastError();
                    continue;
                }

                // get the GUID property
                PROPVARIANT pvGUID;
                PropVariantInit(&pvGUID);

                hr = props->GetValue(PKEY_AudioEndpoint_GUID, &pvGUID);
                if (FAILED(hr)){
                    LOG(LS_ERROR) << "Core Audio: Error while getting audio"
                        " end point guid for device index = " << i << " hr = "
                        << std::hex << hr << "GetLastError = " << GetLastError();
                    continue;
                }

                if (pvGUID.vt != VT_EMPTY && pvGUID.vt != VT_NULL && pvGUID.pwszVal != NULL)
                {
                    std::string hwGUIDStr = CW2A(pvGUID.bstrVal);
                    if (stricmp(deviceGUID.c_str(), hwGUIDStr.c_str()) == 0)
                    {
                        // get the endpoint form factor (used to detect HDMI devices)
                        unsigned int endpointType = 0;
                        bool hdmi = false;
                        PROPVARIANT pvType;

                        PropVariantInit(&pvType);
                        hr = props->GetValue(PKEY_AudioEndpoint_FormFactor, &pvType);
                        if (FAILED(hr))
                        {
                            LOG(LS_ERROR) << "Core Audio: Failed to get endpoint form"
                                " factor (used to detect HDMI devices)"
                                " for device index = " << i << " hr = "
                                << std::hex << hr << "GetLastError = "
                                << GetLastError();
                            continue;
                        }

                        endpointType = pvType.uintVal;
                        PropVariantClear(&pvType);

                        // found the device, now get the hardware Id
                        PROPVARIANT pvHwKey;

                        PropVariantInit(&pvHwKey);
                        hr = props->GetValue(PKEY_AudioHardwareIdString, &pvHwKey);
                        if (FAILED(hr))
                        {
                            LOG(LS_ERROR) << "Core Audio: Failed to get the hardware id"
                                " for device index = " << i << " hr = " <<
                                std::hex << "GetLastError = " << GetLastError();
                            continue;
                        }

                        //Jie: Not sure why we use bstrVal instead of pwszVal, but from the debugging, seems that vt is 31, which is VT_LPWSTR
                        if (pvHwKey.vt != VT_EMPTY && pvHwKey.vt != VT_NULL && pvHwKey.pwszVal != NULL)
                        {
                            // got the hardware key we were looking for
                            std::string hwKey = CW2A(pvHwKey.bstrVal);
                            HardwareKeyFromHardwareId(hwKey, endpointType, vendorId, productId, classId);
                            PropVariantClear(&pvHwKey);
                            // quit the loop
                            found = true;
                        }
                        else
                        {
                            LOG(LS_ERROR) << "Core Audio: Getting hardware id"
                                " with key PKEY_AudioHardwareIdString with"
                                <<"  GUID:"<< hwGUIDStr << " failed. Try"
                                " getting its friendly name";
                            PROPVARIANT pvFriendlyName;

                            PropVariantInit(&pvFriendlyName);
                            hr = props->GetValue(PKEY_Device_FriendlyName, &pvFriendlyName);
                            if (FAILED(hr))
                            {
                                LOG(LS_ERROR) << "Core Audio: Failed to get the"
                                    " friendly name for device index = " << i
                                    << "hr = " << std::hex << hr <<
                                    "GetLastError = " << GetLastError();
                                continue;
                            }

                            if (pvFriendlyName.vt == VT_LPWSTR && pvFriendlyName.pwszVal != NULL)
                            {
                                std::string hwFriendlyNameStr = CW2A(pvFriendlyName.pwszVal);
                                LOG(LS_INFO) << "Core Audio: Device Friendly name: " << hwFriendlyNameStr;
                            }
                        }
                    }
                }
                PropVariantClear(&pvGUID);
            }
        } while (0);

        if (FAILED(hr)) {
            LOG(LS_ERROR) << "Core Audio: Failed to get hardware details "
                " hr = " << std::hex << hr << "GetLastError = "
                << GetLastError();
            found = false;
        }

        // make our vendor, product and class keys uppercase
        StringToUpper(vendorId);
        StringToUpper(productId);
        StringToUpper(classId);

        // assemble the deviceKey;
        if (idKeyWithType) {
            deviceKey = typeDev == eRender ? "SPK" : "MIC";
            deviceKey += ":";
        }
        deviceKey += classId;
        deviceKey += ":";
        deviceKey += vendorId;
        deviceKey += ":";
        deviceKey += productId;

        // return the device id
        deviceIdKey = deviceKey;
        props = NULL;
        device = NULL;
        devices = NULL;

        return found;
    }
    // ----------------------------------------------------------------------
    //  OnMessage()
    //
    //  This is the place where we will receive messages dispatched by
    //  talk_base::Thread
    // ----------------------------------------------------------------------
    void AudioDeviceListenerWin::OnMessage(talk_base::Message *pmsg){
        switch (pmsg->message_id){
        case MSG_CHANGE_ENDPOINT_MIC:
        {
            talk_base::TypedMessageData<std::string> *mguid = static_cast<talk_base::TypedMessageData<std::string> *>(pmsg->pdata);
            ProcessMicChange(mguid->data());
            delete mguid;
            break;
        }
        case MSG_CHANGE_ENDPOINT_SPEAKER:
        {
            talk_base::TypedMessageData<std::string> *sguid = static_cast<talk_base::TypedMessageData<std::string> *>(pmsg->pdata);
            ProcessSpeakerChange(sguid->data());
            delete sguid;
            break;
        }
        case MSG_ENDPOINT_VOLUME_CHANGED:
        {
            talk_base::TypedMessageData<VolumeStats> *data = static_cast<talk_base::TypedMessageData<VolumeStats> *>(pmsg->pdata);
            VolumeStats volStats = data->data();
            if (NULL != m_deviceWatcher){
                m_deviceWatcher->OnVolumeChanged(volStats);
            }
            else{
                LOG(LS_ERROR) << "Core Audio: Device Watcher is null while sending volumes";
            }
            delete data;
            break;
        }
        case MSG_SET_SPEAKER_MUTE:
        {
            talk_base::TypedMessageData<bool> *data =
                static_cast<talk_base::TypedMessageData<bool> *>(pmsg->pdata);
            bool mute = data->data();
            SetSpeakerEndpointMute(mute);
            delete data;
            break;
        }
        case MSG_SET_SPEAKER_VOLUME:
        {
            talk_base::TypedMessageData<float> *data =
                static_cast<talk_base::TypedMessageData<float> *>(pmsg->pdata);
            float vol = data->data();
            SetSpeakerEndpointVolume(vol);
            delete data;
            break;
        }
        case MSG_SET_SESSION_MUTE:
        {
            talk_base::TypedMessageData<bool> *data =
                static_cast<talk_base::TypedMessageData<bool> *>(pmsg->pdata);
            bool mute  = data->data();
            SetSpeakerSessionMute(mute);
            delete data;
            break;
        }
        case MSG_SET_SESSION_VOLUME:
        {
            talk_base::TypedMessageData<float> *data =
                static_cast<talk_base::TypedMessageData<float> *>(pmsg->pdata);
            float vol = data->data();
            SetSessionVolume(vol);
            delete data;
            break;
        }
        case MSG_CHANGE_ENDPOINT_GUID_TIMEOUT:
        {
            auto deviceChangeMsgPtr =
                static_cast<talk_base::ScopedMessageData<PromiseMessageWrapper<HRESULT, DeviceChangeInfo>> *>(pmsg->pdata);
            auto deviceChangePromisePtr = deviceChangeMsgPtr ? deviceChangeMsgPtr->data().get() : NULL;

            if (deviceChangePromisePtr) {
                const DeviceChangeInfo &deviceInfo = deviceChangePromisePtr->getSendDataRef();
                HRESULT hr = deviceInfo.m_isMic ? ProcessMicChange(deviceInfo.m_guid)
                    : ProcessSpeakerChange(deviceInfo.m_guid);
                deviceChangePromisePtr->notify(hr); // Signal the completion to calling thread
            }
            else {
                LOG(LS_ERROR) << "Core Audio: Invalid arguments to MSG_CHANGE_ENDPOINT_MIC_TIMEOUT.";
            }

            delete deviceChangeMsgPtr; // Underlying DeviceChangeInfo object will be deleted as well,
                                            // since it is stored as a scoped pointer.
            break;
        }
        case MSG_GET_VOLUME_STATS_TIMEOUT:
        {
            auto getVolumeMsgPtr =
                static_cast<talk_base::ScopedMessageData<PromiseMessageWrapper<VolumeUpdateInfo>> *>(pmsg->pdata);
            auto updateVolumePromisePtr = getVolumeMsgPtr ? getVolumeMsgPtr->data().get() : NULL;

            if (updateVolumePromisePtr)
            {
                VolumeUpdateInfo updatedVolumeInfo;
                updatedVolumeInfo.m_result = GetCurrentVolStats(updatedVolumeInfo.m_volStats);
                updateVolumePromisePtr->notify(updatedVolumeInfo);
            }
            else
            {
                LOG(LS_ERROR) << "Core Audio: Invalid arguments to MSG_GET_VOLUME_STATS_TIMEOUT.";
            }

            delete getVolumeMsgPtr; // Underlying HRESULT memory will be deleted as well,
                                    // since it is stored as a scoped pointer.
            break;
        }
        case MSG_SET_MIC_VOLUME:
        {
            talk_base::TypedMessageData<float> *data =
                static_cast<talk_base::TypedMessageData<float> *>(pmsg->pdata);
            float vol = data->data();
            SetMicVolume(vol);
            delete data;
            break;
        }
        case MSG_SET_MIC_MUTE:
            talk_base::TypedMessageData<bool> *data =
                static_cast<talk_base::TypedMessageData<bool> *>(pmsg->pdata);
            bool bMute = data->data();
            SetMicMute(bMute);
            delete data;
            break;
        }
    }

    // ----------------------------------------------------------------------
    //  PostSetDeviceGUIDTimeout()
    //
    //  This function will set the device guid which we will be using
    //  to get the audio device in GetDefaultAudioEndPoint()
    //
    //  This change of end point wil be asynchronous and we will
    //  wait for a maximum of the specified timeout period before
    //  returning success/failure.
    // ----------------------------------------------------------------------
    bool AudioDeviceListenerWin::PostSetDeviceGUIDTimeout(bool isMic,
        const std::string& guid,
        unsigned long msecTimeout){

        LOG(LS_INFO) << "Core Audio: AudioDeviceListenerWin::PostSetDeviceGUIDTimeout: "
            << "Type = " << (isMic ? "Mic" : "Speaker") << ", guid = " << guid.c_str()
            << ", timeout = " << msecTimeout;

        bool success = false;

        // Create a pair of promise and future objects to signal result of volume
        // update operation.
        // Due to restriction of libjingle threading mechanism not allowing
        // use to use move semantics, dynamically allocate the promise object,
        // so it is not destroyed at end of scope in case of timeout.
        // Dispatcher thread will explicitly delete it.
        // Here the promise object is encapsulated in DeviceChangeInfo struct.
        auto deviceChangePromisePtr = new PromiseMessageWrapper<HRESULT, DeviceChangeInfo>();
        deviceChangePromisePtr->setSendData(DeviceChangeInfo(isMic, guid));
        // Get the future object to access the shared variable set in dispatcher thread.
        auto deviceChangeCompleteSignal = deviceChangePromisePtr->getFutureObject();
        PostScopedMsgToAudioDeviceListener(MSG_CHANGE_ENDPOINT_GUID_TIMEOUT, deviceChangePromisePtr);
        // Wait for promise to be completed for a maximum time of msecTimeout.
        if (deviceChangeCompleteSignal.valid())
        {
            auto futureStatus =
                deviceChangeCompleteSignal.wait_for(std::chrono::milliseconds(msecTimeout));
            if (std::future_status::ready == futureStatus)
            {
                // Even if promise object has been deleted,
                // it is safe to access the shared variable as it
                // is ref counted.
                HRESULT hr = deviceChangeCompleteSignal.get();
                if (SUCCEEDED(hr))
                {
                    success = true;
                }
                else
                {
                    LOG(LS_ERROR) << "Core Audio: Failed to complete device change with error: "
                        << std::hex << hr;
                }
            }
            else
            {
                LOG(LS_INFO) << "Core Audio: Async device change not completed in " << msecTimeout
                    << " milliseconds. future_status = " << static_cast<int>(futureStatus);
            }
        }
        else
        {
            LOG(LS_ERROR) << "Core Audio: Device change future object does not have valid state";
        }

        return success;
    }

    // ----------------------------------------------------------------------
    //  PostGetCurrentVolStats()
    //
    //  Post a message to dispatcher thread to get the current volume stats
    //  for system mic,system speaker and session
    //  System speaker vol stats corresponds to system speaker volume
    //  Session speaker vol stats corresponds to application
    //  System mic stats correponds to mic volume
    //  We wait for a maximum time specified in timeout for the operation
    //  to complete before returning.
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::PostGetCurrentVolStats(
        VolumeStats& volStats,
        unsigned long msecTimeout){
	    LOG(LS_INFO) << "Core Audio: " << __FUNCTION__ <<
		    "timeout: " << msecTimeout;
        HRESULT result = E_FAIL;
        // Create a pair of promise and future objects to signal result of volume
        // update operation.
        // Due to restriction of libjingle threading mechanism not allowing
        // use to use move semantics, dynamically allocate the promise object,
        // so it is not destroyed at end of scope in case of timeout.
        // Dispatcher thread will explicitly delete it.
        auto volumeStatsPromisePtr = new PromiseMessageWrapper<VolumeUpdateInfo>();
        auto volumeUpdateSignal = volumeStatsPromisePtr->getFutureObject();

        PostScopedMsgToAudioDeviceListener(MSG_GET_VOLUME_STATS_TIMEOUT, volumeStatsPromisePtr);
        // Do not modify promise object beyond this point.

        // Wait for future-promise shared variable to be set for a max duration of
        // msecTimeout milliseconds.
        if (volumeUpdateSignal.valid())
        {
            std::future_status futureStatus =
                volumeUpdateSignal.wait_for(std::chrono::milliseconds(msecTimeout));
            if (std::future_status::ready == futureStatus)
            {
                // Even if promise object has been deleted,
                // it is safe to access the shared variable as it
                // is ref counted.
                auto volumeUpdateInfo = volumeUpdateSignal.get();
                result = volumeUpdateInfo.m_result;
                if (SUCCEEDED(result))
                {
                    volStats = volumeUpdateInfo.m_volStats;
                }
                else
                {
                    LOG(LS_ERROR) << "Failed to get current volume, with error: "
                        << std::hex << result;
                }
            }
            else
            {
                LOG(LS_INFO) << "Core Audio: Async PostGetCurrentVolStats not completed in "
                    << msecTimeout << " milliseconds. future_status = "
                    << static_cast<int>(futureStatus);
            }
        }
        else
        {
            LOG(LS_ERROR) << "Core Audio: Device change future object does not have valid state";
        }
        return result;
    }

    // ----------------------------------------------------------------------
    //  ProcessMicChange()
    //
    //  Change the selected mic to the given guid.
    //  Also trigger a volume update message to be sent to watcher.
    //  **This private method should only be called from dispatcher thread.
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::ProcessMicChange(const std::string& micGUID)
    {
        // TODO: Remove the critical section in this and similar functions
        // after m_micEndPointVolumeNotification is accessed only via
        // dispatcher thread and not SIP thread i.e. when GetCurrentVoltStats is removed.
        talk_base::CritScope cs(&m_CS);
        bool postVolumeUpdate = false;
        HRESULT hr = E_FAIL;
        if (NULL == m_micEndPointVolumeNotification) {
            LOG(LS_ERROR) << "Core Audio: Mic"
                " end point notification pointer is null";
        }
        else if (m_bErrorState){
            LOG(LS_ERROR) << "Core Audio: Setting empty guid to select default device(mic)";
            hr = ChangeEndpoint(m_speakerEndPointVolumeNotification->GetSpeakerGUID(), "");
            postVolumeUpdate = true;
        }
        else if (!micGUID.empty() && (0 != micGUID.compare(m_micEndPointVolumeNotification->GetMicGUID()))){
            LOG(LS_INFO) << "Core Audio: Present mic guid is different";
            hr = ChangeEndpoint(m_speakerEndPointVolumeNotification->GetSpeakerGUID(), micGUID);
            postVolumeUpdate = true;
        }
        else{
            LOG(LS_INFO) << "Core Audio: Present mic guid is same as of passed guid";
            hr = S_OK;
        }

        if (postVolumeUpdate) {
            GetCurrentVolStats(m_volStat);
            m_dispatcherThread.Post(this, MSG_ENDPOINT_VOLUME_CHANGED, new talk_base::TypedMessageData<VolumeStats>(m_volStat));
        }

        return hr;
    }

    // ----------------------------------------------------------------------
    //  ProcessSpeakerChange()
    //
    //  Change the selected speaker to the given guid.
    //  Also trigger a volume update message to be sent to watcher.
    //  **This private method should only be called from dispatcher thread.
    // ----------------------------------------------------------------------
    HRESULT AudioDeviceListenerWin::ProcessSpeakerChange(const std::string& spkGuid)
    {
        // TODO(Suyash): Remove the critical section in this and similar functions
        // after m_speakerEndPointVolumeNotification is accessed only via
        // dispatcher thread and not SIP thread i.e. when GetCurrentVoltStats is removed.
        talk_base::CritScope cs(&m_CS);
        HRESULT hr = E_FAIL;
        bool postVolumeUpdate = false;
        if (NULL == m_speakerEndPointVolumeNotification) {
            LOG(LS_ERROR) << "Core Audio: Speaker"
                " end point notification pointer is null.";
        }
        else if (m_bErrorState){
            LOG(LS_ERROR) << "Core Audio: Setting empty guid to select default device(speaker)";
            hr = ChangeEndpoint("", m_micEndPointVolumeNotification->GetMicGUID());
            postVolumeUpdate = true;
        }
        else if (!spkGuid.empty() && (0 != spkGuid.compare(m_speakerEndPointVolumeNotification->GetSpeakerGUID()))){
            LOG(LS_INFO) << "Core Audio: Present speaker guid is different";
            hr = ChangeEndpoint(spkGuid, m_micEndPointVolumeNotification->GetMicGUID());
            postVolumeUpdate = true;
        }
        else{
            LOG(LS_INFO) << "Core Audio: Present speaker guid is same as of passed guid";
            hr = S_OK;
        }

        if (postVolumeUpdate) {
            GetCurrentVolStats(m_volStat);
            m_dispatcherThread.Post(this, MSG_ENDPOINT_VOLUME_CHANGED, new talk_base::TypedMessageData<VolumeStats>(m_volStat));
        }

        return hr;
    }


    bool AudioDeviceListenerWin::SetMicVolume(float micVol) {
        talk_base::CritScope cs(&m_CS);
        if (NULL != m_micEndPointVolumeNotification){
            return m_micEndPointVolumeNotification->SetMicVolume(micVol);
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Error while setting mic volume. "
                << "m_micEndPointVolumeNotification is NULL.";
            return false;
        }
    }

    bool AudioDeviceListenerWin::SetMicMute(bool bMute) {
        talk_base::CritScope cs(&m_CS);
        if (NULL != m_micEndPointVolumeNotification){
            return m_micEndPointVolumeNotification->SetMicMute(bMute);
        }
        else{
            LOG(LS_ERROR) << "Core Audio: Error while setting mic mute. "
                << "m_micEndPointVolumeNotification is NULL.";
            return false;
        }
    }
}
