/******************************************************************************
 * This is a wrapper around the WebRTC absolute send time remote bitrate
 * estimator.
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Jul 10, 2015
 *****************************************************************************/

#ifndef BWMGR_BJNWEBRTCBWMGR_H
#define BWMGR_BJNWEBRTCBWMGR_H

#include "bwmgr_types.h"
#include "smartpointer.h"

namespace bwmgr
{
// this isn't webrtc code, but the double namespace is to help isolate this implementation
// from a related implementation within connectsip.
namespace webrtc
{
// Interface to Webrtc's absolute send time remote bitrate estimator
class BjnWebrtcBwMgr : public BwMgrInterface, private NonCopyable<BjnWebrtcBwMgr>
{
public:
    BjnWebrtcBwMgr(const BwMgrContext &ctx);
    ~BjnWebrtcBwMgr();

    // This function was defined for a TFRC style bwmgr.
    // We only use it to collect the RTT
    virtual void updatePacketLoss(
        BwMgrMediaType mediaType,
        const LossReport &lossReport);

    // Supplies a packet to the bwmgr
    virtual void updatePacketDelay(
        BwMgrMediaType mediaType,
        RtpPacketInfo &rtpInfo,
        PacketType pktType);

    // misc functions for incoming bwmgr, if any
    virtual void setIncomingMaxBitrate(int32_t bitrate);
    virtual void setIncomingBitrate(int32_t bitrate);
    virtual void setIncomingMinBitrate(int32_t bitrate);
    virtual int32_t getIncomingBitrate(void) const;
    virtual double getIncomingConstantLoss(void) const;
    virtual bool incomingNeedUpdate(void);

    virtual void process(BwMgrMediaType mediaType);

private:
    // The "outgoing" direction isn't relevant at all.   While these are virtual, and can be called by
    // users of the BwMgrInterface, these are left private so that direct users can't invoke them.
    virtual void setMaxBitrate(int32_t bitrate);
    virtual void setBitrate(int32_t bitrate);
    virtual int32_t getBitrate() const;
    virtual double getConstantLoss() const;
    virtual bool needUpdate();


    // Hide all webrtc types in the cpp file, to ensure webrtc includes don't leak
    // out into the public interface...
    struct Privates;
    UniquePtr<Privates> mPriv;
};

} // namespace webrtc
} // namespace bwmgr

#endif /* BWMGR_BJNWEBRTCBWMGR_H */
