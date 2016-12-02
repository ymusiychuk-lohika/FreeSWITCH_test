/******************************************************************************
 * BwMgr header file
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Oct 3, 2013
 *****************************************************************************/

#ifndef SMDataAndActions_H_
#define SMDataAndActions_H_

#include "bwmgr_types.h"
#include "bwmgrsm.h"
#include "logger.h"
#include "rtpstatsutil.h"
#include "lossmonitor.h"

namespace bwmgr {

class SMDataAndActions
: public SMActionHandler
, public BwMgrInterface
, public Logger
, private NonCopyable<SMDataAndActions>
{
public:
    SMDataAndActions(const BwMgrContext& ctx);
    virtual ~SMDataAndActions();

    void    setHandler(BwMgrHandler* handler);

    // BwMgrInterface methods
    virtual void updatePacketLoss(
        BwMgrMediaType mediaType,
        const LossReport &lossReport) {}

    // this is  used for incoming bw control
    virtual void updatePacketDelay(
        BwMgrMediaType mediaType,
        RtpPacketInfo &rtpInfo,
        PacketType     pktType) {}

    // misc functions called by users of this bwmgr

    void    setMaxBitrate(int32_t bitrate);
    int32_t getMaxBitrate(void) const;

    void setMinBitrate(int32_t bitrate);
    int32_t getMinBitrate(void) const;

    void    setBitrate(int32_t bitrate);
    int32_t getBitrate(void) const;
    double  getConstantLoss(void) const;
    bool    needUpdate(void);

    // SMActionHandler functions
    void setBwIncrement(double rate);
    void setTimerInMsecs(uint32_t time);
    void setTimerInMsecsBasedOnActiveBitrate();
    void updateActiveBitrateTimeout();
    void clearTimer(void);
    void updateActiveBwByIncrement(void);
    void decrementActiveBwByIncrement(void);
    void updateActiveBwByLoss(void);
    void moveActiveToSafeBw(void);
    void moveSafeToActiveBw(void);
    AdjustmentsPtr getAdjustments();
protected:

    bool    processPacketLoss(
                uint64_t now,
                double    loss,
                uint16_t  packetSizeAvg = 0,    // not used with incoming bwmgr
                uint16_t  rttMs = 0);           // not used with incoming bwmgr

    uint32_t calcTFRCbps(
                int16_t  avgPackSizeBytes,
                double   packetLoss, // packet loss rate in [0, 1)
                uint16_t rttMs);

    bool    canSustainCurrentBitrate(float lossRate);

    virtual void    notifyBitrateChange(uint32_t bitrate);

    // handle state actions
    void    StateAction_Loss(void);
    void    StateAction_Delay(void);
    void    StateAction_TimeOut(void);
    void    StateAction_MaxRate(void);

    uint32_t    getTimeout(void);
    uint64_t    getLastTimeoutTime(void);
    void        setLastTimeoutTime(uint64_t timeout);
    bool        testAndExpireTimeout(uint64_t now);

    BwMgrHandler* getHandler(void);

    bool        timeToUpdatePacketLoss(uint64_t);

    const char* getSMName(void);
    BwMgrSM*    getStateMachine(void);

    uint64_t    getTimeInMSecs(void);

    // Informs of current loss rate [0-1]
    void setCurrentLossRate(double activeLoss);

protected:
    BwMgrContext mContext;

    BwMgrSM*   mStateMachine;
    LossMonitor mLossMonitor;

    enum
    {
        BWMGR_MIN_BITRATE_INCREMENT         = 16000,
        BWMGR_MAX_BITRATE_INCREMENT         = 80000,

        // a very high bitrate that we're unlikely to exceed.
        // The value itself is arbitrary, as its purpose
        // is to calculate the number of histogram bins needed.
        BWMGR_MAX_BITRATE = 4096000,

        // The rate probe value is the
        MAINTAIN_BIN_WIDTH = BWMGR_MIN_BITRATE_INCREMENT,
    };

private:

    uint32_t decrementBitrate(uint32_t activeBitrate, double rate) const;
    static uint32_t roundBitrate(uint32_t bitrate);

    // used by RtpProxy
    bool        mBwNeedsUpdate;

    BwMgrHandler*       mHandler;

    // used to deal with loss
    double      mActiveLoss;
    uint64_t    mLastLossTime;

    // Used with FSM
    double      mBwChangeForState;
    uint64_t    mLastTimeoutTime;
    uint32_t    mTimeout;
    uint32_t    mLastSafeBw;
    uint32_t    mActiveBw;
    uint32_t    mMinBitrateConfigured;
    uint32_t    mMaxBitrateConfigured;

    SharedPtr<Clock> mTimeSource;

    typedef BasicHistogram<0, BWMGR_MAX_BITRATE, MAINTAIN_BIN_WIDTH> BitrateHistogram;

    // This tracks areas bitrates that were considered safe per 'timeout' events
    BitrateHistogram mStableBitrateHistogram;
};

} // namespace bwmgr

#endif /* BWMGRSM_H_ */
