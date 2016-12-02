
#ifndef TALK_BASE_WIN32PROCESSDETAILS_H_
#define TALK_BASE_WIN32PROCESSDETAILS_H_

#include "talk/base/win32.h"
#include "talk/base/logging_libjingle.h"

namespace talk_base {

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

    ~ProcessDetails()
    {
        if(m_hProcess)
            CloseHandle(m_hProcess);
    }

    DWORD GetProcessId() { return m_dwProcessId; }

    IntegrityLevel GetProcessIntegrityLevel();

    std::wstring GetProcessImage();

private:
    DWORD m_dwProcessId;
    HANDLE m_hProcess;
    static LPQueryFullProcessImageName s_lpQueryFullProcessImageName;

    void InitProcess()
    {
        m_hProcess = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, m_dwProcessId);
        if(NULL == m_hProcess)
        {
            LOG_GLE(LS_ERROR) << "OpenProcess failed";
        }
    }
};

} // namespace talk_base

#endif //TALK_BASE_WIN32PROCESSDETAILS_H_