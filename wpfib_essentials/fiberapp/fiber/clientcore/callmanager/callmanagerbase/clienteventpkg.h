/******************************************************************************
 * Event Subscription Packages Support
 * 
 * Copyright (c) Bluejeans Network, Inc, All Rights Reserved
 *
 * Author: Arun Krishna
 * Date:   03/25/2013
 ******************************************************************************
 */
#ifndef __CLIENTEVENTPKG_H__
#define __CLIENTEVENTPKG_H__

#include "sipmanagerbase.h"

namespace bjn_sky {

#define EVENT_PKG_DIALOG             "dialog"
#define DEFAULT_EXPIRES_DIALOG_PKG    3600
#define STR_MIME_SUBTYPE_DIALOG      "dialog-info+xml"
#define STR_APP_DIALOG_XML           "application/dialog-info+xml"

//-----------------------------------------------------
// Client event package - RFC4235
//-----------------------------------------------------
class ClientEventPkg : public EventPkg
{
  public:
    ClientEventPkg(std::string event, pjsip_dialog* dlg, pjsip_evsub* evsub, 
                   std::string dialogId, pjsip_role_e role, SubId_t subId,
                   std::string mimeType, std::string mimeSubType, SipManager *sipManager);
    virtual ~ClientEventPkg();

    virtual void AppOnSubscribe(std::string &sub_body, int &resp_code);
    virtual void AppOnUnSubscribe(SubTermReason_e termination_reason, bool &auto_notify);
    virtual void AppOnNotify(std::string &not_body, SubTermReason_e termination_reason);
  protected:
    SipManager *sm;
};

//-----------------------------------------------------
// Client Event Factory Method
//-----------------------------------------------------
class ClientEventFactory : public EventFactory
{
  public:
    ClientEventFactory() {}
    virtual ~ClientEventFactory() {}

    virtual EventPkg* Create(std::string event, pjsip_dialog* dlg, pjsip_evsub* evsub,
                             std::string dlgId, pjsip_role_e role, SubId_t subId,
                             std::string mimeType, std::string mimeSubType, SipManager *sipManager)
    {
        return new ClientEventPkg(event, dlg, evsub, dlgId, role, subId, mimeType, mimeSubType, sipManager);
    }
};

extern ClientEventFactory clientEvtFactory_Inst;

} 

#endif // __CLIENTEVENTPKG_H__
