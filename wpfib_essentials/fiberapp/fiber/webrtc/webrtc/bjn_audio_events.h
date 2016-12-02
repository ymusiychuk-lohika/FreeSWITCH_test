#ifndef BJN_AUDIO_EVENTS_H
#define BJN_AUDIO_EVENTS_H


#include <map>
#include <string>
#include <sstream>
#include <vector>
#include <algorithm>
#include "common_types.h"
#include "bjn_audio_stats.h"

#define LOCAL_ECHO_PROBABLE_STR "LocalEchoProbable"
#define SPEAKING_WHILE_MUTED_STR "SpeakingWhileMuted"
#define CONSTANT_AEC_BUFFER_STARVATION_STR "ConstantAecBufferStarvation"
#define AUDIO_DRIVER_GLITCH_STR "AudioDriverGlitch"

enum
{
    ECBS_BUFFER_STARVATION_THRESHOLD = 20, // Below this is a starvation event
};

struct EsmMetrics {
    float mXCorrMetric;
    float mEchoPresentMetric;
    float mEchoRemovedMetric;
    float mEchoDetectedMetric;
    EsmMetrics():
        mXCorrMetric(0.0f),
        mEchoPresentMetric(0.0f),
        mEchoRemovedMetric(0.0f),
        mEchoDetectedMetric(0.0f) { };
};

struct AudioDspMetrics {
    AudioDspMetrics():
        mSpkrMuteState(0.0f),
        mSpkrVolume(0.0f),
        mMicMuteState(0.0f),
        mMicVolume(0.0f) { InitializeNetworkStats(); };
    ~AudioDspMetrics() {};

    void ResetMetrics() {
        mSpkrMuteState = mSpkrVolume = mMicMuteState = mMicVolume = 0.0f;
        mAecMetrics.Reset();
        mAgcMetrics.Reset();
        mEsmMetrics.Reset();
    };

    void InitializeNetworkStats() {
        mNetworkStats.currentBufferSize = 0;
        mNetworkStats.preferredBufferSize = 0;
        mNetworkStats.jitterPeaksFound = 0;
        mNetworkStats.currentPacketLossRate = 0;
        mNetworkStats.currentDiscardRate = 0;
        mNetworkStats.currentExpandRate = 0;
        mNetworkStats.currentPreemptiveRate = 0;
        mNetworkStats.currentAccelerateRate = 0;
        mNetworkStats.clockDriftPPM = 0;
        mNetworkStats.meanWaitingTimeMs = 0;
        mNetworkStats.medianWaitingTimeMs = 0;
        mNetworkStats.minWaitingTimeMs = 0;
        mNetworkStats.maxWaitingTimeMs = 0;
        mNetworkStats.addedSamples = 0;
    };

    void UpdateMetrics(const BjnAecMetrics& aecMetrics,
                       const BjnAgcMetrics& agcMetrics,
                       const BjnEsmMetrics& esmMetrics){
        mAecMetrics.UpdateMetrics(aecMetrics);
        mAgcMetrics.UpdateMetrics(agcMetrics);
        mEsmMetrics.UpdateMetrics(esmMetrics);
    };

    void NormalizeMetrics() {
        mAecMetrics.Normalize();
        mAgcMetrics.Normalize();
        mEsmMetrics.Normalize();
    };

    BjnAecMetrics   mAecMetrics;
    BjnAgcMetrics   mAgcMetrics;
    BjnEsmMetrics   mEsmMetrics;
    float   mSpkrMuteState;
    float   mSpkrVolume;
    float   mMicMuteState;
    float   mMicVolume;
    webrtc::NetworkStatistics mNetworkStats;
};

class WEBRTC_DLLEXPORT VoEAudioStatsCallback
{
public:
    virtual void ReportAudioDspStats(AudioDspMetrics& metrics) = 0;

protected:
    virtual ~VoEAudioStatsCallback() {}
};

class WEBRTC_DLLEXPORT VoEEsmStateCallback
{
public:
    virtual void OnToggleEsmState(bool esmEnabled, EsmMetrics esmMetrics) = 0;

protected:
    virtual ~VoEEsmStateCallback() {}
};

class WEBRTC_DLLEXPORT VoeEcStarvationCallback
{
public:
    virtual void OnBufferStarvation(int bufferStarvationMs, int delayMS) = 0;

protected:
    virtual ~VoeEcStarvationCallback() {}
};

class WEBRTC_DLLEXPORT VoEUnsupportedDeviceCallback
{
public:
    virtual void OnEsmEnabled(bool esmEnabled) = 0;

protected:
    virtual ~VoEUnsupportedDeviceCallback() {}
};

class WEBRTC_DLLEXPORT VoEAudioMicAnimationCallback
{
public:
    virtual void ReportMicAnimationState(int state) = 0;

protected:
    virtual ~VoEAudioMicAnimationCallback() {}
};

typedef std::string GracefulEventNotification;
typedef std::vector<GracefulEventNotification> GracefulEventNotificationList;
typedef std::map<GracefulEventNotification, bool> GracefulEventNotificationFilter;

class WEBRTC_DLLEXPORT VoEAudioNotificationCallback
{
public:
    virtual void OnAudioNotification(GracefulEventNotificationList& notificationList) = 0;

protected:
    virtual ~VoEAudioNotificationCallback() {}
};

#endif

