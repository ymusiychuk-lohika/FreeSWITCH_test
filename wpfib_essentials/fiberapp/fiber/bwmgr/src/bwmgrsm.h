/******************************************************************************
 * State Machine header file
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Oct 23, 2013
 *****************************************************************************/

#ifndef BWMGRSM_H_
#define BWMGRSM_H_

#include "stateMachine.h"

namespace bwmgr {

#define     BWMGR_MAINTAIN_RATE_INCREMENT   0.0

struct SMActionHandler {
    virtual void moveActiveToSafeBw(void) = 0;
    virtual void moveSafeToActiveBw(void) = 0;
    virtual void setBwIncrement(double increment) = 0;
    virtual void updateActiveBwByIncrement(void) = 0;
    virtual void decrementActiveBwByIncrement(void) = 0;
    virtual void updateActiveBwByLoss(void) = 0;
    virtual void setTimerInMsecs(unsigned timeout) = 0;
    virtual void setTimerInMsecsBasedOnActiveBitrate() = 0;
    virtual void updateActiveBitrateTimeout() = 0;
    virtual void clearTimer(void) = 0;

    virtual AdjustmentsPtr getAdjustments() = 0;
};

struct BwMgrState : public State
{
    BwMgrState(const std::string& smName, Logger &logger, const std::string& name);

    const char* getSMName(void) const;

private:
    std::string mSMName;
};

struct StartUp : public BwMgrState
{
    StartUp(const std::string& smName, Logger &logger);

private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

struct FirstDelay : public BwMgrState
{
    FirstDelay(const std::string& smName, Logger &logger);

private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

struct FirstLoss : public BwMgrState
{
    FirstLoss(const std::string& smName, Logger &logger);

private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

struct AdaptToLoss :  public BwMgrState
{
    AdaptToLoss(const std::string& smName, Logger &logger);
    
private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

struct AdaptToDelay : public BwMgrState
{
    AdaptToDelay(const std::string& smName, Logger &logger);

private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

struct RateProbe :  public BwMgrState
{
    RateProbe(const std::string& smName, Logger &logger);

private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

struct SlowProbe :  public BwMgrState
{
    SlowProbe(const std::string& smName, Logger &logger);

private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

struct Maintain :  public BwMgrState
{
    Maintain(const std::string& smName, Logger &logger);

private:
    void onEntry(void* data, unsigned event);
    void onExit(void* data, unsigned event);
};

///////////////////////////////////////////////////////////////////////////////////
// BwMgrSM

class BwMgrSM :
    public StateMachine
{
public:

    BwMgrSM(std::string name, SMActionHandler* handler);
    ~BwMgrSM();

    // Events
    void    Loss(void);
    void    Delay(void);
    void    Timeout(void);
    void    MaxRate(void);

    // sets current state to 'StartUp' state
    void    reset(void);

    // Logging
    void setLoggingInfo(
            void* data,
            UserDefinedLogFunc* func);

    static const char* getEventStr(unsigned event);

protected:

    static TransitionAction moveActiveToSafeBw;
    static TransitionAction moveSafeToActiveBw;

protected:
    Logger          mLog;

    // Allocate the states.
    StartUp         mStartup;
    FirstDelay      mFirstDelay;
    FirstLoss       mFirstLoss;
    AdaptToLoss     mAdaptToLoss;
    AdaptToDelay    mAdaptToDelay;
    RateProbe       mRateProbe;
    Maintain        mMaintain;

    SMActionHandler* mHandler;
};

///////////////////////////////////////////////////////////////////////////////////
// BwMgrSMIncoming - for managing incoming media
class IncomingBwMgrSM : public BwMgrSM
{
public:

    IncomingBwMgrSM(SMActionHandler* handler);
};

///////////////////////////////////////////////////////////////////////////////////
// BwMgrSMIncoming - for managing incoming media
class OutgoingBwMgrSM : public BwMgrSM
{
public:

    OutgoingBwMgrSM(SMActionHandler* handler);
};

}

#endif /* BWMGRSM_H_ */

