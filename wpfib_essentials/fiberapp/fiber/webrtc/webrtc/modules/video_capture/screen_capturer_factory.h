#ifndef __SCREEN_CAPTURER_FACTORY_H_
#define __SCREEN_CAPTURER_FACTORY_H_

//#if defined(OSX)

#if defined(WIN32)
#include "windows/screen_capturer_windows.h"
#elif defined(WEBRTC_MAC)
#include "mac/screen_capturer_mac.h"
#elif defined(WEBRTC_ANDROID)
#include "android/screen_capture_android.h"
#elif defined(WEBRTC_LINUX)
#include "linux/screen_capturer_linux.h"
#endif

#include "screen_capturer.h"

namespace BJN
{
    class ScreenCapturerFactory
    {
        public:
            static ScreenCapturer* CreateScreenCapturer(int32_t id, webrtc::VideoCaptureExternal& captureObserver)
            {
#if defined(WIN32)
                return new ScreenCapturerWindows(id, captureObserver);
#elif defined(WEBRTC_MAC)
                return new ScreenCapturerMac(id, captureObserver);
#elif defined(WEBRTC_ANDROID)
                return new ScreenCapturerAndroid(id, captureObserver);
#elif defined(WEBRTC_LINUX)
                return new ScreenCapturerLinux(id, captureObserver);
#endif
            }
    };

}   //namespace BJN


#endif  //__SCREEN_CAPTURER_FACTORY_H_

