#include "FiberAPI.h"

//HELPERS
std::string getString(const char* chars){	
	return std::string(chars);
};


//INTIALIZATION
CallApi* CreateFiberApiHandle(void)
{
	return new CallApi();
}

void DestroyFiberApiHandle(CallApi* callApi)
{
	if (callApi != NULL)
	{
		callApi->hideWindows();
		callApi->shutdown();
	}
	delete callApi;
	callApi = NULL;
}

//API
void Init(CallApi* callApi, HWND hwnd, const char* name, const char* uri, const char* turnservers, bool forceTurn)
{
	LOG(LS_INFO) << "Initializing CallApi";
	callApi->Init(hwnd, std::string(name), std::string(uri), std::string(turnservers), forceTurn);
};

#if (defined FB_WIN) && (!defined FB_WIN_D3D)
LRESULT HandleWindowMessage(CallApi* callApi, HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
//	LOG(LS_INFO) << "CallAPI WndProc";
	return callApi->HandleWindowMessage(hwnd,uMsg,wParam,lParam);
};
#endif

bool hangupCall(CallApi* callApi)
{
		LOG(LS_INFO)<<"Shutdown sip";		
		callApi->shutdown();
		return true;
};

bool InitializeWidget(CallApi* callApi, int widget, int width, int height, int x, int y)
{
	LOG(LS_INFO) << "widget initialized: " << widget;
	callApi->initializeWidget(widget, width, height, x, y);
	return true;
};

bool ShowRemoteVideo(CallApi *callApi, int width, int height, int posX, int posY, HWND remoteViewWindow) {
	callApi->showRemoteVideo(width, height, posX, posY, remoteViewWindow);
	return true;
}

bool muteAudio(CallApi* callApi, bool state)
{
	callApi->muteAudio(state);
	return true;
};

bool muteVideo(CallApi* callApi, bool state)
{
	callApi->muteVideo(state);
	return true;
};

bool toggleSelfView(CallApi* callApi, bool show)
{
	callApi->toggleSelfView(show);
	return true;
};

#ifdef FB_WIN_D3D
void registerDxWidgetCallback(CallApi* callApi, fnInitWidget cbInit, fnUpdateView cbUpdate)
{
	callApi->registerDxWidgetCallback(cbInit, cbUpdate);
}
#endif

bool registerMessageCallback(CallApi* callApi, MessageCallback callback) {
	callApi->registerMessageCallBack(callback);
	return true;
}

bool registerShowWindowCallback(CallApi* callApi, fnNotify callback) {
	callApi->registerShowWindowCallback(callback);
	return true;
}

bool EnableMeetingFeatures(CallApi *callApi, const char* features) {
	callApi->enableMeetingFeatures(std::string(features));
	return true;
}
