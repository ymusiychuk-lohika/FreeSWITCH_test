/******************************************************************************
 * Subscription Factory/Manager - Handles both Invite based & OOD subscriptions
 * 
 * Copyright (c) Bluejeans Network, Inc, All Rights Reserved
 *
 * Author: Arun Krishna
 * Date:   03/25/2013
 ******************************************************************************
 */
#ifndef __SUBSCRIPTIONFACTORY_H__
#define __SUBSCRIPTIONFACTORY_H__

#include <map>
#include "sipmanagerbase.h"
#include "eventpkg.h"

namespace bjn_sky {

#define Subscription_Factory (SubscriptionFactory::GetFactory())

class PkgRegistration
{
  public:
    PkgRegistration()
                  : mDefaultExpires(0)
                  , mEvtFactory(NULL)
    { }

    PkgRegistration(std::string name, uint32_t defExpires, std::string mimeType, 
                    std::string mimeSubType, EventFactory* evtFactory) 
                  : mName(name)
                  , mDefaultExpires(defExpires)
                  , mMimeType(mimeType)
                  , mMimeSubType(mimeSubType)
                  , mEvtFactory(evtFactory) {}
    ~PkgRegistration() {};

    std::string                   mName;
    uint32_t                      mDefaultExpires;
    std::string                   mMimeType;
    std::string                   mMimeSubType;
    EventFactory*                 mEvtFactory;
};

class SubscriptionFactory
{
  public:
    static SubscriptionFactory* CreateFactory(pjsip_endpoint *endpt);
    static SubscriptionFactory* GetFactory()  { return mInstance; };
    static void DeleteFactory(pjsip_endpoint *endpt);
    void   RegisterEventPackage(std::string name, uint32_t def_expires, 
                                std::string mimeType, std::string mimeSubType, 
                                EventFactory *eventFactory);

    SubId_t HandleUASSubscription(std::string event_name, pjsip_dialog *dlg, 
                                  std::string dialog_id, pjsip_rx_data *rdata, SipManager *sipManager);
    SubId_t CreateUACSubscription(std::string event_name, uint32_t expires, pjsip_dialog *dlg, 
                                  std::string dialog_id, std::string body, SipManager *sipManager);
   void TerminateSubscriptionBySubId(SubId_t subId, std::string body, bool waitFinalNotify=false);
    void TerminateSubscriptionByDialogId(std::string dialog_id, bool waitFinalNotify=false);
    void TerminateSubscriptionByEventName(std::string event_name, std::string dialog_id,
                                          bool waitFinalNotify=false);

    void SendNotification(SubId_t subId, std::string body);

    // Subscription callbacks from core evsub module
    static void subscription_factory_on_state_change(pjsip_evsub *sub, pjsip_event *event);
    static void subscription_factory_on_tsx_state_change(pjsip_evsub *sub, pjsip_transaction *tsx, 
                                                         pjsip_event *event);
    static void subscription_factory_on_rx_subscribe(pjsip_evsub *sub, pjsip_rx_data *rdata, 
                                                     int *p_st_code, pj_str_t **p_st_text, 
                                                     pjsip_hdr *res_hdr, pjsip_msg_body **p_body);
    static void subscription_factory_on_rx_notify(pjsip_evsub *sub, pjsip_rx_data *rdata,
                                                  int *p_st_code, pj_str_t **p_st_text,
                                                  pjsip_hdr *res_hdr, pjsip_msg_body **p_body);
    static void subscription_factory_on_client_refresh(pjsip_evsub *sub);
    static void subscription_factory_on_server_timeout(pjsip_evsub *sub);

  protected:
    SubscriptionFactory();
    ~SubscriptionFactory();
    
    EventPkg* CreateEventPackage(std::string name, pjsip_dialog* dlg, 
                                 pjsip_evsub* sub, std::string dlgId, 
                                 pjsip_role_e role, SubId_t subId, SipManager *sipManager);
    uint32_t GetDefaultExpires(std::string name);

    void InsertSubscription(std::string dlgId, EventPkg* eventPkg);
    EventPkg* FindSubscription(std::string event_name, std::string dlgId, pjsip_role_e role);
    EventPkg* FindSubscription(SubId_t subId);
    
    void DeleteSubscription(std::string dlgId); // Deletes all subscriptions under the dialog
    void DeleteSubscription(std::string dlgId, std::string event_name); // Deletes all matching event subscriptions
    void DeleteSubscription(SubId_t subId);     // Deletes an unique subscription instance
    void DeleteSubscription();                  // Deletes all subscription - wildcarded
    
    SubId_t AllocateSubId() { return ++mLastAssignedSubId; }

    static SubscriptionFactory                      *mInstance;

    typedef std::multimap<std::string, EventPkg*>   SubscriptionsMap;
    typedef SubscriptionsMap::iterator              SubscriptionsMapIt;
    SubscriptionsMap                                mSubscriptions;

    SubId_t                                         mLastAssignedSubId;

    typedef std::map<std::string, PkgRegistration>  RegistrationMap;
    typedef RegistrationMap::iterator               RegistrationMapIt;
    RegistrationMap                                 mPkgRegistrations;

};

}

#endif // __SUBSCRIPTIONFACTORY_H__
