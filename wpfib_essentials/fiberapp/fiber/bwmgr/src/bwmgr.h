/******************************************************************************
 * BwMgr header file
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Oct 3, 2013
 *****************************************************************************/

#ifndef BWMGR_H_
#define BWMGR_H_

#include "strmbwmgr.h"
#include "incomingbwmgr.h"
#include "outgoingbwmgr.h"

namespace bwmgr {

////////////////////////////////////////////////////////////////////////////////
// class BwMgr
class BwMgr
: public BwMgrHandler
, public BwMgrInterface
, public Logger
, private NonCopyable<BwMgr>
{
public:
    BwMgr(const BwMgrContext& ctx);
    ~BwMgr();

    // RtpBandwidthManager virtual functions
    virtual void updatePacketLoss(
        BwMgrMediaType mediaType,
        const LossReport &lossReport);

    virtual void updatePacketDelay(
            BwMgrMediaType mediaType,
            RtpPacketInfo &rtpInfo,
            PacketType pktType);

    // BwMgrHandler
    void OnIncomingBitrateChange(uint32_t bitrate);
    void OnOutgoingBitrateChange(uint32_t bitrate);

    // misc functions called by legacy users of this bwmgr (outgoing only)
    void setMaxBitrate(int32_t bitrate);
    void setBitrate(int32_t bitrate);
    int32_t getBitrate(void) const;
    double getConstantLoss(void) const;
    bool needUpdate(void);

    // misc functions for incoming bwmgr
    void setIncomingMaxBitrate(int32_t bitrate);
    void setIncomingMinBitrate(int32_t bitrate);
    void setIncomingBitrate(int32_t bitrate);
    int32_t getIncomingBitrate(void) const;
    double getIncomingConstantLoss(void) const;
    bool incomingNeedUpdate(void);

    // The following functions provide read-only access to internal state
    // for diagnostic purposes.
    // A typical app shouldn't need to look at these.
    void fetchInternalState(bwmgr::internal::IncomingBWMState &state) const;

private:
    BwMgrContext    mBwMgrCtx;

    OutgoingBwMgr   mOutgoingBwMgr;
    IncomingBwMgr   mIncomingBwMgr;
};

} // namespace bwmgr

#endif /* BWMGRSM_H_ */
