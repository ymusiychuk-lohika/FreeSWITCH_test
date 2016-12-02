// Copyright 2015 Blue Jeans Network. All Rights Reserved

#ifndef TALK_BASE_WIN32CPUPARAMS_H_
#define TALK_BASE_WIN32CPUPARAMS_H_

#include <pdh.h>

namespace talk_base {

#define PROCESSOR_FREQUENCY L"\\Processor Information(*)\\Processor Frequency"

class WinCPUParmas {
public:
    WinCPUParmas();
    ~WinCPUParmas();
    int GetFrequency();

private:
    typedef PDH_STATUS (WINAPI *LPPdhAddEnglishCounter)(
        PDH_HQUERY   hQuery,
        LPCTSTR      szFullCounterPath,
        DWORD_PTR    dwUserData,
        PDH_HCOUNTER *phCounter);

    PDH_HCOUNTER*          m_pPDHCounterFreq;
    UINT                   m_nTotalCount;

    PDH_HQUERY             m_hPDHQueryFreq;
    PDH_HCOUNTER           m_hPDHCounterTotalFreq;
    BOOL                   m_bInitialised;
    LPPdhAddEnglishCounter m_lpPdhAddEnglishCounter;

    BOOL Init();

    BOOL InitProcessorLevel();

    int GetFrequencyMultiProcessor();
};

} //namespace talk_base

#endif //TALK_BASE_WIN32CPUPARAMS_H_
