#ifndef __BJN_WIN_UTILS__
#define __BJN_WIN_UTILS__

#include<iostream>
#include<sstream>
#include<vector>
#include <winsock2.h>
#include<Windows.h>
#include <Sddl.h>
#include <talk/base/basictypes.h>
#include <talk/base/logging_libjingle.h>
#include "sipmanagerbase.h"

using namespace bjn_sky;

namespace BJN
{
    void invokeBrowserFullScreenShortcut();
    void simulateMouseMove(int x, int y);
    int getNameServer(std::vector<std::string> &nameServer);
    //void ClientToScreen(HWND *win, BJNPoint &cursorPos);
    void LogCurrentPowerScheme();
    std::string ConvertImageToPNGBase64(uint8* buffer, int width, int height);
    NetworkInfo platformGetNetworkInfo();
    std::string getKeyFromCameraProductId(std::string productId);
    bool isWindows8AndAbove();
    bool isWindowsVistaAndAbove();
}
#endif
