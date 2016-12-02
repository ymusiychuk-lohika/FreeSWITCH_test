/******************************************************************************
 * Monitors trends in delay for a given stream in order to emit notifications
 * on delay increases.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Oct 23, 2013
 *****************************************************************************/

#ifndef DELAYMONITOR_H
#define DELAYMONITOR_H

#include <memory>
#include "circularbuffer.h"
#include "delaycollector.h"

namespace bwmgr
{

class Logger;

struct DelayMonitorContext
{
    Logger *mLogger;
    uint32_t mStIncrementWindow;
    uint32_t mMtIncrementWindow;
    uint32_t mLtIncrementWindow;
    float mAlpha;
    SharedPtr<Clock> mClock;

    DelayMonitorContext()
        : mLogger(NULL)
        , mStIncrementWindow(bwmgr::DEFAULT_SHORT_TERM_DELAYMONITOR_WINDOW)
        , mMtIncrementWindow(bwmgr::DEFAULT_MID_TERM_DELAYMONITOR_WINDOW)
        , mLtIncrementWindow(bwmgr::DEFAULT_LONG_TERM_DELAYMONITOR_WINDOW)
        , mAlpha(bwmgr::DEFAULT_DELAY_ALPHA)
    {}
};

class ConsecutiveIncreaseMonitor
{
public:
    ConsecutiveIncreaseMonitor(float alpha, uint32_t window);
    ~ConsecutiveIncreaseMonitor();

    void update(int32_t delayValueMs);
    bool hasDelaySpiked() const { return mDelaySpiked; }
    void resetCount()
    {
        mConsecutiveIncrements = 0;
        // If resetting the count, it'd also make sense to clear the spike flag
        mDelaySpiked = false;
    }

    void reset();

    const CircularBuffer<int32_t> &getValueHistory() const { return mValueHistory; }
    int32_t currentDelayValue() const { return mPrevSmoothedValue; }

private:
    Filter mFilter;
    uint32_t mIncrementWindow;
    int32_t mPrevSmoothedValue;

    uint32_t mConsecutiveIncrements;
    // a value history for diagnostic purposes
    CircularBuffer<int32_t> mValueHistory;
    bool mDelaySpiked;
};

// Monitor delay trends on collected delay between frames for a stream.
class DelayMonitor
{
public:
    enum
    {
        // ranges for the histogram.  In general, I'd expect a bias toward the max.
        MIN_EXPECTED_DELAY = -440, // in ms
        MAX_EXPECTED_DELAY = 1048, // in ms
        HIST_STEP_SIZE = 16
    };

    DelayMonitor(const DelayMonitorContext &ctx);
    ~DelayMonitor();

    // Collects delay on a particular packet.
    void updatePacketDelay(const RtpPacketInfo &pktInfo);

    // Resets running state
    void reset();

    bool hasDelaySpiked() const { return mDelaySpiked; }

    // Read-only access to internals for diagnostic purposes.
    bool hasFrameChanged() const { return mDelayCollector.frameChanged(); }
    int32_t currentDelayValue() const { return mShortTermMonitor.currentDelayValue(); }
    int32_t rawDelayValue() const { return mRawValueHistory.empty() ? 0 : mRawValueHistory.back(); }

        // Tests if a delay value is at least could be described as a "mild" outlier
    bool isDelayMildOutlier(int delayValue) const { return isShortTermIncreaseOutlier(delayValue); }

private:
    enum
    {
        HIST_NUM_BINS = (MAX_EXPECTED_DELAY - MIN_EXPECTED_DELAY) / HIST_STEP_SIZE + 2,

        // Used to see if we're stuck in a very far end of the histogram
        MAX_HISTOGRAM_MEDIAN = MAX_EXPECTED_DELAY - 200,
        MIN_HISTOGRAM_MEDIAN = MIN_EXPECTED_DELAY + 32,

        // For the histogram to be useful, some amount of frames need to be
        // processed.  100 was arbitrarily chosen, but seemed like a good
        // amount of data.
        HISTOGRAM_LEARNING_PERIOD = 100,

        // history window for WindowedRunningMeanAndVariance.  Value was arbitrarily chosen,
        // though some length was desired for value stability
        STDDEV_WINDOW = 60,

        // Don't allow more delay spikes for a brief penalty period.
        DELAY_SPIKE_PENALTY_PERIOD_MS = 3000,

        // Once at least this many frames were processed, perform self-health checks on the histogram.
        HISTOGRAM_SANITY_CHECK = 1024,

        // Use parallel histograms to essentially expire old data.  Effective history is
        // 2*HISTOGRAM_SWAP_INTERVAL
        HISTOGRAM_SWAP_INTERVAL = 4096,
    };

    // Keeps track of delay trends between frames.
    void onCompletedFrame(uint64_t currentTimeMs);

    bool isHistogramInBadShape() const;

    void triggerDelaySpike(uint64_t currentTimeMs);

    // Updates running statistics and histogram
    void updateStatistics(int rawDelayValue, int smoothedDelayValue);
    void updateStdDev(int delayValue);

    bool isHistogramOutlier(int32_t delayValueMs
            , float outlierPercentile
            , float maxBinPercentile) const;

    bool isShortTermHistogramOutlier(int delayValue) const;
    bool isMidTermHistogramOutlier(int delayValue) const;

    bool isShortTermIncreaseOutlier(int32_t delayValueMs) const;
    bool isMidTermIncreaseOutlier(int32_t delayValueMs) const;
    bool isLongTermIncreaseOutlier(int32_t delayValueMs) const;

    void saveRawDelay(int32_t delayValueMs);

    int32_t getMedianForCalculations() const;

    // Helper functions to print out debug log messages to avoid code clutter.
    void logDelaySpike(const char *name, const CircularBuffer<int32_t> &valueHistory) const;
    void logShortTermDelaySpike() const;
    void logMidTermDelaySpike() const;
    void logLongTermDelaySpike() const;

    void logDelayHistogram() const;
    void logSmoothedDelays() const;
    void logRawDelays() const;

    void invalidateRunningStats();

    DelayMonitorContext mContext;
    DelayCollector mDelayCollector;

    // We have three monitors -- the idea is that shorter history ones have stricter qualifications
    // to hit as to avoid false positives, where as the longer term ones tend to be relatively hard to
    // hit, so we use light filtering on inputs.
    ConsecutiveIncreaseMonitor mShortTermMonitor;
    ConsecutiveIncreaseMonitor mMidTermMonitor;
    ConsecutiveIncreaseMonitor mLongTermMonitor; // Long history, less filtering.

    OverlappedHistogram<MIN_EXPECTED_DELAY, MAX_EXPECTED_DELAY, HIST_STEP_SIZE> mDelayHistogram;
    WindowedRunningMeanAndVariance<STDDEV_WINDOW> mRunningStats;

    CircularBuffer<int16_t> mRawValueHistory; // for diagnostics...

    uint64_t mTimeOfLastLogPrint; // For interval delay messages

    // Member vars to cache assorted statistics from mDelayHistogram & mRunningStats
    // to avoid unnecessary recomputation.
    int32_t mMedianDelay;
    float mStdDev;
    float mCurrentDelayPercentile;
    float mCurrentBinDensity;

    uint64_t mNotificationPenaltyExpirationTime;
    bool mIsSuppressingDelaySpikes;

    bool mDelaySpiked;
};
}

#endif // ifndef DELAYMONITOR_H_
