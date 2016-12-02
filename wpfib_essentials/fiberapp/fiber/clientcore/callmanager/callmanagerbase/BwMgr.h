//
//  BwMgr.h
//  Acid
//
//  Created by Emmanuel Weber on 5/24/12.
//  Copyright (c) 2012 Bluejeans. All rights reserved.
//
//
///////////////////////////////////////////////////////////////////////////////

#ifndef Acid_BwMgr_h
#define Acid_BwMgr_h
#include <math.h>
#include <limits>

#include <pjsua-lib/pjsua.h>
#include <bwmgr_types.h>
#include <rtpstatsutil.h>
#include "basevideocodec.h"
#include "bandwidthpolicy.h"

#define DEFAULT_VID_BITRATE_KBPS	1200
#define MAX_VID_BITRATE_KBPS	4032
#define STARTUP_VID_BITRATE_KBPS	384
#define DEFAULT_VID_BITRATE_BPS	(DEFAULT_VID_BITRATE_KBPS * 1000)
#define MAX_VID_BITRATE_BPS	(MAX_VID_BITRATE_KBPS * 1000)
#define STARTUP_VID_BITRATE_BPS (STARTUP_VID_BITRATE_KBPS * 1000)

#define MAX_TCP_VID_BITRATE_KBPS           384
#define STARTUP_TCP_VID_BITRATE_KBPS       256
#define MAX_TCP_VID_BITRATE_BPS            (MAX_TCP_VID_BITRATE_KBPS * 1000)
#define STARTUP_TCP_VID_BITRATE_BPS        (STARTUP_TCP_VID_BITRATE_KBPS * 1000)

#if defined(ENABLE_H264_OSX_HWACCL) || defined(ENABLE_H264_WIN_HWACCL)
#define H264_HW_CODEC_STARTUP_TCP_VID_BITRATE_KBPS       300
#define H264_HW_CODEC_STARTUP_TCP_VID_BITRATE_BPS        (H264_HW_CODEC_STARTUP_TCP_VID_BITRATE_KBPS * 1000)
#endif

////////////////////////////////////////////////////////////////////////////////
// forwards

class GracefulEventHandler;
class GracefulDegradationBandwidthMonitor;

namespace bjn_sky
{
    struct MeetingFeatures;
}

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// template RunningWindowedAverageT

#undef min
#undef max

template <class T>
class RunningWindowedAverageT
{
public:
    RunningWindowedAverageT(unsigned int windowSize, bool doMinMax = false)
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

	mMinValue   = std::numeric_limits<T>::max();
        mMaxValue   = std::numeric_limits<T>::min();
    }

    void push(T x)
    {
        T poppedVal=0;

        if (mNumEntries < mWindowSize)
        {
            mNumEntries++;
        }
        else
        {
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
                poppedVal == mMinValue ||
                poppedVal == mMaxValue ||
                x > mMaxValue ||
                x < mMinValue)
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
            for(unsigned int i=0;i<mNumEntries;i++)
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

    unsigned int getNumEntries(void)
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
    unsigned int mWindowSize;
    unsigned int mNumEntries;
    unsigned int mNextEntry;
};

////////////////////////////////////////////////////////////////////////////////

enum
{
    DEFAULT_GD_BW_DURATION_SEC = 45,
    DEFAULT_GD_BW_LOSS_PERCENT_CUTOFF = 10,
};

struct GracefulDegradationBandwidthMonitorConfig
{
    GracefulDegradationBandwidthMonitorConfig()
        : mDurationSec(DEFAULT_GD_BW_DURATION_SEC)
        , mLossPercentCutoff(DEFAULT_GD_BW_LOSS_PERCENT_CUTOFF)
    {
    }

    uint32_t mDurationSec;
    int32_t  mLossPercentCutoff;
};


//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// class BwMgr

class BwMgr
{
public:
    BwMgr();
    
    ~BwMgr();
    
    void update(pj_time_val current, bool presentationMode);
    
    void updateSendRates(int bitrate, bool presentationMode);
    
    void reset(pjsua_call_id callId);
    
    void cleanBitrateMgr();
    
    inline unsigned int getAudioSendBw(void) const
    {
        return mStats[AUDIO_MED_IDX].bwSent;
    }
    
    inline unsigned int getAudioRecvBw(void) const
    {
        return mStats[AUDIO_MED_IDX].bwRecv;
    }
    
    inline unsigned int getVideoSendBw(void) const
    {
        return mStats[VIDEO_MED_IDX].bwSent;
    }
    
    inline unsigned int getVideoRecvBw(void) const
    {
        return mStats[VIDEO_MED_IDX].bwRecv;
    }
    
    inline unsigned int getContentSendBw(void) const
    {
        return mStats[CONTENT_MED_IDX].bwSent;
    }

    inline float getContentSendFPS(void) const
    {
        return mStats[CONTENT_MED_IDX].sendFps;
    }

    inline float getVideoSendFPS(void) const
    {
        return mStats[VIDEO_MED_IDX].sendFps;
    }

    inline float getContentToEncoderFPS(void) const
    {
        return mStats[CONTENT_MED_IDX].deliveredToEncoderFps;
    }

    inline float getVideoToEncoderFPS(void) const
    {
        return mStats[VIDEO_MED_IDX].deliveredToEncoderFps;
    }

    inline float getContentCaptureFPS(void) const
    {
        return mStats[CONTENT_MED_IDX].fromCapturerFps;
    }

    inline float getVideoCaptureFPS(void) const
    {
        return mStats[VIDEO_MED_IDX].fromCapturerFps;
    }

    inline unsigned int getContentRecvBw(void) const
    {
        return mStats[CONTENT_MED_IDX].bwRecv;
    }

    inline unsigned int getAudioRtt(void) const
    {
        return mStats[AUDIO_MED_IDX].rtt;
    }

    inline unsigned int getVideoRtt(void) const
    {
        return mStats[VIDEO_MED_IDX].rtt;
    }
    
    inline float getAudioRecvLoss(void) const
    {
        return mStats[AUDIO_MED_IDX].recvLoss;
    }
    
    inline float getAudioSendLoss(void) const
    {
        return mStats[AUDIO_MED_IDX].sendLoss;
    }
    
    inline float getVideoRecvLoss(void) const
    {
        return mStats[VIDEO_MED_IDX].recvLoss;
    }

    inline float getVideoSendLoss(void) const
    {
        return mStats[VIDEO_MED_IDX].sendLoss;
    }
    
    inline unsigned int getAudioRecvJitter(void) const
    {
        return mStats[AUDIO_MED_IDX].recvJitter;
    }

    inline unsigned int getAudioSendJitter(void) const
    {
        return mStats[AUDIO_MED_IDX].sendJitter;
    }
    
    inline unsigned int getVideoRecvJitter(void) const
    {
        return mStats[VIDEO_MED_IDX].recvJitter;
    }
    
    inline unsigned int getVideoSendJitter(void) const
    {
        return mStats[VIDEO_MED_IDX].sendJitter;
    }
    
    inline pj_time_val lastRtcpUpdate(int med_idx) const
    {
        pj_time_val time = mLastRtcpTS[med_idx]; 
        if(!mFirstRtcpRecv[med_idx]) {
            pj_gettimeofday(&time);
        }
        return time;
    }
    
    inline pj_time_val lastRtpUpdate(int med_idx) const
    {
        return mLastRtpTS[med_idx];
    }

    inline void resetLastRtpUpdate(int med_idx)
    {
        pj_gettimeofday(&mLastRtpTS[med_idx]);
    }

    inline void resetLastRtcpUpdate(int med_idx) {
        mFirstRtcpRecv[med_idx] = false;
        pj_gettimeofday(&mLastRtcpTS[med_idx]);
    }

    void setASTHeaderId(int med_idx, uint8_t hdrId);

    inline void setBitrate(int bitrate)
    {
        mBitRate = bitrate;
        updateVideoStreamTargetRate(bitrate);
    }
    
    inline void setMinBitrate(int bitrate)
    {
        mMinBitrateConfigured = bitrate;
    }

    inline void setMaxBitrate(int bitrate)
    {
        mMaxBitrateConfigured = bitrate;
        updateVideoStreamMaxRate(bitrate);
        if (mDelayMonitorEnabled)
        {
            mBitrateMgr->setMaxBitrate(mMaxBitrateConfigured);
        }
    }

    inline void setIncomingMaxBitrate(int bitrate)
    {
        if (mDelayMonitorEnabled)
        {
            mBitrateMgr->setIncomingMaxBitrate(mMaxBitrateConfigured);
        }
    }

    inline int getMaxBitrate() const
    {
        return mMaxBitrateConfigured;
    }
    
    void setMaxBitrateForCpuLoad(int bitrate);

    // 
    inline int getMaxBitrateForCpuLoad(void) const
    {
        return mMaxBitRateForCpuLoad;
    }

    int getMinBitrateConstraintForCpuLoad() const
    {
        return MIN_CPU_BITRATE;
    }

    inline int getBitrate(void) const
    {
        return mBitRate;
    }

    inline bool needUpdate(void)
    {
        bool update = mNeedUpdate;
        mNeedUpdate = false;
        return update;
    }   

    inline void setNeedUpdate(bool update)
    {
        mNeedUpdate = update;
    }

    void enableDelayMonitor(bool delay);

    void enableGracefulDegradationMonitor(GracefulEventHandler *eventHandler,
                                          const GracefulDegradationBandwidthMonitorConfig &config);

    ////////////////////////////////////////////////////////////////////////////
    // manage stream bandwidth rates through the stream bandwidth manager
    // This manages stream rates
    //      max       - the configured max rate (our max rate)
    //      cpu         the cpu load max rate
    //      flow      - the remote sides indicated max rate
    //      target    - a temporary max rate (determined by network, start, etc)
    //      effective - a min of the maxes
    //      allowed   - the allowed current rate (generally set by use of effective)

    void        updateVideoStreamMaxRate(unsigned rate);
    void        updateContentStreamMaxRate(unsigned rate);
    void        updateVideoStreamTargetRate(unsigned rate);
    void        updateContentStreamTargetRate(unsigned rate);
    void        updateVideoStreamFlowControlRate(unsigned rate);
    void        updateContentStreamFlowControlRate(unsigned rate);
    void        updateVideoStreamCpuLoadRate(unsigned rate);
    unsigned    getVideoStreamAllowedRate(void) const;
    unsigned    getContentStreamAllowedRate(void) const;
    unsigned    getVideoStreamEffectiveMax(void)   const;
    void        setContentPercentage(unsigned percent);
    void        startContentSend(void);
    void        stopContentSend(void);
    void        setContentEnabled(bool enabled);
    bool        contentEnabled(void);

    void        initBandwidthPolicy(const bjn_sky::MeetingFeatures &features);
    void        sendTMMBR(int birate);

protected:
    void sendTMMBN(int birate);
    
    void updatePacketLoss(
            unsigned short  packetSizeAvg,
            double    loss,
            unsigned short  rttMs,
			unsigned long   time);

    void adjustTemporalLayersIfNeeded(int vidIdx,unsigned long time, int direction);
    void getRedAdjustment(unsigned long time, double loss);

    int getBandwidthAdjustmentDirection(double loss, unsigned short rtt);

    int CalcTFRCbps(
        short  avgPackSizeBytes,
        double   packetLoss,
        unsigned short rttMs);

    void onIncomingRTPPacket(unsigned vidIdx, const void *pkt, pj_ssize_t pktLen, pjmedia_rtp_pkt_type pkt_type);

    // passed to pjmedia endpoint and called from media stream
    static void on_bwmgr_update_cb(void* mgr, unsigned idx, void* pkt, pj_ssize_t len, pjmedia_rtp_pkt_type pkt_type);
    static void bitrateMgrLogFunc( void * userData, int logLevel, const char *format, va_list args);

private:
    void onIncomingBitrateChange(uint32_t bitrate);
    void updateStreamStats(unsigned mediaIndex, const pjsua_stream_stat &streamStat, float time);

    class BwMgrHandler : public bwmgr::BwMgrHandler
    {
    public:
        BwMgrHandler(BwMgr &ctx) : mCtx(ctx) {}

    private:
        virtual void OnIncomingBitrateChange(uint32_t bitrate)
        {
            mCtx.onIncomingBitrateChange(bitrate);
        }
        // Not used...
        virtual void OnOutgoingBitrateChange(uint32_t bitrate) {}

        BwMgr &mCtx;
    };

    enum
    {
        MIN_CPU_BITRATE = 384000,
    };

    pjsua_call_id        callId;

    struct StreamStats
    {
        unsigned bwSent;
        unsigned bwRecv;
        unsigned lastByteSent;
        unsigned lastByteRecv;
        float    sendLoss;
        float    recvLoss;
        unsigned sendJitter;
        unsigned recvJitter;
        unsigned sendLostPktCount;
        unsigned recvLostPktCount;
        unsigned sendPktCount;
        unsigned recvPktCount;
        float    fromCapturerFps;
        float    deliveredToEncoderFps;
        float    sendFps;
        int      rtt;
        unsigned lastFramesFromCapturer;
        unsigned lastFramesDeliveredToEncoder;
        unsigned lastFramesSent;
    };
    StreamStats mStats[MAX_MED_IDX];

    pj_time_val lastUpdate;

    int	    mBitRate;
    int	    mMinBitrateConfigured;
    int	    mMaxBitrateConfigured;
    int     mMaxBitRateForCpuLoad;
    int     mRateLevelCount;

    bool    mFrequentLoss;
    bool    mNeedUpdate;
    bool    mOutputDebug;
    bool    mUseSecondaryLossThreshold;
    bool    mProbeForRate;
    bool    mPauseAtIncrement;
    bool    mLastPresentationMode;
    bool    mDelayMonitorEnabled;

    double  mRateIncreaseAmount;

    // Fec autocontol values
    bool                mLayersEnabled;
    double              mLayersLossThreshhold;
    unsigned long       mLayersMaxLossTimeout;
    unsigned long       mLayersLastMaxLossTime;
    unsigned long       mLayersMinLossTimeout;
    unsigned long       mLayersLastMinLossTime;

    // redundant audio values
    bool                mRedEnabled;
    double              mRedLossThreshhold;
    unsigned long       mRedMaxLossTimeout;
    unsigned long       mRedLastMaxLossTime;
    unsigned long       mRedMinLossTimeout;
    unsigned long       mRedLastMinLossTime;
    unsigned            mRedPacketDistance;
    unsigned long       mRedLastPktDistTime;
    unsigned long       mRedPktDistTimeout;

    RunningWindowedAverageT<double>     mLossFrequency;
    RunningWindowedAverageT<double>     mAverageLoss;
    bwmgr::WindowedCorrelationCoeffT<double> mCorrelation;

    // methods and class to manage the bandwidth of specific streams
    BandwidthPolicy mBandwidthPolicy;

    // class that will determine appropriate bitrates by examining packet delayS
    bwmgr::UniquePtr<bwmgr::BwMgrInterface> mBitrateMgr;
    BwMgrHandler mBwMgrHandler;
    bool mUseAdjustedTimeouts;

    bwmgr::UniquePtr<GracefulDegradationBandwidthMonitor> mGDBandwidthMonitor;

    pj_time_val mLastRtpTS[MAX_MED_IDX];
    pj_time_val mLastRtcpTS[MAX_MED_IDX];
    bool        mFirstRtcpRecv[MAX_MED_IDX];

    uint8_t mASTHdrId[MAX_MED_IDX];
};

//
////////////////////////////////////////////////////////////////////////////////

#endif
