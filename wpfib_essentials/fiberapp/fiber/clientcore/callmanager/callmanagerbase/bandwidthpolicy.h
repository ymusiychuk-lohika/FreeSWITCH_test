/******************************************************************************
 * Bandwidth policy
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Emmanuel Weber
 * Date:   08/16/2010
 *****************************************************************************/


#ifndef BANDWIDTHPOLICY_H
#define BANDWIDTHPOLICY_H

#include <stdint.h>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////
// class BandwidthPolicy

class BandwidthPolicy
{
public:
    enum StreamType
    {
        AUDIO_SEND   = 0,
        VIDEO_SEND   = 1,
        CONTENT_SEND = 2,
        AUDIO_RECV   = 3,
        VIDEO_RECV   = 4,
        CONTENT_RECV = 5,
        STREAM_NUM   = 6
    };
    
    BandwidthPolicy(uint32_t uMaxSend,uint32_t uMaxRecv);
    void bandwidthPolicyInit(bool videoBwIsSessionBw);
    
    void start(void);
    bool onSendFlowControl(uint32_t index,uint32_t flow);
    bool onRecvFlowControl(uint32_t index,uint32_t flow);
    void initStream(uint32_t index,uint32_t max);
    void resetStream(uint32_t index);
    
    void getPeopleAndContentRecvRates(uint32_t& people, uint32_t& content, bool adjustForContent);
    void startContentRecv(void);
    void stopContentRecv(void);
    
    void getPeopleAndContentSendRates(uint32_t& people, uint32_t& content, bool adjustForContent);
    void startContentSend(void);
    void stopContentSend(void);

    uint32_t getEffectiveMax(uint32_t index)  const;

    inline uint32_t getMaxSend(void) const
    {
        return mMaxSend;
    }

    inline void setMaxSend(uint32_t maxSend)
    {
        mMaxSend        = maxSend;
        mStreams[AUDIO_SEND].mMax      = std::min<uint32_t>(mStreams[AUDIO_SEND].mMax,  mMaxSend);
        mStreams[VIDEO_SEND].mMax      = std::min<uint32_t>(mStreams[VIDEO_SEND].mMax,  mMaxSend);
        mStreams[CONTENT_SEND].mMax    = std::min<uint32_t>(mStreams[CONTENT_SEND].mMax,mMaxSend);
        mStreams[AUDIO_SEND].mTarget   = mStreams[AUDIO_SEND].mMax;
        mStreams[VIDEO_SEND].mTarget   = mStreams[VIDEO_SEND].mMax;
        mStreams[CONTENT_SEND].mTarget = mStreams[CONTENT_SEND].mMax;
        mStreams[VIDEO_SEND].mAllowed   = mStreams[VIDEO_SEND].mMax;
        mStreams[CONTENT_SEND].mAllowed = mStreams[CONTENT_SEND].mMax;
    }

    inline uint32_t getMaxRecv(void) const
    {
        return mMaxRecv;
    }

    inline void setMaxRecv(uint32_t maxRecv)
    {
        mMaxRecv = maxRecv;
        mStreams[AUDIO_RECV].mMax      = std::min<uint32_t>(mStreams[AUDIO_RECV].mMax  ,mMaxRecv);
        mStreams[VIDEO_RECV].mMax      = std::min<uint32_t>(mStreams[VIDEO_RECV].mMax  ,mMaxRecv);
        mStreams[CONTENT_RECV].mMax    = std::min<uint32_t>(mStreams[CONTENT_RECV].mMax,mMaxRecv);
        mStreams[AUDIO_RECV].mTarget   = mStreams[AUDIO_RECV].mMax;
        mStreams[VIDEO_RECV].mTarget   = mStreams[VIDEO_RECV].mMax;
        mStreams[CONTENT_RECV].mTarget = mStreams[CONTENT_RECV].mMax;
        mStreams[VIDEO_RECV].mAllowed   = mStreams[VIDEO_RECV].mMax;
        mStreams[CONTENT_RECV].mAllowed = mStreams[CONTENT_RECV].mMax;
    }

    inline void updateMax(uint32_t index, uint32_t max)
    {
        if (index < STREAM_NUM)
        {
            mStreams[index].mMax        = max;
            mStreams[index].mAllowed    = max;
            mStreams[index].mCur        = max;
            mStreams[index].mTarget     = max;
            mStreams[index].mCpu     = max;
        }
    }

    void updateTarget(uint32_t index, uint32_t max);

    inline void updateCpu(uint32_t index, uint32_t max)
    {
        if (index < STREAM_NUM)
        {
            mStreams[index].mCpu        = max;
        }
    }

    inline uint32_t getMaxAudioSend(void) const
    {
        return mStreams[AUDIO_SEND].mMax;
    }

    inline uint32_t getMaxAudioRecv(void) const
    {
        return mStreams[AUDIO_RECV].mMax;
    }

    inline uint32_t getMaxVideoSend(void) const
    {
        return mStreams[VIDEO_SEND].mMax;
    }

    inline uint32_t getMaxVideoRecv(void) const
    {
        return mStreams[VIDEO_RECV].mMax;
    }

    inline uint32_t getMaxContentSend(void) const
    {
        return mStreams[CONTENT_SEND].mMax;
    }

    inline uint32_t getMaxContentRecv(void) const
    {
        return mStreams[CONTENT_RECV].mMax;
    }
    
    inline uint32_t getMax(uint32_t index)
    {
        return mStreams[index].mMax;
    }
    
    inline uint32_t getFlow(uint32_t index)
    {
        return mStreams[index].mFlow;
    }
    
    inline void setFlow(uint32_t index,uint32_t flow)
    {
        mStreams[index].mFlow = flow;
    }

    inline uint32_t getCur(uint32_t index)
    {
        return mStreams[index].mCur;
    }

    inline void setCur(uint32_t index,uint32_t cur)
    {
        mStreams[index].mCur = cur;
    }

    inline uint32_t getTarget(uint32_t index)
    {
        return mStreams[index].mTarget;
    }
    
    inline uint32_t getAllowed(uint32_t index) const
    {
        return mStreams[index].mAllowed;
    }

    inline void setContentEnabled(bool enabled)
    {
        mContentEnabled = enabled;
    }

    inline bool contentEnabled()
    {
        return mContentEnabled;
    }

    inline void setContentPercentage(uint32_t percent)
    {
        mContentPercentageSet = true;
        mContentPercentage = percent;
    }

    inline uint32_t totalSendBW(void)
    {
        return getFlow(AUDIO_SEND) + getFlow(VIDEO_SEND) + getFlow(CONTENT_SEND);
    }

    inline uint32_t totalRecvBW(void)
    {
        return getFlow(AUDIO_RECV) + getFlow(VIDEO_RECV) + getFlow(CONTENT_RECV);
    }

private:

    struct Stream
    {
        // the three field below are the candidates for the max rate. If more candidates
        // reveal themselves a better structure should be used for determining the max of all
        uint32_t mMax;          // The initial max limit by call rate, configuration, etc
        uint32_t mFlow;         // The limit imposed by a flow control value
        uint32_t mTarget;       // The limit based on various conditions (network, etc)
        uint32_t mCpu;          // The limit based on cpu load
        uint32_t mAllowed;      // Our current bandwidth

        uint32_t mCur;          // Our current bandwidth
    };
   
    uint32_t                    mMaxSend;
    uint32_t                    mMaxRecv;
    uint32_t                    mContentPercentage;
    bool                        mContentPercentageSet;
    bool                        mStarted;
    bool                        mContentEnabled;
    bool                        mTreatVideoBandwidthAsSessionBandwidth;
    Stream                      mStreams[STREAM_NUM];
};

//
///////////////////////////////////////////////////////////////////////////////


#endif // BANDWIDTHPOLICY_H
