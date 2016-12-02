#ifndef __WIN8_USER32_MODULE_H__
#define __WIN8_USER32_MODULE_H__
#include <Windows.h>

namespace BJN
{
    typedef LONG (WINAPI * pfnGetDisplayConfigBufferSizes)(UINT Flags, UINT *pNumPathArrayElements, UINT *pNumModeInfoArrayElements);
    typedef LONG (WINAPI * pfnQueryDisplayConfig)(UINT Flags,UINT *pNumPathArrayElements, DISPLAYCONFIG_PATH_INFO *pPathInfoArray,
    UINT *pNumModeInfoArrayElements, DISPLAYCONFIG_MODE_INFO *pModeInfoArray, DISPLAYCONFIG_TOPOLOGY_ID *pCurrentTopologyId);
    typedef LONG (WINAPI *pfnDisplayConfigGetDeviceInfo)(DISPLAYCONFIG_DEVICE_INFO_HEADER* pDeviceInfoHeader);

    class Win8User32Module
    {
    public:
        Win8User32Module()
        {
            m_hUser = GetModuleHandle(L"user32.dll");
            if(m_hUser)
            {
                m_FpGetDisplayConfigBufferSizes = (pfnGetDisplayConfigBufferSizes) GetProcAddress(m_hUser, "GetDisplayConfigBufferSizes");
                m_FpQueryDisplayConfig = (pfnQueryDisplayConfig) GetProcAddress(m_hUser, "QueryDisplayConfig");
                m_FpDisplayConfigGetDeviceInfo = (pfnDisplayConfigGetDeviceInfo) GetProcAddress(m_hUser, "DisplayConfigGetDeviceInfo");
            }
        }
        ~Win8User32Module()
        {
            //no need to free library
        }
        LONG GetDisplayConfigBufferSizes(_In_ UINT32 Flags, _Out_ UINT32 *pNumPathArrayElements, _Out_ UINT32 *pNumModeInfoArrayElements)
        {
            if(m_FpGetDisplayConfigBufferSizes)
            {
                return m_FpGetDisplayConfigBufferSizes(Flags, pNumPathArrayElements, pNumModeInfoArrayElements);
            }
            else
            {
                return ERROR_NOT_SUPPORTED ;
            }
        }
        LONG QueryDisplayConfig(_In_ UINT32 Flags, _Inout_ UINT32 *pNumPathArrayElements, _Out_ DISPLAYCONFIG_PATH_INFO *pPathInfoArray,
            _Inout_ UINT32 *pNumModeInfoArrayElements, _Out_ DISPLAYCONFIG_MODE_INFO *pModeInfoArray,
            _Out_opt_ DISPLAYCONFIG_TOPOLOGY_ID *pCurrentTopologyId)
        {
            if(m_FpQueryDisplayConfig)
            {
                return m_FpQueryDisplayConfig(Flags, pNumPathArrayElements, pPathInfoArray, pNumModeInfoArrayElements, pModeInfoArray, pCurrentTopologyId);
            }
            else
            {
                return ERROR_NOT_SUPPORTED;
            }
        }
        int GetProcessDpiAwareType()
        {
            int dpiAwareType = -1;
            HMODULE hShCore = LoadLibrary(L"Shcore.dll");
            if(hShCore)
            {
                typedef  HRESULT  (WINAPI *fnPtr)(HANDLE hprocess,int * dpiAware);
                fnPtr fnpGetProcessDpiAwareness = (fnPtr)(GetProcAddress(hShCore, "GetProcessDpiAwareness"));
                if(fnpGetProcessDpiAwareness)
                {
                    HANDLE hProcess;
                    hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, GetCurrentProcessId());
                    if(hProcess)
                    {
                        fnpGetProcessDpiAwareness(hProcess,&dpiAwareType);
                        CloseHandle(hProcess);
                    }
                }
                FreeLibrary(hShCore);
            }
            /*GetProcessDpiAwareness API failed we need to fallback to user32.dll IsProcessDPIAware API*/
            if(dpiAwareType < 0)
            {
                if(m_hUser)
                {
                    typedef BOOL (*fnPtr)();
                    fnPtr IsProcessDPIAware = (fnPtr)GetProcAddress(m_hUser, "IsProcessDPIAware");
                    if(IsProcessDPIAware) {
                        dpiAwareType = IsProcessDPIAware();
                    }
                }
            }
            dpiAwareType = (dpiAwareType < 0) ? 0 : dpiAwareType;
            return dpiAwareType;
        }

        LONG DisplayConfigGetDeviceInfo(DISPLAYCONFIG_DEVICE_INFO_HEADER* pDeviceInfoHeader)
        {
            if(m_FpDisplayConfigGetDeviceInfo) {
                return m_FpDisplayConfigGetDeviceInfo(pDeviceInfoHeader);
            }
            return ERROR_GEN_FAILURE;
        }

    private:
        HMODULE m_hUser;
        pfnGetDisplayConfigBufferSizes m_FpGetDisplayConfigBufferSizes;
        pfnQueryDisplayConfig          m_FpQueryDisplayConfig;
        pfnDisplayConfigGetDeviceInfo  m_FpDisplayConfigGetDeviceInfo;
    };
}
#endif
