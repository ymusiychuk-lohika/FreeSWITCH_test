
#ifndef WEBRTC_WIN32PROCESSDETAILS_H_
#define WEBRTC_WIN32PROCESSDETAILS_H_

#include <Windows.h>
#include <map>

#ifdef BJN_WEBRTC_CHANGES
#include "trace.h"
#else
#include "bjnAppCaptureLog.h"
#endif

#ifdef EXCLUDE_LOGS
// Don't enable logs
#undef Log
#define Log(...)
#endif //EXCLUDE_LOGS

namespace webrtc {

// Process integrity level
typedef enum _IntegrityLevel
{
    Unknown = -1,
    Low = 0,
    Medium = 1,
    High = 2
}IntegrityLevel;

typedef BOOL (WINAPI *LPFN_ISWOW64PROCESS) (HANDLE, PBOOL);
typedef BOOL (WINAPI *LPQueryFullProcessImageName)(
  HANDLE hProcess,
  DWORD dwFlags,
  LPTSTR lpExeName,
  PDWORD lpdwSize
);

// Process info related class
class ProcessDetails
{
public:
    ProcessDetails(DWORD dwProcessId) : m_dwProcessId(dwProcessId), m_hProcess(NULL)
    {
        InitProcess();
    }

    ProcessDetails(HWND hwnd) : m_dwProcessId(0), m_hProcess(NULL)
    {
        // compare against thread id which cannot be 0
        if(0 != GetWindowThreadProcessId(hwnd, &m_dwProcessId))
            InitProcess();
    }

    ProcessDetails() : m_dwProcessId(0), m_hProcess(NULL)
    {
    }

    ~ProcessDetails()
    {
        if(m_hProcess)
            CloseHandle(m_hProcess);
    }

    void Init(DWORD dwProcessId) {
        m_dwProcessId = dwProcessId;
        InitProcess();
    }

    BOOL IsValid() { return (NULL != m_hProcess) ? TRUE : FALSE ;}

    DWORD GetProcessId() { return m_dwProcessId; }

    std::wstring GetProcessImage();

    IntegrityLevel GetProcessIntegrityLevel();

    BOOL WINAPI SafeIsWow64Process(PBOOL Wow64Process);

    BOOL Is64BitProcess();

    static BOOL WINAPI SafeIsWow64Process(HANDLE hProcess, PBOOL Wow64Process);

    static BOOL Is64BitOS();

    BOOL IsAlive();

    BOOL CompareProcessName(ProcessDetails& pd);

    BOOL GetProcessGroup(std::map<UINT, BOOL>& listProcessGroup);

#ifdef _WIN32_WINNT_WIN8
    BOOL IsProcessUnderAppContainer();
#endif //_WIN32_WINNT_WIN8

    BOOL IsWOWProcess();

    BOOL IsProcessInJob();

private:
    DWORD m_dwProcessId;
    HANDLE m_hProcess;
    std::wstring m_sProcessName;

    static LPFN_ISWOW64PROCESS s_fnIsWow64Process;
    static LPQueryFullProcessImageName s_lpQueryFullProcessImageName;


    void InitProcess()
    {
        m_hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_dwProcessId);
        if(NULL == m_hProcess)
        {
#ifdef BJN_WEBRTC_CHANGES
            WEBRTC_TRACE(webrtc::kTraceError, webrtc::kTraceVideoCapture, 0,
                "OpenProcess failed: %d", GetLastError());
#else
            Log("OpenProcess failed: Process ID: %d, error = %d\n", m_dwProcessId, GetLastError());
#endif
        }
    }
};

} // namespace webrtc

#endif //WEBRTC_WIN32PROCESSDETAILS_H_