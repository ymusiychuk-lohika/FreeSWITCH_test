
#include "talk/base/win32processdetails.h"

namespace talk_base {

LPQueryFullProcessImageName ProcessDetails::s_lpQueryFullProcessImageName = NULL;

IntegrityLevel ProcessDetails::GetProcessIntegrityLevel()
{
    if(NULL == m_hProcess)
    {
        return Unknown;
    }

    HANDLE hToken = NULL;
    PTOKEN_MANDATORY_LABEL pTIL = NULL;
    IntegrityLevel il = Unknown;

    DWORD dwLengthNeeded;
    DWORD dwError = ERROR_SUCCESS;

    DWORD dwIntegrityLevel;
    if (OpenProcessToken(m_hProcess, TOKEN_QUERY |
        TOKEN_QUERY_SOURCE, &hToken))
    {
        if (!GetTokenInformation(hToken, TokenIntegrityLevel,
            NULL, 0, &dwLengthNeeded))
        {
            dwError = GetLastError();
            if (dwError == ERROR_INSUFFICIENT_BUFFER)
            {
                pTIL = (PTOKEN_MANDATORY_LABEL)LocalAlloc(0,
                    dwLengthNeeded);
                if (pTIL != NULL)
                {
                    if (GetTokenInformation(hToken, TokenIntegrityLevel,
                        pTIL, dwLengthNeeded, &dwLengthNeeded))
                    {
                        dwIntegrityLevel = *GetSidSubAuthority(pTIL->Label.Sid,
                            (DWORD)(UCHAR)(*GetSidSubAuthorityCount(pTIL->Label.Sid)-1));

                        if (dwIntegrityLevel < SECURITY_MANDATORY_MEDIUM_RID)
                        {
                            il = Low;
                        }
                        else if (dwIntegrityLevel >= SECURITY_MANDATORY_MEDIUM_RID &&
                            dwIntegrityLevel < SECURITY_MANDATORY_HIGH_RID)
                        {
                            il = Medium;
                        }
                        else if (dwIntegrityLevel >= SECURITY_MANDATORY_HIGH_RID)
                        {
                            il = High;
                        }
                    }
                    else
                    {
                        LOG_GLE(LS_ERROR) << "GetTokenInformation: Failed to get integrity level";
                    }
                }
                else
                {
                    LOG_GLE(LS_ERROR) << "LocalAlloc: Failed to get integrity level";
                }
            }
            else
            {
                LOG_GLE(LS_ERROR) << "GetTokenInformation: Failed to get integrity level";
            }
        }
        //else success
    }
    else
    {
        LOG_GLE(LS_ERROR) << "OpenProcessToken: Failed to get integrity level";
    }

    if(pTIL)
        LocalFree(pTIL);

    if(hToken)
        CloseHandle(hToken);

    return il;
}



std::wstring ProcessDetails::GetProcessImage()
{
    if(NULL == m_hProcess)
    {
        return L"";
    }

    if (NULL == s_lpQueryFullProcessImageName)
    {
        HMODULE hModule = GetModuleHandle(L"kernel32.dll");
        if (hModule == NULL)
        {
            return FALSE;
        }
        s_lpQueryFullProcessImageName = reinterpret_cast<LPQueryFullProcessImageName>(
            GetProcAddress(hModule, "QueryFullProcessImageNameW"));
        if (s_lpQueryFullProcessImageName == NULL)
        {
            return L"";
        }
    }

    WCHAR szPath[MAX_PATH] = {0};
    DWORD szSize = _countof(szPath);
    if(FALSE == s_lpQueryFullProcessImageName(m_hProcess, 0, szPath, &szSize))
    {
        return L"";
    }

    return szPath;
}

} //namespace talk_base