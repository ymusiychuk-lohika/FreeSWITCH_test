/******************************************************************************
 * Generic State Machine header file
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * Author: Brian Ashford
 * Date:   Oct 23, 2013
 *****************************************************************************/

#ifndef BJNSM_H_
#define BJNSM_H_

#include <list>
#include <string>
#include <vector>

#include "logger.h"

namespace bwmgr {

typedef void(TransitionAction)(void* data);

class State;

struct Transition {
    Transition()
    : mNextState(0)
    , mAction(0)
    , mContext(0) {}
    
    State*  mNextState;
    TransitionAction* mAction;
    void*   mContext;
};


typedef std::list<void*> StateList;

class StateMachine;

class State
{
public:
    State(Logger &logger, std::string name, unsigned numTransitions)
    : mLog(logger)
    , mName(name)
    , mTransitions(numTransitions) {}

    virtual ~State() {}

    virtual void onEntry(void* context, unsigned event) {}

    virtual void onExit(void* context, unsigned event) {}

    void addTransition(unsigned event, State* next, TransitionAction* action = 0, void* context = 0)
    {
        if (event < mTransitions.size())
        {
            mTransitions[event].mNextState = next;
            mTransitions[event].mAction = action;
            mTransitions[event].mContext = context;
        }
    }

    Transition* operator[](unsigned index)
    {
        Transition* retval = 0;
        if (index < mTransitions.size())
        {
            retval = &mTransitions[index];
        }
        return retval;
    }

    const char* getName(void) const
    {
        return mName.c_str();
    }

protected:
    Logger&                 mLog;

private:
    std::string             mName;
    std::vector<Transition> mTransitions;
};

class StateMachine {
public:
    StateMachine(const std::string& name, void* data)
    : mName(name)
    , mCurrentState(0)
    , mContext(data)
    , mLastEvent(INVALID_EVENT)
    {}

    void setCurrentState(State* state)
    {
        mCurrentState = state;
    }
    
    State* getState(void)
    {
        return mCurrentState;
    }

    const char* getStateName(void) const
    {
        if (mCurrentState)
        {
            return mCurrentState->getName();
        }
        else
        {
            return "<None>";
        }
    }

    const char* getSMName(void) const
    {
        return mName.c_str();
    }

    void postEvent(unsigned event)
    {
        if (mCurrentState)
        {
            mLastEvent = event;
            Transition* transition = (*mCurrentState)[event];
            if (transition->mNextState)
            {
                mCurrentState->onExit(mContext, event);
                if (transition->mAction)
                {
                    (transition->mAction)(transition->mContext);
                }
                mCurrentState = transition->mNextState;
                mCurrentState->onEntry(mContext, event);
            }
        }
    }

    void clearEventCode()
    {
        mLastEvent = INVALID_EVENT;
    }

    unsigned getLastEventCode() const
    {
        return mLastEvent;
    }

private:
    enum { INVALID_EVENT = ~0 };
    std::string mName;
    State*      mCurrentState;
    void*       mContext;
    unsigned    mLastEvent;
};
}

#endif /* BJNSM_H_ */

