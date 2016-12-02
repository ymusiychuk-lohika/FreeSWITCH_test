#ifndef BJN_AUDIO_STATS_H
#define BJN_AUDIO_STATS_H

#include <float.h>
#include <algorithm>

struct BjnAecMetrics {
    BjnAecMetrics():
        mNumUpdates(0),
        mLinearErleDb(0.0f),
        mReportedLatencyMs(0.0f),
        mFilteredLatencyMs(0.0f),
        mBulkDelayLatencyMs(0.0f),
        mPrimaryReflectionDelayMs(0.0f),
        mResetCounter(0.0f),
        mBufferedDataMs(0.0f),
        mRawSkewMin(0.0f),
        mRawSkewMax(0.0f),
        mFilteredSkew(0.0f),
        mSoftResets(0) { };
    ~BjnAecMetrics() {};

    void UpdateMetrics(const BjnAecMetrics& metrics) {
        mNumUpdates++;
        mLinearErleDb = std::max(mLinearErleDb, metrics.mLinearErleDb);
        mFilteredLatencyMs += metrics.mFilteredLatencyMs;
        mPrimaryReflectionDelayMs += metrics.mPrimaryReflectionDelayMs;
        mResetCounter = std::max(mResetCounter, metrics.mResetCounter);
        mBufferedDataMs = std::min(mBufferedDataMs, metrics.mBufferedDataMs);
        mRawSkewMin = std::min(mRawSkewMin, metrics.mRawSkewMin);
        mRawSkewMax = std::max(mRawSkewMax, metrics.mRawSkewMax);
        mFilteredSkew += metrics.mFilteredSkew;
    };

    void Normalize() {
        float denom = std::max(1.0f, (float)mNumUpdates);
        mFilteredLatencyMs /= denom;
        mPrimaryReflectionDelayMs /= denom;
        mFilteredSkew /= denom;
    };

    void Reset() {
        mNumUpdates = 0;
        mLinearErleDb = 0.0f;
        mReportedLatencyMs = 0.0f;
        mFilteredLatencyMs = 0.0f;
        mBulkDelayLatencyMs = 0.0f; // TODO (brant) do we want this?
        mPrimaryReflectionDelayMs = 0.0f;
        mResetCounter = 0.0f;
        mBufferedDataMs = FLT_MAX;
        mRawSkewMin = FLT_MAX;;
        mRawSkewMax = FLT_MIN;
        mFilteredSkew = 0.0f;
    };

    uint32_t mNumUpdates;
    float mLinearErleDb;
    float mReportedLatencyMs;
    float mFilteredLatencyMs;
    float mBulkDelayLatencyMs;
    float mPrimaryReflectionDelayMs;
    float mResetCounter;
    float mBufferedDataMs;
    float mRawSkewMin;
    float mRawSkewMax;
    float mFilteredSkew;
    int32_t mSoftResets;
};

struct BjnAgcMetrics {
    BjnAgcMetrics():
        mNumUpdates(0),
        mMicrophoneLevel(0.0f),
        mDigitalGainDb(0.0f),
        mAgcSaturation(0.0f),
        mAgcAdaptationGating(0.0f) { };
    ~BjnAgcMetrics() {};

    void UpdateMetrics(const BjnAgcMetrics& metrics) {
        mNumUpdates++;
    };

    void Normalize() {
        float denom = std::max(1.0f, (float)mNumUpdates);
        mMicrophoneLevel /= denom;
        mDigitalGainDb /= denom;
        mAgcSaturation /= denom;
        mAgcAdaptationGating /= denom;
    };

    void Reset() {
        mNumUpdates = 0;
        mMicrophoneLevel = 0.0f;
        mDigitalGainDb = 0.0f;
        mAgcSaturation = 0.0f;
        mAgcAdaptationGating = 0.0f;
    };

    uint32_t mNumUpdates;
    float mMicrophoneLevel;
    float mDigitalGainDb;
    float mAgcSaturation;
    float mAgcAdaptationGating;
};

struct BjnEsmMetrics {
    BjnEsmMetrics():
        mNumUpdates(0),
        mNoiseLevelDb(-FLT_MIN),
        mSignalLevelDb(-FLT_MIN),
        mSpkrLevelDb(-FLT_MIN),
        mTypingEvents(0.0f) { };
    ~BjnEsmMetrics() {};

    void UpdateMetrics(const BjnEsmMetrics& metrics) {
        mNumUpdates++;
        mNoiseLevelDb = std::max(metrics.mNoiseLevelDb, mNoiseLevelDb);
        mSignalLevelDb = std::max(metrics.mSignalLevelDb, mSignalLevelDb);
        mSpkrLevelDb = std::max(metrics.mSpkrLevelDb, mSpkrLevelDb);
        mTypingEvents += metrics.mTypingEvents;
    };

    void Normalize() {
        float denom = std::max(1.0f, (float)mNumUpdates);
        mTypingEvents /= denom;
    };

    void Reset() {
        mNumUpdates = 0;
        mNoiseLevelDb = -FLT_MAX;
        mSignalLevelDb = -FLT_MAX;
        mSpkrLevelDb = -FLT_MAX;
        mTypingEvents = 0.0f;
    };

    uint32_t mNumUpdates;
    float mNoiseLevelDb;
    float mSignalLevelDb;
    float mSpkrLevelDb;
    float mTypingEvents;
};

#endif
