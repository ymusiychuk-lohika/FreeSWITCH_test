/******************************************************************************
 * (Yet another class to ) Tracks bitrate and loss over a history window
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: jwallace
 * Date:   Oct 29, 2013
 *****************************************************************************/

#ifndef WINDOWEDBITRATEANDLOSSTRACKER_H_
#define WINDOWEDBITRATEANDLOSSTRACKER_H_

#include <deque>
#include "bwmgr_types.h"

namespace bwmgr
{

// Simple bitrate and loss rate accounting on a stream over a window
class WindowedBitrateAndLossTracker
{
public:
    WindowedBitrateAndLossTracker(uint32_t windowLenInMs, uint32_t numPacketsToTrack);
    ~WindowedBitrateAndLossTracker();

    // Applies details of current packet to the tracker
    void update(const RtpPacketInfo &pktInfo);

    // returns current bitrate of the window in bits per second (valid as of the last
    // packet processed)
    uint32_t getCurrentBitrate() const;

    // Calculates loss within the window.
    uint32_t numLostPackets() const;

    // Returns loss / (loss + numPackets).
    float getLossPercentage() const;

    bool isFull() const
    {
        return mArrivalHistory.size() >= mNumPacketsToTrack;
    }

    // Raw values -- handy for unit tests
    uint32_t numPackets() const { return mSeqHistory.size(); }
    uint32_t numBytes() const { return mPayloadLengthSum; }
    uint64_t timeDelta() const;

    uint16_t minSeq() const { return mMinSeq; }
    uint16_t maxSeq() const { return mMaxSeq; }

    // Clears out out current values
    void reset();

    void pruneOldPackets(uint32_t numToDelete);

private:
    // Subset of RtpPacketInfo that contains only the elems of interest to
    // this class
    struct BitrateElem
    {
        BitrateElem(const RtpPacketInfo &pktInfo)
            : arrivalTimeMs(pktInfo.arrivalTimeMs)
            , rtpSeq(pktInfo.rtpSeq)
            , payloadLength(pktInfo.payloadLength)
        {
        }

        uint64_t arrivalTimeMs;
        uint16_t rtpSeq;
        uint16_t payloadLength;
    };

    // Removes frames older than timeCutoffMs.  Returns num packets removed
    uint32_t purgeOldPackets(uint64_t timeCutoffMs);

    void deleteOldestPacket();

    // Updates the new minimum sequence number for loss tracking
    void updateMinSeq();

    // Removes packets older than the current minimum sequence number
    void pruneSeqHistory();

    std::deque<BitrateElem> mArrivalHistory; // This is a time oriented sequence to track bitrate
    std::deque<uint16_t> mSeqHistory; // This is sequence number oriented to track loss %
    const uint32_t mWindowLenInMs;
    const uint32_t mNumPacketsToTrack;
    uint32_t mPayloadLengthSum;

    // min/max RTP sequence numbers seen within the time window.
    // Note: given the range of these, wraparounds do occur, so it is possible min > max
    // -- use the rtpLess (and related functions) to compare.
    uint16_t mMinSeq;
    uint16_t mMaxSeq;
};

} // namespace bwmgr

#endif // ifndef WINDOWEDBITRATEANDLOSSTRACKER_H_
