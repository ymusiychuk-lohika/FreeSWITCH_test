#include "winUtils.h"
#include "winnetwork_monitor.h"
#include "skinnysipmanager.h"
#include "Iphlpapi.h"

typedef NETIO_STATUS (NETIOAPI_API_ *PNIIC) (ADDRESS_FAMILY, PIPINTERFACE_CHANGE_CALLBACK, PVOID, BOOLEAN, HANDLE);
typedef NETIO_STATUS (NETIOAPI_API_ *PCMCN2) (HANDLE);

WinNetworkMonitor::WinNetworkMonitor(talk_base::Thread* client_thread,
    talk_base::MessageHandler *client_msg_hndler)
    :NetworkMonitor(client_thread,client_msg_hndler),
    mPipTable (NULL),
    mHNotification(NULL),
    mMonitoringSupported(FALSE),
    mInitialized(FALSE)
{
    // initialize critical section
    InitializeCriticalSection(&mCriticalSection);

    if (BJN::isWindowsVistaAndAbove())
    {
        LOG(LS_INFO) << "Network monitoring is supported on this operating system";

        fetchIfTable();
        mMonitoringSupported = TRUE;
    }
    else
    {
        LOG(LS_INFO) << "Network monitoring is not supported on this operating system";
    }
}

void  WinNetworkMonitor::start ()
{
    if (!mMonitoringSupported)
    {
        return;
    }

    if (!mInitialized)
    {
        LOG(LS_ERROR) << "Cannot start Network monitoring, object not initialized";
        return;
    }

    ADDRESS_FAMILY family = AF_UNSPEC;
    PNIIC pNIIC;
    mLibrary = LoadLibraryA("iphlpapi.dll");
    if (!mLibrary)
    {
        LOG(LS_ERROR) << "Failed to load iphlpapi.dll, cannot start monitoring";
        return;
    }

    pNIIC = (PNIIC)GetProcAddress(mLibrary, "NotifyIpInterfaceChange");

    if (pNIIC)
    {
        // register for changes in interface parameters
        if (pNIIC(family, (PIPINTERFACE_CHANGE_CALLBACK)InterfaceChangeCallback,
                  (PVOID)this, TRUE, &mHNotification) != NO_ERROR)
        {
            LOG(LS_ERROR) << "Could not subscribe to notificaiton of IP interface change. Error ["
                << GetLastError() << "]";
        }
    }
}

void  WinNetworkMonitor::stop()
{
    // if we have valid handle that implies subscription was successful, unsubscribe notification
    if (mHNotification)
    {
        PCMCN2 pCMCN2 = (PCMCN2)GetProcAddress(mLibrary, "CancelMibChangeNotify2");
        if (pCMCN2)
        {
            pCMCN2(mHNotification);
        }
    }

    if (mLibrary)
    {
        FreeLibrary(mLibrary);
        mLibrary = NULL;
    }

    DeleteCriticalSection(&mCriticalSection);
    mHNotification = NULL;
    mInitialized = FALSE;
}

//
// If an interface came up, check if no other was up if yes notify to sip thread.
// Ignore Loopback interface.
void WinNetworkMonitor::handleInterfaceUp(const NET_LUID& interfaceLuid)
{
    LOG(LS_INFO) << __FUNCTION__ << " "<< interfaceLuid.Value;
    BOOL allInterfacesWereDown = true;

    // check if there is already an interface which is up, ignore this notification
    for (int i = 0; i < (int) mPipTable->NumEntries; i++)
    {
        if ((mPipTable->Table[i].InterfaceLuid.Info.IfType == IF_TYPE_SOFTWARE_LOOPBACK) ||
            (mPipTable->Table[i].InterfaceLuid.Value == interfaceLuid.Value))
        {
            continue;
        }

        // if there is an interface which is already connected
        if (mPipTable->Table[i].Connected == TRUE)
        {
            allInterfacesWereDown = false;
            break;
        }
    }

    // notify only when all were down and this is a new interface which has come up
    if (allInterfacesWereDown)
    {
        LOG(LS_INFO) << "Post [MSG_NETWORK_INTERFACE_UP] event to SIP thread" << interfaceLuid.Value;
        clientThread->Post(clientMsgHandler, bjn_sky::MSG_NETWORK_INTERFACE_UP);
    }
}

// This method does following. Handles one interface went down,
//    -- if there is none connected, send interface down event to sip thread.
//    -- check if any other is available if yes notify to sip thread (TODO)
void WinNetworkMonitor::handleInterfaceDown(const NET_LUID& interfaceLuid)
{
    LOG(LS_INFO) << __FUNCTION__ << " "<< interfaceLuid.Value;
    BOOL allInterfacesAreDown = true;

    // check if there is already an interface which is up, ignore this notification
    for (int i = 0; i < (int) mPipTable->NumEntries; i++)
    {
        if ((mPipTable->Table[i].InterfaceLuid.Info.IfType == IF_TYPE_SOFTWARE_LOOPBACK) ||
            (mPipTable->Table[i].InterfaceLuid.Value == interfaceLuid.Value))
        {
            continue;
        }

        // if there is an interface which is connected
        if (mPipTable->Table[i].Connected == TRUE)
        {
            allInterfacesAreDown = false;
            // TODO
            // clientThread->Post(clientMsgHandler, bjn_sky::MSG_ONE_INTERFACE_DOWN_OTHER_UP);
            break;
        }
    }

    if (allInterfacesAreDown)
    {
        LOG(LS_INFO) << "Post [MSG_NETWORK_INTERFACE_DOWN] event to SIP thread" << interfaceLuid.Value;
        clientThread->Post(clientMsgHandler, bjn_sky::MSG_NETWORK_INTERFACE_DOWN);
    }
}

void WINAPI WinNetworkMonitor::InterfaceChangeCallback(PVOID CallerContext,
    PMIB_IPINTERFACE_ROW Row,
    MIB_NOTIFICATION_TYPE NotificationType)
{
    WinNetworkMonitor* pWinNM = (WinNetworkMonitor*) CallerContext;

    if (pWinNM)
    {
        pWinNM->handleInterfaceChange(Row, NotificationType);
    }
}

void WINAPI WinNetworkMonitor::handleInterfaceChange(PMIB_IPINTERFACE_ROW Row,
    MIB_NOTIFICATION_TYPE NotificationType)
{
    if (Row != NULL)
    {
        MIB_IPINTERFACE_ROW iface;
        iface.Family = Row->Family;
        iface.InterfaceLuid = Row->InterfaceLuid;
        iface.InterfaceIndex = Row->InterfaceIndex;

        UINT errcode = GetIpInterfaceEntry (&iface);
        if (errcode != NO_ERROR)
        {
            LOG(LS_INFO) << "Interface LUID: [" << Row->InterfaceLuid.Value << "] fetch not succesful";
            return;
        }

        // request ownership of critical section
        EnterCriticalSection(&mCriticalSection);

        for (int i = 0; i < (int) mPipTable->NumEntries; i++)
        {
            // find the interface
            if ((mPipTable->Table[i].InterfaceLuid.Value != iface.InterfaceLuid.Value) ||
                (mPipTable->Table[i].InterfaceLuid.Info.IfType == IF_TYPE_SOFTWARE_LOOPBACK))
            {
                continue;
            }

            // check if there is a change in connected state
            if (mPipTable->Table[i].Connected != iface.Connected)
            {
                mPipTable->Table[i] = iface;

                if (mPipTable->Table[i].Connected)
                {
                    handleInterfaceUp(mPipTable->Table[i].InterfaceLuid);
                }
                else
                {
                    handleInterfaceDown(mPipTable->Table[i].InterfaceLuid);
                }
                break;
            }
        }

        // release ownership of critical section
        LeaveCriticalSection(&mCriticalSection);
    }
}

BOOL WinNetworkMonitor::fetchIfTable()
{
    LOG(LS_INFO) << "start fetching interface table";
    DWORD dwRetVal = GetIpInterfaceTable(AF_UNSPEC, &mPipTable);
    if (dwRetVal != NO_ERROR) {
        LOG(LS_ERROR) << "GetIpInterfaceTable returned. Error: " << dwRetVal;
        mInitialized = false;
        return FALSE;
    }

    mInitialized = true;
    LOG(LS_INFO) << "interface table fetch operation completed";
    return TRUE;
}