#ifndef __SCREEN_CAPTURER_UTILS_H__
#define __SCREEN_CAPTURER_UTILS_H__

#include <string>

namespace webrtc {

    class ScreenCapturerUtils
    {
    public:
        static std::wstring getLatestPluginPathFromRegistry();

    };
}
#endif