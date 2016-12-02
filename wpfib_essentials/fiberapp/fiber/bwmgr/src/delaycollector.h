/****************************************************************************
 * delaycollector.h
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   06/21/2013
 *****************************************************************************/

#ifndef DELAYCOLLECTOR_H
#define DELAYCOLLECTOR_H

#include "bwmgr_types.h"
#include "rtpstatsutil.h"

namespace bwmgr
{

class DelayCollector {
public:
    DelayCollector();
    ~DelayCollector();

    // Apply timing details of a packet (done for each packet in a frame)
    void    updatePacketDelay(const RtpPacketInfo &pktInfo);

    // Marks the most recently frame as the new reference
    // (eg. if a keyframe was received)
    void    updateReferencePacket();

    // Reset running state (but keep config values)
    void    reset();

    // Gets the unfiltered delay for the previous frame.  As a convenience
    // it returns true when a new value is available
    bool    getPacketDelay(int32_t& delay);

    // Same as above -- caller needs to test frameChanged()
    int32_t getFrameDelay() const { return mLastAvgFrameDelay; }

    // Returns true on first packet, or first packet after an RTP timestamp change
    bool    frameChanged() const
    {
        return mFirstPacketAfterNewFrame;
    }

    // Number of RTP frames that elapsed since the "reference" frame
    uint32_t    getNumberOfFramesSinceReference() const
    {
        return mNumFramesSinceRef;
    }

    uint64_t    getReferenceRecvTime(void)
    {
        return mReferencePacket.getReceiveTime();
    }

    uint64_t    getPacketRecvTime(void)
    {
        return mCurrentPacket.getReceiveTime();
    }

protected:
    // Not much more than a tuple of timestamp/recvtime
    class  PacketStats {
    public:
        PacketStats()
        : mReceiveTime(0)
        , mTimestamp(0)
        , mSeq(0)
        , mIsDefined(false)
        {}

        uint32_t getTimestamp(void) const
        {
            return mTimestamp;
        }

        // receive time in milliseconds
        uint64_t getReceiveTime(void) const
        {
            return mReceiveTime;
        }

        uint16_t getSeq() const
        {
            return mSeq;
        }

        // resets to initial state
        void reset();

        // Sets timing of current packet
        void updatePacketTimes(const RtpPacketInfo &pktInfo);

        int64_t getNormalizedDelay(const PacketStats& ref) const;

        bool IsDefined() const { return mIsDefined; }

     protected:
        uint64_t    mReceiveTime;
        uint32_t    mTimestamp;
        uint16_t    mSeq;
        bool        mIsDefined;
    };



    // Saves delay details for the previous frame
    // Only makes sense to call this upon receipt of a new frame
    void    saveDelayOfCompletedFrame();

    // Returns the delay associated with the current frame against
    // the reference frame.
    int32_t getCurrentFrameDelay() const;

    int32_t updateTotalFrameDelay(const RtpPacketInfo &pktInfo);

    // Clears running delay stats for current frame
    void    resetFrameDelay();

    PacketStats     mReferencePacket;
    PacketStats     mCurrentPacket;
    PacketStats     mFirstPacketInFrame;

    int32_t         mCurrentFrameDelay;
    uint32_t        mNumPacketsInFrame;
    uint32_t        mNumFramesSinceRef;

    uint32_t        mLastAvgFrameDelay;

    bool            mFirstPacketAfterNewFrame;
};

} // namespace bwmgr

#endif /* DELAYCOLLECTOR_H_ */
