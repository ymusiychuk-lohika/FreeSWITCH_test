/******************************************************************************
 * The RTX Manager monitors network characteristics to determine whether it is
 * worthwhile using packet retransmission, and what sort of delay may be needed
 * to use it.
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   02/12/2015
 *         Ported to fiber on March 5, 2015, converting C++11 constructs and
 *         other elements that don't work in fiber.
 *****************************************************************************/

#ifndef BJN_RTXMANAGER_H
#define BJN_RTXMANAGER_H

#include <deque>
#include <rtpstatsutil.h>
#include <pjsua-lib/pjsua.h>

namespace bjn_sky
{

class RtxEventHandler
{
public:
    virtual ~RtxEventHandler() {}
    virtual void OnRtxEnablementChange(bool newState) = 0;
    virtual void OnRtxDelayChange(uint32_t minDelayMs) = 0;
    virtual void OnRtxOutboundLossRateChange(float lossRate) = 0;
    virtual void OnRTTChange(uint32_t avgRTT) = 0;
    virtual void OnRtxNumNackRetransmitChange(uint32_t retransmits) = 0;
    virtual void OnRtxNackIntervalChange(uint32_t retransmitInterval) = 0;
};

struct RtxConfigurableParams
{
    uint32_t mMaxAllowedDelayInMs;
    float    mTolerableInBoundLoss;

    RtxConfigurableParams():
    mMaxAllowedDelayInMs(300),
    mTolerableInBoundLoss(0.001)
    {

    };
};

struct NackLossRange {
    float lossLowThreshold;
    float lossHighThreshold;
    uint16_t numNacks;

    NackLossRange(float low, float high, uint16_t numNacks):
    lossLowThreshold(low),
    lossHighThreshold(high),
    numNacks(numNacks)
    {

    };

    NackLossRange(const NackLossRange * ptr) :
    lossLowThreshold(ptr->lossLowThreshold),
    lossHighThreshold(ptr->lossHighThreshold),
    numNacks(ptr->numNacks)
    {

    }
};

typedef struct NackLossRange NackLossRange_t;

class RtxManager
{
public:
    RtxManager(RtxEventHandler &handler, bool isHandlerReady, RtxConfigurableParams &params);
    ~RtxManager();

    // If the RtxEventHandler is not ready to handle events at the time RtxManager is constructed,
    // it should call HandlerIsReady() when it is ready, and the initial set of events will be fired.
    // Similarly, if it ceases to be ready, it can be called with false.
    void HandlerReady(bool isReady);
    bool IsEnabled() const { return mEnabled; }
    uint32_t GetCurrentDelay() const { return mCurrentDelay; }
    float GetCurrentOutboundLossRate() const;
    uint32_t GetAvgRTT() const { return mAvgRTT;}

    void Update(uint32_t rttMs, uint32_t outCumulLoss, uint32_t outPktCount);
    void UpdateInBoundStats(uint32_t outCumulLoss, uint32_t outPktCount);

private:
    struct LossDetails
    {
        pj_time_val updateTime;
        uint32_t cumulLoss;
        uint32_t numPackets;
    };

    class LossIsOld
    {
    private:
        const pj_time_val &now;
    public:
        LossIsOld(const pj_time_val &t) : now(t) {}
        bool operator()(const LossDetails &ld);
    };
    void UpdateOutboundPacketLoss(const pj_time_val &updateTime
            , uint32_t cumulLoss, uint32_t pktCount);

    void UpdateInBoundPacketLoss(const pj_time_val &updateTime
            , uint32_t cumulLoss, uint32_t pktCount);

    void UpdateRoundTripTime(const pj_time_val &updateTime
            , uint32_t rttMs);

    void UpdateNumNackRetransmitVal();
    void UpdateRtxState();
    void OnRtxDisabled();
    void OnRtxEnabled(uint32_t currDelay);

    RtxEventHandler &mHandler;

    std::deque<LossDetails> mLoss;
    std::deque<LossDetails> mInBoundLoss;
    bwmgr::WindowedAverageT<float> mRTT;
    pj_time_val mLastRTTUpdate;
    pj_time_val mLastLossRateUpdate;
    pj_time_val mLastDisableConditionsWereTrue;
    pj_time_val mLastEnableConditionsWereTrue;

    uint32_t mCurrentDelay;
    uint32_t mMaxDelay;
    uint32_t mNumRTTSubmissions;
    float mLossRate;
    float mInBoundLossRate;
    bool mEnabled;
    bool mRtxReqEnabled;
    bool mHandlerReady;
    RtxConfigurableParams mConfigParams;
    uint32_t mAvgRTT;
    uint32_t mNumNackRetransmits;
    std::vector<NackLossRange_t> mNumNackVsLossRangeTable;
};

} // namespace bjn_sky

#endif /* BJN_RTXMANAGER_H */
