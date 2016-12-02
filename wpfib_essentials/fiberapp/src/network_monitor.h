#ifndef H_NETWORK_MONITOR_H_
#define H_NETWORK_MONITOR_H_

#include "talk/base/messagequeue.h"
#include "talk/base/thread.h"

class NetworkMonitor {
  public:
    NetworkMonitor(talk_base::Thread* client_thread,
                       talk_base::MessageHandler *client_msg_hndler) 
    {
       clientThread = client_thread;
       clientMsgHandler = client_msg_hndler;
    }
    virtual ~NetworkMonitor() {}
    virtual void start() {}
    virtual void stop() {isRunning = false; return;}
  
  protected:
    talk_base::Thread *clientThread;
    talk_base::MessageHandler *clientMsgHandler;
    bool isRunning;
};
#endif
