#ifndef __WIN_NETWORK_MONITOR_H
#define __WIN_NETWORK_MONITOR_H

#include <winsock2.h>
#include <ws2def.h>
#include <ws2ipdef.h>
#include <windows.h>
#include <iphlpapi.h>

#include "network_monitor.h"

class WinNetworkMonitor : public NetworkMonitor {
public:
    WinNetworkMonitor(talk_base::Thread* client_thread,
                       talk_base::MessageHandler *client_msg_hndler);
    virtual ~WinNetworkMonitor() {}
    void start();
    void stop();

private:
    static void WINAPI InterfaceChangeCallback(PVOID CallerContext, PMIB_IPINTERFACE_ROW Row,
                                             MIB_NOTIFICATION_TYPE NotificationType);
    void WINAPI handleInterfaceChange(PMIB_IPINTERFACE_ROW Row, MIB_NOTIFICATION_TYPE NotificationType);
    BOOL fetchIfTable();
    void handleInterfaceUp(const NET_LUID& interfaceLuid);
    void handleInterfaceDown(const NET_LUID& interfaceLuid);

    PMIB_IPINTERFACE_TABLE mPipTable;
    CRITICAL_SECTION mCriticalSection;
    HMODULE mLibrary;
    HANDLE mHNotification;
    BOOL mMonitoringSupported;
    BOOL mInitialized;
};
#endif //__WIN_NETWORK_MONITOR_H