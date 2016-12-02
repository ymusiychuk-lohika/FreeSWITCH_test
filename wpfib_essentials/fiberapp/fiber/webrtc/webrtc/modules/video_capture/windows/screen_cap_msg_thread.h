#ifndef __SCREEN_CAP_MSG_THREAD_H_
#define __SCREEN_CAP_MSG_THREAD_H_

#include "../screen_capturer.h"
#include "video_capture_defines.h"
#include "screen_cap_border.h"
#include "screen_cap_magnifier.h"

namespace BJN
{
    class CaptureMessageQueueThread
    {
    public:
        CaptureMessageQueueThread();
        ~CaptureMessageQueueThread();
        /*Added appsharing parameter to remove green border while app sharing*/
        void Start(HMONITOR hMon, bool useMagnifier, bool isAppSharing, bool enableInfoBar);
        void Stop();
        bool CopyMagnifierData(BYTE* buffer, UINT size, int width, int height, bool& useFallbackCapture);

    private:
        static LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
        static DWORD WINAPI CaptureMessageQueueThreadProc(LPVOID lpParameter);
        DWORD ThreadProc();
        void Init();
        void Close();

    private:
        HINSTANCE               _hInstance;
        HANDLE                  _hThreadEvent;
        HANDLE                  _hThread;
        BorderWin               _border;
        bool                    _useMagnifier;
        MagCapture              _magCapture;
        bool                    _useDWM;
        bool                    _reEnableDWM;
        HMONITOR                _hMonitor;
        bool                    _isAppSharing;
        bool                    _useAnimationCtrl;
        bool                    _reEnableAnimation;
        bool                    _enableInfoBar;
    };

}

#endif // __SCREEN_CAP_MSG_THREAD_H_
