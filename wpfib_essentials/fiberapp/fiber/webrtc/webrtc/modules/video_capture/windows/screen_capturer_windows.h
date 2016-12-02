#ifndef __SCREEN_CAPTURER_WINDOWS_H_
#define __SCREEN_CAPTURER_WINDOWS_H_

#include "../screen_capturer.h"
#include "video_capture_defines.h"
#include "screen_cap_msg_thread.h"
#include "screen_cap_border.h"
#include <vector>
#include "win_capture_feature_flags.h"

namespace BJN
{
    typedef struct _MonitorInfo
    {
        HMONITOR    hMonitor;
        RECT        rcMonitor;
        RECT        rcWork;
    } MonitorInfo;

    typedef struct _MonitorsData
    {
        std::vector<MonitorInfo>  list;
        int         primaryMonitorIndex;
    } MonitorsData;

    typedef struct _AppWindowInfo
    {
        HWND        hwnd;
        RECT        rcWin;
        DWORD       pid;
    } AppWindowInfo;

    typedef struct _AppWindowList
    {
        DWORD                       pid;
        std::vector<AppWindowInfo>  list;
    } AppWindowList;

    typedef enum _ProcessSharingErrorCode
    {
        ErrorSuccess,
        ErroGenericFailure,
        FirstFrameNotWritten,
        ErrorUseFallback
    }ProcessSharingErrorCode;

    class ScreenCapturerWindows: public ScreenCapturer
    {
        public:
            ScreenCapturerWindows(int32_t id, webrtc::VideoCaptureExternal& captureObserver);
            ~ScreenCapturerWindows();
            void init();
            void destroy();
            void captureConfigComplete();
            static int32_t getCaptureScreenCount();
            static webrtc::VideoCaptureCapability getCaptureCapability(int screenIndex);

        protected:
            void captureFrame();

        private:
            void drawMouse(HDC hDC);
            bool copyScreenToBitmap32bpp(webrtc::VideoCaptureCapability& resultantCap);
            bool copyScreenToBitmap(webrtc::VideoCaptureCapability& resultantCap);
            bool copyMagnifierToBitmap(webrtc::VideoCaptureCapability& resultantCap);
            ProcessSharingErrorCode copyDesktopDuplicationToBitmap(webrtc::VideoCaptureCapability& resultantCap);
            bool copyApplicationToBitmap2(uint32_t pid, webrtc::VideoCaptureCapability& resultantCap);
            bool CheckIfMetroApp();
            bool ResizeCaptureBuffer2(int width, int height);
            static BOOL CALLBACK ScreenCapturerWindows::AppCaptureEnumWindowsProc(HWND hwnd, LPARAM lParam);
            bool SetObjectToLowIntegrity(HANDLE Object);
            void NotifyAppCaputreFailure();

            // Screen capture using Desktop Duplication API enabled
            inline bool isScreenCaptureUsingDDEnabled();

            BJN::FeatureType GetCaptureType();

            void ClearSharedMemoryData();

        private:
            CaptureMessageQueueThread    m_messageThread;
            bool                         m_useMagnifier;
            BYTE*                        m_captureBuffer;
            UINT                         m_captureBufferSize;
            int                          m_captureBufferWidth;
            int                          m_captureBufferHeight;
            HMONITOR                     m_captureMonitor;
            bool                         m_configComplete;
            HANDLE                       m_sharedMemory;
            LPTSTR                       m_sharedBuffer;
            HANDLE                       m_writeEvent;
            HANDLE                       m_firstWriteEvent;
            UINT                         m_quitMessage;
            bool                         m_isOsWindows8AndAbove;
            HBRUSH                       m_overlappedBrush;
            HMONITOR                     m_capturePrimaryMonitor;
            bool                         m_isSafeToCaptureSecondaryMonitor;
            BJN::FeatureType             m_captureType;
            BOOL                         m_bGlobalHooks; // Flag to indicate global hook for App share
            PROCESS_INFORMATION          m_pi;
            AppCaptureProcessParameters* m_pAppCaptureProcessParams;
            bool                         m_enableInfoBar;
    };

}   //namepspace BJN


#endif  //__SCREEN_CAPTURER_WINDOWS_H_