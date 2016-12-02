/******************************************************************************
 * Bandiwdth managment implementation based on TFRC
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Emmanuel Weber
 * Date:   10/24/2011
 *****************************************************************************/

#ifndef BW_MANAGMENT_H
#define BW_MANAGMENT_H

#include <cstdint>
#include <map>
#include <mutex>
#include <string>
#include <loom/platformlib/trace.h>
#include <connector/common/rtpproxy.h>
#include <connector/common/rtpstats.h>
#include <connector/common/delayanalyzer.h>

namespace Connector {

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// template RunningWindowedAverageT

#define     DEFAULT_WINDOWED_AVG_WINDOW_SIZE    10
template <class T>
class RunningWindowedAverageT
{
public:
    RunningWindowedAverageT(uint32_t windowSize = DEFAULT_WINDOWED_AVG_WINDOW_SIZE, bool doMinMax = false)
        : mDoMinMax(doMinMax)
        , mWindowSize(windowSize)
    {
        mWindow = new T[windowSize];
        reset();
    }

    ~RunningWindowedAverageT()
    {
        if(mWindow)
            delete[] mWindow;
    }

    void reset(void)
    {
        mNumEntries = 0;
        mNextEntry  = 0;
        mTotal      = (T)0;

        std::numeric_limits<T> limits;
        mMinValue   = limits.max();
        mMaxValue   = limits.min();
    }

    void push(T x)
    {
        T poppedVal = T();
        bool hasPoppedVal = false;

        if (mNumEntries < mWindowSize)
        {
            mNumEntries++;
        }
        else
        {
            hasPoppedVal = true;
            poppedVal = mWindow[mNextEntry];
            mTotal -= poppedVal;
        }

        mTotal += x;

        mAverage = mTotal / (T)mNumEntries;

        mWindow[mNextEntry++] = x;
        if(mNextEntry == mWindowSize)
        {
            mNextEntry = 0;
        }

        if (mDoMinMax)
        {
            if (mNumEntries == 1 ||
                x > mMaxValue ||
                x < mMinValue ||
                (hasPoppedVal && (poppedVal == mMinValue || poppedVal == mMaxValue)))
            {
                resetMinMax();
            }
        }
    }

    T getAverage(void) const
    {
        return mAverage;
    }

    void resetMinMax(void)
    {
        if(mNumEntries > 0)
        {
            for(uint32_t i=0;i<mNumEntries;i++)
            {
                if (mWindow[i] < mMinValue)
                {
                    mMinValue = mWindow[i];
                }
                if (mWindow[i] > mMaxValue)
                {
                    mMaxValue = mWindow[i];
                }
            }
        }
    }

    void getMinMax(T& minval, T& maxval)
    {
        if (!mDoMinMax)
        {
            resetMinMax();
        }

        minval = mMinValue;
        maxval = mMaxValue;
    }

    uint32_t getNumEntries(void)
    {
        return mNumEntries;
    }

    bool isFull(void)
    {
        return (mNumEntries >= mWindowSize);
    }

private:
    bool mRefresh;

    T*  mWindow;
    T   mMinValue;
    T   mMaxValue;
    T   mAverage;
    T   mTotal;

    bool     mDoMinMax;
    uint32_t mWindowSize;
    uint32_t mNumEntries;
    uint32_t mNextEntry;
};

///////////////////////////////////////////////////////////////////////////////
// bandwidthslot

#define NUM_LOSS_PER_BANDWIDTH_SLOTS     48
#define BANDWIDTH_AMOUNT_PER_SLOT        32000
#define MAX_UNCHANGED_SLOT_LOSS_STATUS   10

class BandwidthSlot {
public:
    BandwidthSlot()
    : mCount(0) {}

    void    setMinLoss(float loss)
    {
        mMinLoss = loss;
    }

    void    loadSlot(float loss)
    {
        mLoss.push(loss);
        mCount++;
    }

    uint32_t getCount()
    {
        return mCount;
    }

    float   getAverageLoss(void)
    {
        return mLoss.getAverage();
    }

    bool    getHasLoss(void)
    {
        return mHasLoss;
    }

protected:

    RunningWindowedAverageT<float>    mLoss;
    float       mMinLoss;
    bool        mHasLoss;
    uint32_t    mCount;
};

///////////////////////////////////////////////////////////////////////////////
// class BWMgrTFRC

enum BwManagementMode {
    eBwManagementModeDelay,
    eBwManagementModeLoss
};
        
class BWMgrTFRC : public Connector::RtpBandwidthManager
{
    public:
        BWMgrTFRC();
        virtual ~BWMgrTFRC();

        virtual void updatePacketLoss(
            uint16_t  packetSizeAvg,
            double    loss,
            uint16_t  rttMs);

        virtual void updatePacketDelay(
            MHDriver::ChannelType type,
            uint32_t    timestamp,
            uint32_t    incomingbw,
            double      incomingloss);
              
        virtual int getBandwidthAdjustmentDirection(double loss, uint16_t rtt);
        
        void setBwManagementMode(BwManagementMode mode);
        BwManagementMode getBwManagementMode(void);

        inline void setMaxBitrate(int32_t bitrate)
        {
            mMaxBitrateConfigured = bitrate;
        }

        inline void setBitrate(int32_t bitrate)
        {
            mBitRate = bitrate;
        }

        inline int32_t getBitrate(void) const
        {
            return mBitRate;
        }

        inline double getConstantLoss(void) const
        {
            return mConstantLoss;
        }

        inline bool needUpdate(void)
        {
            bool update = mNeedUpdate;
            mNeedUpdate = false;
            return update;
        }
        
        inline int32_t getDirection(void)
        {
            return mDirection;
        }
        
        inline uint32_t getRtcpInterval(void) const
        {
            return cRtcpIntervalMs;
        }

        inline void     setStartDelay(uint32_t delay)
        {
            mStartDelay = delay;
        }

        inline void     setGuid(std::string guid)
        {
            mGuid = guid;
        }

        inline void     setMeetingId(std::string id)
        {
            mMeetingId = id;
        }

        inline void     enableLogging(bool log)
        {
            mEnableLog = log;
        }

        // loss per bandwidth array methods. Used to help
        // recognize times of constant loss

        uint32_t getLossPerBandwidthSlot(uint32_t bw);
        float    getLossPerBandwidth(uint32_t bw);
        void     pushLossPerBandwidth(uint32_t bw, float loss = 0.0);
        uint32_t getHighestMinLossPerBandwidth(float minLoss = 0.0);
        uint32_t getHighestLossPerBandwidth(void);
        uint32_t    getCountofLossPerBandwidth(uint32_t bw);
        uint32_t    getCountOfOccupiedLossPerBandwidth(void);
        void        dumpLossPerBandwidth(void);
        
        // Delay analyzer specific code
        void getNewBandwidthByDelay(uint32_t delay);
        int getBandwidthAdjustmentDirectionByDelay(uint32_t delay);
   
    protected:

        int32_t CalcTFRCbps(
            int16_t  avgPackSizeBytes,
            double   packetLoss,
            uint16_t rttMs);

        double    DetectConstantLoss(double loss);

        void dumpDelayAnalyzerStats(
            uint32_t bandwidth,
            double loss,
            int32_t delay,
            uint32_t timestamp);

    private:
        std::string mGuid;
        std::string mMeetingId;
        
        int32_t mBitRate;
        int32_t mLastBitRate;
        int32_t mMinBitrateConfigured;
        int32_t mMaxBitrateConfigured;
        int32_t mDirection;
        int32_t mStartDelay;
        uint32_t mRateLevelCount;
        uint32_t mZeroLossCount;
        uint32_t mZeroLossThreshhold;
        uint32_t mLossVarianceCount;
        uint32_t mSlotVarianceCount;
        uint32_t mLastSlot;
        uint32_t mLockedDownSlot;
        uint32_t mLockedDownRate;
        double  mConstantLoss;
        double  mRateIncreaseAdjustAmount;

        bool    mFrequentLoss;
        bool    mNeedUpdate;
        bool    mUseSecondaryLossThreshold;
        bool    mProbeForRate;
        bool    mPauseAtIncrement;
        bool    mEnableLog;
        
        RunningMeanAndVariance  mLossVariance;
        RunningMeanAndVariance  mSlotVariance;

        Connector::WindowedCorrelationCoeffT<double>	mCorrelation;
        
        WindowedAverageT<double>        mAveragedCorrelation;
        WindowedAverageT<double>        mLossFrequency;

        BandwidthSlot   mLossPerBandwidthSlots[NUM_LOSS_PER_BANDWIDTH_SLOTS];

        static const uint32_t cRtcpIntervalMs;
        
        BwManagementMode    mBwManagementMode;
        DelayAnalyzer       mDelayAnalyzer;
};

//
///////////////////////////////////////////////////////////////////////////////

}

#endif // BW_MANAGMENT_H
