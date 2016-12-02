/******************************************************************************
 * BwMgr header file
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Oct 3, 2013
 *****************************************************************************/

#ifndef INCOMINGBWMGR_H_
#define INCOMINGBWMGR_H_

#include "duplicatepacketchecker.h"
#include "smdataandactions.h"
#include "windowedbitrateandlosstracker.h"

namespace bwmgr {

class DelayMonitor;

class IncomingBwMgr
: public SMDataAndActions
{
public:
    IncomingBwMgr(const BwMgrContext& ctx);
    virtual ~IncomingBwMgr();

    // this is  used for incoming bw control
    virtual void updatePacketDelay(
        BwMgrMediaType mediaType,
        RtpPacketInfo &rtpInfo,
        PacketType     pktType);

    // The following function is intended to be called by test applications
    // rather than actual bandwidth management users
    void fetchInternalState(bwmgr::internal::IncomingBWMState &state) const;

protected:

    void    notifyBitrateChange(uint32_t bitrate);

    bool    processPacketDelay();
    bool    processPacketLoss(uint32_t incomingBitrate);

private:
    WindowedBitrateAndLossTracker mBitrateLossTracker;
    DelayMonitor*   mDelayMonitor;
    DuplicatePacketChecker mDupeCheck;
    uint64_t        mStartTimeInMsecs;
    uint32_t        mActiveSSRC;
};

} // namespace bwmgr

#endif /* BWMGRSM_H_ */
