/******************************************************************************
 *  audio_network_monitor.cpp
 *
 *  This simple Audio Network Monitor 'peaks' at the inbound/outbound RTCP
 *  reports and tries to make a conservative decision as to whether the
 *  audio streams have been degraded to a point where some graceful degration
 *  warning should be emitted.
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brant Jameson
 *****************************************************************************/

#ifndef __RX_AUDIO_NETWORK_MONITOR_H__
#define __RX_AUDIO_NETWORK_MONITOR_H__

#include <string>
#include <stdint.h>
#include <pjmedia.h> // for rtcp network reports
#include <pjlib.h>

#include "moving_average.h"

static const uint32_t kDefaultLossThresh = 25;
static const uint32_t kDefaultLossHyst = 2;
static const uint32_t kDefaultLossTau = 40; // 40s smoother
static const uint32_t kRtcpInterval = 2;
static const uint32_t kMinMvAvgWindow = 5;

class AudioNetmon
{
private:
    bool    mEnabled;
    bool    mRxNetworkPoor;
    bool    mTxNetworkPoor;
    bool    mRxNetworkGotPoor;
    bool    mTxNetworkGotPoor;
    uint32_t    mRxLossThresh;
    uint32_t    mTxLossThresh;
    uint32_t    mMeanRxLoss;
    uint32_t    mMeanTxLoss;
    MovingAverage<uint32_t>    mRxLoss;
    MovingAverage<uint32_t>    mTxLoss;
    pjmedia_rtcp_stat          mRtcpStats;

public:
    AudioNetmon():
        mEnabled(false),
        mRxNetworkPoor(false),
        mTxNetworkPoor(false),
        mRxNetworkGotPoor(false),
        mTxNetworkGotPoor(false),
        mRxLossThresh(kDefaultLossThresh),
        mTxLossThresh(kDefaultLossThresh),
        mMeanRxLoss(0),
        mMeanTxLoss(0),
        mRxLoss(kDefaultLossTau/kRtcpInterval),
        mTxLoss(kDefaultLossTau/kRtcpInterval) { };
    ~AudioNetmon() {};

    void Reset();
    int Initialize(uint32_t lossInbound = kDefaultLossThresh,
                   uint32_t tauInbound = kDefaultLossTau,
                   uint32_t lossOutbound = kDefaultLossThresh,
                   uint32_t tauOutbound = kDefaultLossTau);
    void Enable(bool enable) {
        mEnabled = enable;
    };
    bool IsEnabled() const {
        return mEnabled;
    };
    bool UpdateAudioStats(const pjmedia_rtcp_stat& audStats);
    bool DidInboundNetworkGetPoor() const {
        return mRxNetworkGotPoor;
    };
    bool DidOutboundNetworkGetPoor() const {
        return mTxNetworkGotPoor;
    };
};

#endif // #define __RX_AUDIO_NETWORK_MONITOR_H__
