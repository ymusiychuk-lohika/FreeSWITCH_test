/******************************************************************************
 * This defines a duplicate packet checker, which is essentially a sliding
 * window of packets in which there are two states -- received or not received.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Mar 31, 2014
 *****************************************************************************/

#ifndef BWMGR_DUPLICATEPACKETCHECKER_H
#define BWMGR_DUPLICATEPACKETCHECKER_H

#include <bitset>
#include <stdint.h>
#include "rtputil.h"

namespace bwmgr {

// This checks for duplicate packets within a given window.  Underneath
// the hood, this is nothing more than a wrapper around a std::bitset, in this uses
// it as a shifting window of bools to determine whether a packet was previously
// received or not.
class DuplicatePacketChecker
{
public:

    // Since bitsets require the size to be known at compile-time, we specify this ahead
    // of time.  In general, since we're compiling for 64-bit platforms, the size should
    // be a multiple of 64.  256 (32 bytes) should give us about 5 seconds of history
    // for a typical audio stream, though a little less for a video stream
    enum
    {
        WINDOW_SIZE = 256,
        NEG_DELTA_TO_TREAT_AS_NEW_STREAM = -4096,
    };
    DuplicatePacketChecker()
        : mHighestSeqNo(0)
        , mFirstPacket(true)
    {
    }

    // Inserts seqno into the duplicate packet checker.
    // returns false if the sequence number is already present.
    bool insertAndTest(uint16_t seqNo)
    {
        if (mFirstPacket)
        {
            mDupeWindow[0] = true;
            mHighestSeqNo = seqNo;
            mFirstPacket = false;
            return true;
        }

        int16_t delta = seqDelta(seqNo, mHighestSeqNo);
        bool isNewStream = delta >= WINDOW_SIZE || delta <= NEG_DELTA_TO_TREAT_AS_NEW_STREAM;
        if (isNewStream)
        {
            // clear out everything
            reset();
            // this will be treated as the first packet.
            return insertAndTest(seqNo);
        }

        // if new seqno is higher than mHighestSeqNo, then it won't be a dupe
        if (delta > 0)
        {
            mHighestSeqNo = seqNo;

            // shift up the window by the delta to expire out old packets
            mDupeWindow <<= delta;

            mDupeWindow[0] = true;
            mHighestSeqNo = seqNo;
            return true;
        }

        // if we're here, delta is <= 0.
        delta = -delta;
        if (delta < WINDOW_SIZE)
        {
            // if the bit is set, we've seen it before
            if (mDupeWindow[delta])
                return false;
            // otherwise, set the bit for the packet received.
            mDupeWindow[delta] = true;
            return true;
        }

        // It too old for us to tell if this is a duplicate packet or not.  However
        // we'll let it get dropped.
        return false;
    }

    // Restores to construct-time state
    void reset()
    {
        mDupeWindow.reset();
        mFirstPacket = true;
    }

    uint16_t getHighestSeqNo() const { return mHighestSeqNo; }

private:
    std::bitset<WINDOW_SIZE> mDupeWindow;
    uint16_t mHighestSeqNo;
    bool mFirstPacket;
};

} // namespace bwmgr


#endif // ifndef BWMGR_DUPLICATEPACKETCHECKER_H
