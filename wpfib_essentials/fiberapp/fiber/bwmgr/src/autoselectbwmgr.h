/******************************************************************************
 * This is another wrapper to allow one object to handle two types of
 * bandwidth management -- the WebRTC absolute send time and our BJN
 * bandwidth manager.
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Jul 14, 2015
 *****************************************************************************/

#ifndef BWMGR_AUTOSELECTBWMGR_H
#define BWMGR_AUTOSELECTBWMGR_H

#include <memory>
#include "bwmgr_types.h"
#include "logger.h"
#include "smartpointer.h"

namespace bwmgr
{

// This wraps the bwmgr interface so that we'll use WebRTC's absolute send time
// bwmgr or our own bwmgr.  We'll start with the WebRTC bwmgr by default but
// switch if incoming packets lack the absolute send time information
class AutoSelectBwMgr : public BwMgrInterface, private NonCopyable<AutoSelectBwMgr>
{
public:
    AutoSelectBwMgr(const BwMgrContext &ctx);
    ~AutoSelectBwMgr();

    // this is more for outgoing bw control
    virtual void updatePacketLoss(
        BwMgrMediaType mediaType,
        const LossReport &lossReport);

    // this is  used for incoming bw control
    virtual void updatePacketDelay(
        BwMgrMediaType mediaType,
        RtpPacketInfo &rtpInfo,
        PacketType pktType);

    // misc functions for incoming bwmgr, if any
    virtual void setIncomingMaxBitrate(int32_t bitrate);
    virtual void setIncomingBitrate(int32_t bitrate);
    virtual void setIncomingMinBitrate(int32_t bitrate);
    virtual int32_t getIncomingBitrate() const;
    virtual double getIncomingConstantLoss() const;
    virtual bool incomingNeedUpdate();

    // misc functions called by legacy users of this bwmgr (outgoing only)
    virtual void setMaxBitrate(int32_t bitrate);
    virtual void setBitrate(int32_t bitrate);
    virtual int32_t getBitrate() const;
    virtual double getConstantLoss() const;
    virtual bool needUpdate();

    virtual void process(BwMgrMediaType mediaType);

private:
    typedef SharedPtr<BwMgrInterface> BwMgrPtr;

    enum Mode
    {
        BJN_BWMGR,
        WEBRTC_BWMGR,
    };

    class BitrateChangeHandler : public BwMgrHandler
    {
    public:
        BitrateChangeHandler(AutoSelectBwMgr &owner) : mOwner(owner) {}
    private:
        void OnIncomingBitrateChange(uint32_t bitrate)
        {
            mOwner.OnIncomingBitrateChange(bitrate);
        }
        void OnOutgoingBitrateChange(uint32_t bitrate)
        {
            mOwner.OnOutgoingBitrateChange(bitrate);
        }
        AutoSelectBwMgr &mOwner;
    };

    void OnIncomingBitrateChange(uint32_t bitrate);
    void OnOutgoingBitrateChange(uint32_t bitrate);

    void changeBwMgr(Mode mode);

    BitrateChangeHandler mBitrateChangeHandler;

    // this is the underlying bwmgr --
    // we'll use a shared_ptr so that we can keep reference counts, in which requires
    // functions using this to make a shared_ptr copy to increment that reference.
    // The reason behind the madness is to allow us to get away without using a mutex,
    // at the risk of potentially operating on a stale object.
    BwMgrPtr mBwMgr;
    BwMgrContext mContext;
    Logger mLog;

    // We want to intercept bitrates, even in functions declared const
    // by bwmgr interface
    mutable uint32_t mIncomingBitrate;
    mutable uint32_t mOutgoingBitrate;

    Mode mMode;
};

} // namespace bwmgr

#endif /* BWMGR_AUTOSELECTBWMGR_H */
