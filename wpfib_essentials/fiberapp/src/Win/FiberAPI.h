#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#include <iostream>
#include "sys/types.h"
#include <signal.h>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "callapi.h"

using namespace bjn_sky;

#define EXTERN_C_DLL_EXPORT extern "C"  __declspec(dllexport)

//HELPERS
std::string getString(const char* chars);

//INITIALIZATION
EXTERN_C_DLL_EXPORT CallApi* CreateFiberApiHandle(void);

EXTERN_C_DLL_EXPORT void DestroyFiberApiHandle(CallApi* callApi);

//API
EXTERN_C_DLL_EXPORT void Init(CallApi* callApi, HWND hwnd,  const char* name, const char* uri, const char* turnservers, bool forceTurn);

#if (defined FB_WIN) && (!defined FB_WIN_D3D)
EXTERN_C_DLL_EXPORT LRESULT HandleWindowMessage(CallApi* callApi, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

EXTERN_C_DLL_EXPORT bool hangupCall(CallApi* callApi);

EXTERN_C_DLL_EXPORT bool InitializeWidget(CallApi* callApi, int widget, int width, int height, int x, int y);

EXTERN_C_DLL_EXPORT bool muteAudio(CallApi* callApi, bool state);

EXTERN_C_DLL_EXPORT bool muteVideo(CallApi* callApi, bool state);

EXTERN_C_DLL_EXPORT bool toggleSelfView(CallApi* callApi, bool show);

EXTERN_C_DLL_EXPORT bool registerMessageCallback(CallApi* callApi, MessageCallback callback);
EXTERN_C_DLL_EXPORT bool registerShowWindowCallback(CallApi* callApi, fnNotify callback);

EXTERN_C_DLL_EXPORT bool ShowRemoteVideo(CallApi *callApi, int width, int height, int posX, int posY, HWND remoteViewWindow);

EXTERN_C_DLL_EXPORT bool EnableMeetingFeatures(CallApi *callApi, const char* features);

#ifdef FB_WIN_D3D
EXTERN_C_DLL_EXPORT void registerDxWidgetCallback(CallApi* callApi, fnInitWidget cbInit, fnUpdateView cbUpdate);
#endif