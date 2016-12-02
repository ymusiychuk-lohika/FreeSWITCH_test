#include "winUtils.h"
#include <iphlpapi.h>
#include <powrprof.h>
#include <wlanapi.h>
#pragma warning( push )
#pragma warning( disable : 4005 )
#include <Wincodec.h>
#pragma warning( pop )
#include "base64.h"
#include <atlbase.h>
#include <atlconv.h>
#pragma comment(lib, "wlanapi.lib")
#pragma comment(lib, "IPHLPAPI.lib")

namespace BJN
{
    void invokeBrowserFullScreenShortcut()
    {
        keybd_event(VK_F11,MapVirtualKey(VK_F11,0),0,0);
        keybd_event(VK_F11,MapVirtualKey(VK_F11,0),KEYEVENTF_KEYUP,0);
    }

    void simulateMouseMove(int x, int y)
    {
        INPUT mouseInput;
        memset(&mouseInput, 0, sizeof(mouseInput));
        mouseInput.type = INPUT_MOUSE;
        mouseInput.mi.dwFlags = MOUSEEVENTF_MOVE;
        mouseInput.mi.dx = x;
        mouseInput.mi.dy = y;
        mouseInput.mi.mouseData = 0;
        mouseInput.mi.time = 0;
        mouseInput.mi.dwExtraInfo = 0;
        SendInput(1, &mouseInput, sizeof(mouseInput));
    }

    int getNameServer(std::vector<std::string> &nameServer)
    {
        FIXED_INFO *pFixedInfo;
        ULONG ulOutBufLen;
        DWORD dwRetVal;
        IP_ADDR_STRING *pIPAddr;

        pFixedInfo = (FIXED_INFO *) malloc(sizeof (FIXED_INFO));
        if (pFixedInfo == NULL) {
            return 1;
        }
        ulOutBufLen = sizeof (FIXED_INFO);

        // Make an initial call to GetAdaptersInfo to get
        // the necessary size into the ulOutBufLen variable
        if (GetNetworkParams(pFixedInfo, &ulOutBufLen) == ERROR_BUFFER_OVERFLOW) {
            free(pFixedInfo);
            pFixedInfo = (FIXED_INFO *) malloc(ulOutBufLen);
            if (pFixedInfo == NULL) {
                return 1;
            }
        }

        if (dwRetVal = GetNetworkParams(pFixedInfo, &ulOutBufLen) == NO_ERROR) {
            nameServer.push_back(pFixedInfo->DnsServerList.IpAddress.String);
            pIPAddr = pFixedInfo->DnsServerList.Next;
            while(pIPAddr)
            {
                nameServer.push_back(pIPAddr->IpAddress.String);
                pIPAddr = pIPAddr->Next;
            }
        } else {
            return 1;
        }

        if (pFixedInfo)
            free(pFixedInfo);

        return 0;
    }

    //void ClientToScreen(HWND *win, BJNPoint &cursorPos)
    //{
    //    ::ClientToScreen(hwin, (LPPOINT)&cursorPos);
    //}

    const int kMaxPowerSchemeNameLen = 256;
    struct ActivePowerData {
        UINT    index;
        WCHAR   name[kMaxPowerSchemeNameLen];
		bool	found;
    };

    // enumeration callback for XP power scheme name
    BOOLEAN CALLBACK PowerSchemeEnumProcXP(UINT Index,
        DWORD NameSize, LPTSTR Name,
        DWORD DescriptionSize, LPTSTR Description,
        PPOWER_POLICY Policy, LPARAM Context)
    {
        ActivePowerData* data = (ActivePowerData*) Context;
        if (Index == data->index)
        {
            // found the one we want, copy name
            wcsncpy(data->name, Name, _countof(data->name));
			// indicate success
			data->found = true;
            // return FALSE to stop the enumeration
            return FALSE;
        }
        // keep enumerating
        return TRUE;
    }

    // typedefs for the Vista and above power APIs
    typedef DWORD (WINAPI*PowerGetActiveSchemeFunc)(HKEY UserRootPowerKey,
        GUID **ActivePolicyGuid);
    typedef DWORD (WINAPI*PowerReadFriendlyNameFunc)(HKEY RootPowerKey,
        const GUID *SchemeGuid,
        const GUID *SubGroupOfPowerSettingsGuid,
        const GUID *PowerSettingGuid,
        PUCHAR Buffer,
        LPDWORD BufferSize);

    bool LogCurrentPowerSchemeVistaAndAbove()
    {
        // this code has to run on XP so we have to
        // dynamically link to Vista and above APIs
        bool found = false;
        WCHAR name[kMaxPowerSchemeNameLen] = { 0 };

        HMODULE hPowrprof = LoadLibrary(L"powrprof.dll");
        if (hPowrprof != NULL)
        {
            PowerGetActiveSchemeFunc pGetActive =
                (PowerGetActiveSchemeFunc)GetProcAddress(hPowrprof, "PowerGetActiveScheme");
            PowerReadFriendlyNameFunc pGetFriendlyName =
                (PowerReadFriendlyNameFunc)GetProcAddress(hPowrprof, "PowerReadFriendlyName");
            if (pGetActive && pGetFriendlyName)
            {
                DWORD len = sizeof(name);
                GUID* pGuid = NULL;
                if (pGetActive(NULL, &pGuid) == ERROR_SUCCESS)
                {
                    if (pGetFriendlyName(NULL, pGuid, NULL, NULL,
                        (PUCHAR) name, &len) == ERROR_SUCCESS)
                    {
                        found = true;
                    }
                    LocalFree(pGuid);
                }
            }
            FreeLibrary(hPowrprof);
        }

        if (found)
        {
            LOG(LS_INFO) << "Active power scheme: " << CW2A(name);
        }
        else
        {
            LOG(LS_INFO) << "Failed to determine the active power scheme";
        }
        return found;
    }

    void LogCurrentPowerSchemeXP()
    {
        UINT activeIndex = 0;
        if (GetActivePwrScheme(&activeIndex))
        {
            ActivePowerData data = { 0 };
            data.index = activeIndex;
			data.found = false;
            EnumPwrSchemes(&PowerSchemeEnumProcXP, (LPARAM) &data);
			// EnumPwrSchemes can return FALSE when we exit the enum
			// early so we rely on our found variable to indicate success
			if (data.found)
            {
                LOG(LS_INFO) << "Active power scheme: " << CW2A(data.name);
                return;
            }
        }
        LOG(LS_INFO) << "Failed to determine the active power scheme";
    }

    void LogCurrentPowerScheme()
    {
        // check which OS version we are running on
        OSVERSIONINFO info = { 0 };
        info.dwOSVersionInfoSize = sizeof(info);
        GetVersionEx(&info);
        if (info.dwMajorVersion >= 6)
        {
            // Vista and above
            LogCurrentPowerSchemeVistaAndAbove();
        }
        else
        {
            // XP and below
            LogCurrentPowerSchemeXP();
        }
    }

    std::string ConvertImageToPNGBase64(uint8* buffer, int width, int height)
    {
        std::string base64PNG;
        IWICImagingFactory *piFactory = NULL;
        IWICBitmapEncoder *piEncoder = NULL;
        IWICBitmapFrameEncode *piBitmapFrame = NULL;
        IWICBitmapScaler *piScaler = NULL;
        IStream *pMemStream = NULL;
        IWICStream *piStream = NULL;

        HRESULT hr = CoCreateInstance(
                        CLSID_WICImagingFactory,
                        NULL,
                        CLSCTX_INPROC_SERVER,
                        IID_IWICImagingFactory,
                        (LPVOID*) &piFactory);

        if (SUCCEEDED(hr))
        {
            hr = CreateStreamOnHGlobal(NULL, TRUE, &pMemStream);
        }

        if (SUCCEEDED(hr))
        {
            hr = piFactory->CreateStream(&piStream);
        }

        if (SUCCEEDED(hr))
        {
            hr = piStream->InitializeFromIStream(pMemStream);
        }

        if (SUCCEEDED(hr))
        {
           hr = piFactory->CreateEncoder(GUID_ContainerFormatPng, NULL, &piEncoder);
        }

        if (SUCCEEDED(hr))
        {
            hr = piEncoder->Initialize(piStream, WICBitmapEncoderNoCache);
        }

        if (SUCCEEDED(hr))
        {
            hr = piEncoder->CreateNewFrame(&piBitmapFrame, NULL);
        }

        if (SUCCEEDED(hr))
        {      
            hr = piBitmapFrame->Initialize(NULL);
        }

        if (SUCCEEDED(hr))
        {
            hr = piFactory->CreateBitmapScaler(&piScaler);
        }

        if (SUCCEEDED(hr))
        {
            hr = piBitmapFrame->SetSize(width, height);
        }

        // save as 24 bit rather than 32 to save memory
        //WICPixelFormatGUID formatGUID = GUID_WICPixelFormat24bppBGR;
        WICPixelFormatGUID formatGUID = GUID_WICPixelFormat32bppBGRA;
        if (SUCCEEDED(hr))
        {
            hr = piBitmapFrame->SetPixelFormat(&formatGUID);
        }

        if (SUCCEEDED(hr))
        {
            // We're expecting to write out 24bppRGB. Fail if the encoder cannot do it.
            //hr = IsEqualGUID(formatGUID, GUID_WICPixelFormat24bppBGR) ? S_OK : E_FAIL;
            hr = IsEqualGUID(formatGUID, GUID_WICPixelFormat32bppBGRA) ? S_OK : E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            //UINT cbStride = (width * 24 + 7)/8/***WICGetStride***/;
            UINT cbStride = (width * 32 + 7)/8/***WICGetStride***/;
            UINT cbBufferSize = height * cbStride;

            hr = piBitmapFrame->WritePixels(height, cbStride, cbBufferSize, buffer);
        }

        if (SUCCEEDED(hr))
        {
            hr = piBitmapFrame->Commit();
        }    

        if (SUCCEEDED(hr))
        {
            hr = piEncoder->Commit();
        }

        if (SUCCEEDED(hr))
        {
            if (pMemStream)
            {
                STATSTG stat = { 0 };
                hr = pMemStream->Stat(&stat, STATFLAG_NONAME | STATFLAG_NOOPEN);
                if (SUCCEEDED(hr))
                {
                    // size is cbSize, assume less than 4GB!
                    ULONG dataSize = (ULONG) stat.cbSize.LowPart;
                    BYTE* data = new BYTE[dataSize];
                    if (data)
                    {
                        LARGE_INTEGER startPos = { 0 };
                        pMemStream->Seek(startPos,  STREAM_SEEK_SET, NULL);
                        ULONG bytesRead = 0;
                        pMemStream->Read(data, dataSize, &bytesRead);

                        // convert to base64 encoded PNG
                        base64PNG = "data:image/png;base64,";
                        base64PNG += base64_encode(data, dataSize);

                        delete [] data;
                        data = NULL;
                    }
                }
            }
        }

        if (piFactory)
            piFactory->Release();

        if (piScaler)
            piScaler->Release();

        if (piBitmapFrame)
            piBitmapFrame->Release();

        if (piEncoder)
            piEncoder->Release();

        if (piStream)
            piStream->Release();

        if (pMemStream)
            pMemStream->Release();

        return base64PNG;
    }

    NetworkInfo platformGetNetworkInfo()
    {
        NetworkInfo netInfo;
        HANDLE hClient = NULL;
        DWORD dwResult = 0;
        DWORD dwMaxClient = 2;
        DWORD dwCurVersion = 0;
        PWLAN_INTERFACE_INFO pIfInfo = NULL;
        PWLAN_INTERFACE_INFO_LIST pIfList = NULL;
        PWLAN_CONNECTION_ATTRIBUTES pConnectInfo = NULL;
        DWORD connectInfoSize = sizeof(WLAN_CONNECTION_ATTRIBUTES);
        WLAN_OPCODE_VALUE_TYPE opCode = wlan_opcode_value_type_invalid;
        unsigned int i;
        try{
            dwResult = WlanOpenHandle(dwMaxClient, NULL, &dwCurVersion, &hClient);
            if (dwResult != ERROR_SUCCESS) {
                throw -1;
            }
            dwResult = WlanEnumInterfaces(hClient, NULL, &pIfList);
            if (dwResult != ERROR_SUCCESS) {
                throw -2;
            } else {
                for (i = 0; i < (int) pIfList->dwNumberOfItems; i++) {
                    pIfInfo = (WLAN_INTERFACE_INFO *) & pIfList->InterfaceInfo[i];
                    // If interface state is connected, call WlanQueryInterface
                    // to get current connection attributes
                    if (pIfInfo->isState == wlan_interface_state_connected)
                    {
                        dwResult =  WlanQueryInterface(hClient,
                            &pIfInfo->InterfaceGuid,
                            wlan_intf_opcode_current_connection,
                            NULL,
                            &connectInfoSize,
                            (PVOID *) &pConnectInfo,
                            &opCode);
                        if (dwResult != ERROR_SUCCESS) {
                            throw -3;
                        }
                        break;
                    }
                }
            }
        }
        catch(...)
        {
            netInfo.connectionStatus = bjn_sky::NetworkInfo::stat_unknown;
        }
        if (pConnectInfo != NULL)
        {
            netInfo.connectionStatus = bjn_sky::NetworkInfo::stat_ok;
            netInfo.wlanSignalQuality = pConnectInfo->wlanAssociationAttributes.wlanSignalQuality;
            WlanFreeMemory(pConnectInfo);
            pConnectInfo = NULL;
        }
        if (pIfList != NULL) {
            WlanFreeMemory(pIfList);
            pIfList = NULL;
        }
        if(hClient != NULL)
        {
            WlanCloseHandle(hClient, NULL);
            hClient = NULL;
        }
        return netInfo;
    }

    std::string getKeyFromCameraProductId(std::string productId)
    {
        std::string vId; 
        std::string pId;
        std::stringstream vIdss, pIdss;

        std::transform(productId.begin(), productId.end(),productId.begin(), ::toupper);
        // get the vendor id
        char *vendorKey = "VID_";
        const char *startChar = strstr(productId.c_str(), vendorKey);
        if (startChar)
        {
            int vIdi;
            vId.assign(startChar + 4, 4);
            vIdss << std::hex << vId;
            vIdss >> vIdi;
            vId = std::to_string((long long) vIdi);
        }

        // get the product id
        char *productKey = "PID_";
        startChar = strstr(productId.c_str(), productKey);
        if (startChar)
        {
            int pIdi;
            pId.assign(startChar + 4, 4);
            pIdss << std::hex << pId;
            pIdss >> pIdi;
            pId = std::to_string((long long) pIdi);
        }
        std::string key = "UVC Camera VendorID_" + vId + " ProductID_" + pId; 
        return key;
    }

    bool isWindows8AndAbove()
    {
        OSVERSIONINFO osInfo = { 0 };
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        GetVersionEx(&osInfo);
        if ((osInfo.dwMajorVersion > 6)
            || ((osInfo.dwMajorVersion == 6) && (osInfo.dwMinorVersion >= 2)))
        {
            return true;
        }
        return false;
    }

    bool isWindowsVistaAndAbove()
    {
        OSVERSIONINFO osInfo = { 0 };
        osInfo.dwOSVersionInfoSize = sizeof(osInfo);
        GetVersionEx(&osInfo);

        // Major version for vista is 6 and minor is 0, so no need to check for minor version here
        if (osInfo.dwMajorVersion >= 6)
        {
            return true;
        }
        return false;
    }
}
