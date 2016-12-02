#ifndef H_NETWORKMONITOR_FACTORY_H
#define H_NETWORKMONITOR_FACTORY_H


#ifdef FB_X11
#include "X11/x11network_monitor.h"
#elif defined FB_WIN
#include "Win/winnetwork_monitor.h"
#elif defined FB_MACOSX
#include "Mac/macnetwork_monitor.h"
#endif

#include "network_monitor.h"

#define WIN_ENABLE_NW_MONITOR 0x0001
#define X11_ENABLE_NW_MONITOR 0x0002
#define MAC_ENABLE_NW_MONITOR 0x0004

class NetworkMonitorFactory
{
public:
    static NetworkMonitor* CreateNetworkMonitor(talk_base::Thread* client_thread,
                                   talk_base::MessageHandler *client_msg_hndler, uint16_t flag) {
#ifdef FB_X11
    if(flag & X11_ENABLE_NW_MONITOR) {
        return new X11NetworkMonitor(client_thread,client_msg_hndler);
    }
    else {
        return new NetworkMonitor(client_thread,client_msg_hndler);
    }

#elif defined FB_WIN
    if(flag & WIN_ENABLE_NW_MONITOR) {
        return new WinNetworkMonitor(client_thread,client_msg_hndler);
    }
    else {
        return new NetworkMonitor(client_thread,client_msg_hndler);
    }
#elif defined FB_MACOSX
    if(flag & MAC_ENABLE_NW_MONITOR) {
        return new MacNetworkMonitor(client_thread,client_msg_hndler);
    }
    else {
        return new NetworkMonitor(client_thread,client_msg_hndler);
    }
#else
    return new NetworkMonitor(client_thread,client_msg_hndler);
#endif
    }
};

#endif

