/******************************************************************************
 * Event Subscription Packages Support
 * 
 * Copyright (c) Bluejeans Network, Inc, All Rights Reserved
 *
 * Author: Arun Krishna
 * Date:   03/25/2013
 ******************************************************************************
 */
#ifndef __EVENTPKG_H__
#define __EVENTPKG_H__

#include <string>
#include <pjsip.h>
#include <pjsip_simple.h>
#include <pjlib-util.h>
#include <pjlib.h>
#include <pj/types.h>
#include <stdint.h>

namespace bjn_sky {
class SipManager;
#define EXPIRES_NONE    0
extern const pj_str_t EVENT_STR_EVENT;
extern const pj_str_t EVENT_STR_APPLICATION;
extern const pj_str_t EVENT_STR_TERMINATED;
    
typedef uint32_t SubId_t;

#define REASON_CLIENT_TERMINATED "client-terminated"
#define REASON_SERVER_TERMINATED "server-terminated"
#define REASON_REFRESH_TIMEOUT   "refresh-timeout"
#define REASON_NOT_KNOWN           "unknown"

typedef enum 
{
    SUB_TERMINATION_NONE,
    SUB_TERMINATION_CLIENT,
    SUB_TERMINATION_SERVER,
    SUB_TERMINATION_TIMEOUT,
    SUB_TERM_MAX
} SubTermReason_e;

//-------------------------------------------------------
// Base class for all event packages implementation
// Application to provide callbacks
//-------------------------------------------------------
class EventPkg
{
  public:
    EventPkg(std::string event, pjsip_dialog* dlg, pjsip_evsub* evsub, 
             std::string dialogId, pjsip_role_e role, SubId_t subId,
             std::string mimeType, std::string mimeSubType);
    virtual ~EventPkg() {}

    void EventPkgOnSubscribe(pjsip_rx_data *rdata, int *p_st_code, pjsip_msg_body **p_body);
    void EventPkgOnNotify(pjsip_rx_data *rdata);

    void EventPkgDoSubscribe(uint32_t expires, std::string body);
    void EventPkgDoNotify(std::string body);

    void EventPkgDoClientRefresh();
    void EventPkgOnTimeout();
    void EventPkgOnTerminate(std::string body);
    void EventPkgOnSubscriptionStateChange(pjsip_evsub_state state);
    bool EventPkgIsDone() { return mDone; }

    //////////////////////////////
    // Application callbacks
    /////////////////////////////

    // AK : TODO : 
    //   1. Add ability for app to attach subscribe resp body
    //   2. Pass the mime type/subtype as the argument for app
    virtual void AppOnSubscribe(std::string &sub_body, int &resp_code)=0; 
    virtual void AppOnUnSubscribe(SubTermReason_e termination_reason, bool &auto_notify)=0;

    virtual void AppOnNotify(std::string &not_body, SubTermReason_e termination_reason)=0;

    std::string   GetEventName()       { return mEvent; }
    pjsip_role_e  GetRole()            { return mRole; }
    std::string   GetDialogId()        { return mDialogId; } 
    SubId_t       GetSubscriptionId()  { return mSubId; }

  protected:
    void SendSubscribe(std::string body);
    void SendNotify(std::string body);
    void SetState(pjsip_evsub_state state);
    void SetReason(SubTermReason_e reason);
    const char* GetReasonStr(SubTermReason_e reason);
    char* pjStrToChar(pj_str_t* str);

    std::string       mEvent;
    pjsip_dialog*     mDialog;
    pjsip_evsub*      mSub;
    pjsip_evsub_state mState;
    std::string       mDialogId;
    pjsip_role_e      mRole;
    SubId_t           mSubId;
    uint32_t          mExpires;

    SubTermReason_e   mReason;

    bool              mDone;
    pj_str_t          mMIME_TYPE;
    pj_str_t          mMIME_SUBTYPE;
    char*             recvbody;
};

//--------------------------------------------------------
// Abstract Event Factory Method
//--------------------------------------------------------
class EventFactory
{
  public:
    EventFactory() {}
    virtual ~EventFactory() {}

    virtual EventPkg* Create(std::string event, pjsip_dialog* dlg, pjsip_evsub* evsub,
                             std::string dlgId, pjsip_role_e role, SubId_t subId,
                             std::string mimeType, std::string mimeSubType, SipManager *sipManager)=0;
};

} 

#endif // __EVENTPKG_H__
