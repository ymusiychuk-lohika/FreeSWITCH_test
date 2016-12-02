/*
 *
 * Copyright 2015 Blue Jeans Network. All Rights Reserved
 *
 */

#include "talk/base/win32.h"  // first because it brings in win32 stuff
#include <pdhmsg.h>
#include "win32cpuparams.h"

namespace talk_base {

    WinCPUParmas::WinCPUParmas() :
      m_hPDHQueryFreq(INVALID_HANDLE_VALUE),
      m_hPDHCounterTotalFreq(INVALID_HANDLE_VALUE),
      m_bInitialised(FALSE),
      m_lpPdhAddEnglishCounter(NULL),
      m_pPDHCounterFreq(NULL),
      m_nTotalCount(0) {
          if(Init()) {
              m_bInitialised = TRUE;
          }
    }

    WinCPUParmas::~WinCPUParmas() {
        if(0 != m_nTotalCount && NULL != m_pPDHCounterFreq) {
            for(UINT nIndex=0; nIndex < m_nTotalCount; nIndex++) {
                if(INVALID_HANDLE_VALUE != m_pPDHCounterFreq[nIndex]) {
                    PdhRemoveCounter(m_pPDHCounterFreq[nIndex]);
                }
            }
            delete[] m_pPDHCounterFreq;
        }

        if(INVALID_HANDLE_VALUE != m_hPDHCounterTotalFreq) {
            PdhRemoveCounter(m_hPDHCounterTotalFreq);
        }
        if(INVALID_HANDLE_VALUE != m_hPDHQueryFreq) {
            PdhCloseQuery(m_hPDHQueryFreq);
        }
    }

    int WinCPUParmas::GetFrequency() {
        // If the total Processor Frequency returns proper value then return that value
        // otherwise calculate the average of Processor Frequency from each processor
        if(FALSE == m_bInitialised) {
            return 0;
        }

        PDH_STATUS resStatus;
        PDH_FMT_COUNTERVALUE counterVal = {0};

        if(0 == m_nTotalCount) {
            resStatus = PdhCollectQueryData(m_hPDHQueryFreq);
            if(ERROR_SUCCESS != resStatus) {
                return 0;
            }

            resStatus = PdhGetFormattedCounterValue(m_hPDHCounterTotalFreq, PDH_FMT_DOUBLE|PDH_FMT_NOCAP100|PDH_FMT_NOSCALE, NULL, &counterVal);
            if(ERROR_SUCCESS != resStatus) {
                return 0;
            }
        }
        else
            return GetFrequencyMultiProcessor();

        return (int)counterVal.doubleValue;
    }

    BOOL WinCPUParmas::Init() {
        HMODULE hPDHDll = GetModuleHandle(L"Pdh.dll");
        m_lpPdhAddEnglishCounter = (LPPdhAddEnglishCounter) GetProcAddress(hPDHDll, "PdhAddEnglishCounter");

        PDH_STATUS resStatus;
        resStatus = PdhOpenQuery(NULL, NULL, &m_hPDHQueryFreq);
        if(ERROR_SUCCESS != resStatus) {
            return FALSE;
        }

        if(NULL != m_lpPdhAddEnglishCounter) {
            // This is required for cross language version of windows OS
            resStatus = m_lpPdhAddEnglishCounter(m_hPDHQueryFreq, PROCESSOR_FREQUENCY, NULL, &m_hPDHCounterTotalFreq);
        }
        else {
            // This is required for WinXP
            resStatus = PdhAddCounter(m_hPDHQueryFreq, PROCESSOR_FREQUENCY, NULL, &m_hPDHCounterTotalFreq);
        }

        if(ERROR_SUCCESS != resStatus) {
            return FALSE;
        }

        // This API has to be called twice once here and once in GetFrequecy function
        resStatus = PdhCollectQueryData(m_hPDHQueryFreq);
        if(ERROR_SUCCESS != resStatus) {
            return FALSE;
        }

        if(0 == GetFrequency()) {
            // in windows 7 total value is always 0, so need to calculate the average
            // so reinit the processor frequency calculation mechanism
            if(0 == m_nTotalCount) {
                // initialise the processor level monitoring
                return InitProcessorLevel();
            }
        }

        // All is well here
        return TRUE;
    }

    BOOL WinCPUParmas::InitProcessorLevel() {
        // Add PDH Frequency query for each processor seperately and need to calculate the average
        DWORD dwExpandSize = 0;
        PDH_STATUS resStatus = PdhExpandWildCardPath(NULL, PROCESSOR_FREQUENCY, NULL, &dwExpandSize, PDH_NOEXPANDCOUNTERS);

        if(PDH_MORE_DATA != resStatus) {
            return FALSE;
        }

        // Actual call to retrive the proper values of processor frequency
        LPTSTR pwszExpandList = new WCHAR[dwExpandSize];
        resStatus = PdhExpandWildCardPath(NULL, PROCESSOR_FREQUENCY, pwszExpandList, &dwExpandSize, PDH_NOEXPANDCOUNTERS);
        if(ERROR_SUCCESS != resStatus) {
            delete[] pwszExpandList;
            return FALSE;
        }

        // Calculate the total count first
        int nTotalCount = 0;
        for (LPTSTR pszz = pwszExpandList; *pszz; pszz += lstrlen(pszz) + 1) {
            if(NULL != wcsstr(pszz, L"Total")) {
                // ignore the total counter, which we actually want to skip
                continue;
            }
            nTotalCount++;
        }

        // Close already open counter and queries
        if(INVALID_HANDLE_VALUE != m_hPDHCounterTotalFreq) {
            // Close the non working counter value
            PdhRemoveCounter(m_hPDHCounterTotalFreq);
            m_hPDHCounterTotalFreq = INVALID_HANDLE_VALUE;
        }

        if(INVALID_HANDLE_VALUE != m_hPDHQueryFreq) {
            // Close the already open Query
            PdhCloseQuery(m_hPDHQueryFreq);
            m_hPDHQueryFreq = INVALID_HANDLE_VALUE;
        }

        m_nTotalCount = nTotalCount;

        m_pPDHCounterFreq = new PDH_HCOUNTER[m_nTotalCount];
        // Initialise the array object
        for(UINT nIndex=0; nIndex < m_nTotalCount; nIndex++) {
            m_pPDHCounterFreq[nIndex] = INVALID_HANDLE_VALUE;
        }

        resStatus = PdhOpenQuery(NULL, NULL, &m_hPDHQueryFreq);
        if(ERROR_SUCCESS != resStatus) {
            delete[] pwszExpandList;
            // No need to delete m_pPDHCounterFreq, in destructor it is taken care
            return FALSE;
        }
        int nIndex = 0;
        for (LPTSTR pszz = pwszExpandList; *pszz; pszz += lstrlen(pszz) + 1) {
            if(NULL != wcsstr(pszz, L"Total")) {
                // ignore the total counter, as it returns 0
                continue;
            }

            if(NULL != m_lpPdhAddEnglishCounter) {
                // This is required for cross language version of windows OS
                resStatus = m_lpPdhAddEnglishCounter(m_hPDHQueryFreq, pszz, NULL, &m_pPDHCounterFreq[nIndex]);
            }
            else {
                // This is required for WinXP & Win 7
                resStatus = PdhAddCounter(m_hPDHQueryFreq, pszz, NULL, &m_pPDHCounterFreq[nIndex]);
            }

            if(ERROR_SUCCESS != resStatus) {
                delete[] pwszExpandList;
                // No need to delete m_pPDHCounterFreq, in destructor it is taken care
                return FALSE;
            }

            nIndex++;
        }

        // This API needs to be called twice once here and once inside GetFrequencyMultiProcessor
        resStatus = PdhCollectQueryData(m_hPDHQueryFreq);
        if(ERROR_SUCCESS != resStatus) {
            delete[] pwszExpandList;
            // No need to delete m_pPDHCounterFreq, in destructor it is taken care
            return FALSE;
        }

        // Initialise it again
        GetFrequencyMultiProcessor();

        delete[] pwszExpandList;
        return TRUE;
    }

    int WinCPUParmas::GetFrequencyMultiProcessor() {
        if(0 == m_nTotalCount)
        {
            return 0;
        }

        PDH_STATUS resStatus;
        resStatus = PdhCollectQueryData(m_hPDHQueryFreq);
        if(ERROR_SUCCESS != resStatus) {
            return 0;
        }

        int nTotalRet = 0;
        for(UINT nIndex=0; nIndex < m_nTotalCount; nIndex++) {
            PDH_FMT_COUNTERVALUE counterVal;

            resStatus = PdhGetFormattedCounterValue(m_pPDHCounterFreq[nIndex], PDH_FMT_DOUBLE|PDH_FMT_NOCAP100|PDH_FMT_NOSCALE, NULL, &counterVal);
            if(ERROR_SUCCESS != resStatus) {
                return 0;
            }

            nTotalRet += (int)counterVal.doubleValue;
        }

        // blind average! only option left on Windows 7
        return (int) (nTotalRet / m_nTotalCount);
    }
 } // namespace talk_base
