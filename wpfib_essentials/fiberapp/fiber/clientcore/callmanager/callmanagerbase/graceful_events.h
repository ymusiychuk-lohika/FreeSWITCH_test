//
//  graceful_events.h
//  
//  Created by Brant on 10/16/14.
//  Copyright (c) 2014 Bluejeans. All rights reserved.
//

#ifndef _BJN_GRACEFUL_EVENTS_H_
#define _BJN_GRACEFUL_EVENTS_H_

#include <pj/config.h>
#ifdef BJN_WEBRTC_VOE
#include "../../../webrtc/webrtc/bjn_audio_events.h"
#elif TARGET_OS_IPHONE
#include <map>
#include <string>
#include <vector>
#define LOCAL_ECHO_PROBABLE_STR "LocalEchoProbable"
#define SPEAKING_WHILE_MUTED_STR "SpeakingWhileMuted"
typedef std::string GracefulEventNotification;
typedef std::vector<GracefulEventNotification> GracefulEventNotificationList;
typedef std::map<GracefulEventNotification, bool> GracefulEventNotificationFilter;
#endif

#define CHOPPY_RX_CLIENT_AUDIO  "ChoppyRxClientAudio"
#define CHOPPY_TX_CLIENT_AUDIO  "ChoppyTxClientAudio"

class GracefulEventHandler
{
public:
    virtual void EmitGracefulEvent(const GracefulEventNotification &event) = 0;
    virtual void EmitGracefulEvents(const GracefulEventNotificationList &eventList) = 0;
protected:
    virtual ~GracefulEventHandler() {}
};

class GracefulEventCollector : public GracefulEventHandler
{
    
public:
    GracefulEventCollector();
    ~GracefulEventCollector();
 
    virtual void EmitGracefulEvent(const GracefulEventNotification &event);
    virtual void EmitGracefulEvents(const GracefulEventNotificationList &eventList);

    void SetGracefulEventsFilter(const GracefulEventNotificationFilter& filter);
    void PopGracefulEvents(GracefulEventNotificationFilter& queue);

private:
    GracefulEventNotificationFilter mGracefulEventsFilter;
    GracefulEventNotificationFilter mGracefulEventsQueue;
};

extern const char *GE_EXCESSIVE_SYSTEM_CPU_LOAD;
extern const char *GE_INBOUND_VIDEO_BANDWIDTH_INSUFFICIENT;
// Event invoked when fiber outbound/denim inbound video bandwidth seems insufficient
extern const char *GE_OUTBOUND_VIDEO_BANDWIDTH_INSUFFICIENT;

#endif
