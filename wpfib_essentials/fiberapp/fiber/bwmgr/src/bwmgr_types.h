/******************************************************************************
 * Assorted types that individually are small enough to not warrant their
 * own header file.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Oct 28, 2013
 *****************************************************************************/

#ifndef BWMGR_TYPES_H
#define BWMGR_TYPES_H

#include <stdint.h>
#include <stdarg.h>
#include <cstddef>
#include <cstring>
#include "noncopyable.h"
#include "smartpointer.h"

namespace bwmgr
{

enum PacketType
{
    NORMAL_RTP,
    RECOVERED_BY_FEC,
    RECOVERED_BY_RTX,
};

// A subset of an RTP header that is relevant to bandwidth management calculations
struct RtpPacketInfo
{
    uint64_t arrivalTimeMs;
    uint32_t rtpTimestamp;
    uint16_t rtpSeq;
    uint16_t payloadLength;
    uint32_t ssrc;
    uint32_t absoluteSendTime;
    bool     hasAbsoluteSendTime;

    RtpPacketInfo()
    {
        // Since at this time, this struct contains just POD types,
        // in which 0 is a reasonable init value
        memset(this, 0, sizeof(*this));
    }
};

enum BwMgrMediaType
{
    BMMGR_MEDIA_AUDIO   = 0x1,
    BMMGR_MEDIA_VIDEO   = 0x2,
    BMMGR_MEDIA_CONTENT = 0x4,
    BMMGR_MEDIA_MASK    = 0x7,
};

typedef void (UserDefinedLogFunc)( void * userData, int logLevel, const char *format, va_list args);

// If a handler is not set, be it incoming or outgoing, the
// BwMgr's needUpdate() (legacy outgoing) and/or incomingNeedUpdate()
// functions may be polled to detect a bitrate change
class BwMgrHandler
{
public:
    virtual ~BwMgrHandler() {}

    virtual void OnIncomingBitrateChange(uint32_t bitrate) = 0;
    virtual void OnOutgoingBitrateChange(uint32_t bitrate) = 0;
};

// An alternate representation of time of seconds, and 1/(2^32) fractions of a second.
struct NtpTime
{
    uint32_t seconds;
    uint32_t fraction;
};

// This is an interface to externally specify a clock used -- this is intended
// allow a unit test framework to provide it's own time source so that it may
// run faster than wall clock time.
class Clock
{
public:
    virtual ~Clock() {}

    // Retrieves a time point in units of milliseconds
    virtual uint64_t GetCurrentTimeMsec() = 0;

    // This doesn't actually track NTP, but it returns the current time in a
    // seconds, fraction format.
    // This is merely a default implementation that uses GetCurrentTimeMsec().
    // You may be better off with a custom implementation, especially if the
    // clock is more precise than milliseconds.
    virtual NtpTime GetCurrentTimeNtp()
    {
        uint64_t msec = GetCurrentTimeMsec();
        NtpTime ntp;
        ntp.seconds = static_cast<uint32_t>(msec / 1000);
        msec %= 1000; // remainder of milliseconds
        // convert the millisecond remainder into Q32
        ntp.fraction = static_cast<uint32_t>((msec << 32) / 1000);
        return ntp;
    }

    // Factory that returns an implementation that wraps the
    // system's monotonic clock source.
    static Clock *Create();
};


// Default values for the context
enum
{
    BWMGR_START_DELAY            = 4,
    BWMGR_MIN_BITRATE_CONFIGURED = 64000,
    DEFAULT_SHORT_TERM_DELAYMONITOR_WINDOW  = 7,
    DEFAULT_MID_TERM_DELAYMONITOR_WINDOW  = 9,
    DEFAULT_LONG_TERM_DELAYMONITOR_WINDOW  = 11,
};
static const float DEFAULT_DELAY_ALPHA = (float)0.24;

class Adjustments
{
public:
    virtual ~Adjustments() {}

    virtual uint32_t GetStartupIntervalMs() = 0;
    virtual uint32_t GetRateProbeIntervalMs() = 0;
    virtual uint32_t GetAdaptToDelayIntervalMs() = 0;
    virtual uint32_t GetAdaptToLossIntervalMs() = 0;
    virtual uint32_t GetBaseMaintainTimeoutMs() = 0;
    virtual uint32_t GetMaxMaintainTimeoutMs() = 0;
    virtual float GetRateProbeIncrement() = 0;
    virtual float GetDelayEventDecrement() = 0;
};

typedef SharedPtr<Adjustments> AdjustmentsPtr;

// Older values that had longer maintain timeouts...
class LegacyBJNAdjustments : public Adjustments
{
public:
    virtual uint32_t GetStartupIntervalMs();
    virtual uint32_t GetRateProbeIntervalMs();
    virtual uint32_t GetAdaptToDelayIntervalMs();
    virtual uint32_t GetAdaptToLossIntervalMs();
    virtual uint32_t GetBaseMaintainTimeoutMs();
    virtual uint32_t GetMaxMaintainTimeoutMs();
    virtual float GetRateProbeIncrement();
    virtual float GetDelayEventDecrement();
};

class StaticBJNAdjustments : public Adjustments
{
public:
    virtual uint32_t GetStartupIntervalMs();
    virtual uint32_t GetRateProbeIntervalMs();
    virtual uint32_t GetAdaptToDelayIntervalMs();
    virtual uint32_t GetAdaptToLossIntervalMs();
    virtual uint32_t GetBaseMaintainTimeoutMs();
    virtual uint32_t GetMaxMaintainTimeoutMs();
    virtual float GetRateProbeIncrement();
    virtual float GetDelayEventDecrement();
};

struct BwMgrContext {

    bool     mEnableIncomingBwMgr;  // use this to disable the
    bool     mEnableOutgoingBwMgr;  // bwmgr state machine

    uint32_t mStartDelay;           // how long before rates start adjusting upward

    uint32_t mMediaTypesForDelay; // A bitmask of allowed media types
    uint32_t mMediaTypesForInLoss;
    uint32_t mMediaTypesForOutLoss;

    uint32_t mStartRate;
    uint32_t mMaxEncodeRate;
    uint32_t mMinEncodeRate;
    uint32_t mMaxDecodeRate;
    uint32_t mMinDecodeRate;

    UserDefinedLogFunc* mLogFunc;
    void*               mLogData;

    BwMgrHandler*       mIncomingHandler;   // leaving the handler pointer NULL does
    BwMgrHandler*       mOutgoingHandler;   // not disable the bwmgr, just the callback

    SharedPtr<Clock> mTimeSource; // Optional -- this allows an externally supplied clock,
                                  // such as for unit tests.

    // Provides a mechanism to override bwmgr timeouts with custom behaviors.  If left null,
    // reasonable static defaults are provided
    SharedPtr<Adjustments> mAdjustments;

    uint32_t mStDelayMonitorWindow;
    uint32_t mMtDelayMonitorWindow;
    uint32_t mLtDelayMonitorWindow;
    float    mDelayAlpha;

    BwMgrContext()
    : mEnableIncomingBwMgr(false)
    , mEnableOutgoingBwMgr(false)
    , mStartDelay(BWMGR_START_DELAY)
    , mMediaTypesForDelay(BMMGR_MEDIA_MASK)
    , mMediaTypesForInLoss(BMMGR_MEDIA_MASK)
    , mMediaTypesForOutLoss(BMMGR_MEDIA_MASK)
    , mStartRate(0)
    , mMaxEncodeRate(0)
    , mMinEncodeRate(BWMGR_MIN_BITRATE_CONFIGURED)
    , mMaxDecodeRate(0)
    , mMinDecodeRate(BWMGR_MIN_BITRATE_CONFIGURED)
    , mLogFunc(0)
    , mLogData(0)
    , mIncomingHandler(0)
    , mOutgoingHandler(0)
    , mStDelayMonitorWindow(DEFAULT_SHORT_TERM_DELAYMONITOR_WINDOW)
    , mMtDelayMonitorWindow(DEFAULT_MID_TERM_DELAYMONITOR_WINDOW)
    , mLtDelayMonitorWindow(DEFAULT_LONG_TERM_DELAYMONITOR_WINDOW)
    , mDelayAlpha(DEFAULT_DELAY_ALPHA)
    {}
};

struct LossReport
{
    double lossRate;
    uint16_t rttMs;
    uint16_t packetSizeAvg;
    uint32_t packetsExpectedInReport;
};

class BwMgrInterface
{
public:

    virtual ~BwMgrInterface() {}

    // this is more for outgoing bw control
    virtual void updatePacketLoss(
        BwMgrMediaType mediaType,
        const LossReport &lossReport) = 0;

    // this is  used for incoming bw control
    virtual void updatePacketDelay(
        BwMgrMediaType mediaType,
        RtpPacketInfo &rtpInfo,
        PacketType pktType) {}

    // Some implementations need to be kicked once in a while to process events.
    // So this should be called every few milliseconds to maybe do some
    // event handling.
    virtual void process(BwMgrMediaType mediaType) {};

    // misc functions called by legacy users of this bwmgr (outgoing only)
    virtual void setMaxBitrate(int32_t bitrate) = 0;
    virtual void setBitrate(int32_t bitrate) = 0;
    virtual int32_t getBitrate(void) const = 0;
    virtual double getConstantLoss(void) const = 0;
    virtual bool needUpdate(void) = 0;

    // misc functions for incoming bwmgr, if any
    virtual void setIncomingMaxBitrate(int32_t bitrate) {}
    virtual void setIncomingMinBitrate(int32_t bitrate) {}
    virtual void setIncomingBitrate(int32_t bitrate) {}
    virtual int32_t getIncomingBitrate(void) const { return 0; }
    virtual double getIncomingConstantLoss(void) const { return 0.0; }
    virtual bool incomingNeedUpdate(void) { return false; }
};

////////////////////////////////////////////////////////////////////////////////
// Functions to create and destroy the bandwidth manager

BwMgrInterface* CreateBwMgr(const BwMgrContext& context);
void DestroyBwMgr(BwMgrInterface* bwmgr);

// Internal details
namespace internal
{
enum
{
    WINDOWED_LOSS_TRACKER_MIN_DURATION_MS = 15000,
    WINDOWED_LOSS_TRACKER_MIN_PKT_HIST = 768,
};

// readonly access to internals intended for diagnositic purposes
struct IncomingBWMState
{
    IncomingBWMState()
        : stateName(NULL)
        , transitionName(NULL)
        , frameChanged(false)
        , delayValue(0)
        , rawDelayValue(0)
        , windowedBitrate(0)
        , windowedLossRate(0)
    {
    }

    const char *stateName;
    const char *transitionName;
    bool frameChanged;
    int32_t delayValue; // valid on frame change
    int32_t rawDelayValue;
    uint32_t windowedBitrate;
    float windowedLossRate;
};

} // namespace bwmgr::internal

} // namespace bwmgr

#endif // ifndef BWMGR_TYPES_H
