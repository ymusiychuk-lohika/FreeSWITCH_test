#ifndef __CALL_API_H_
#define __CALL_API_H_
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600

#include "skinnysipmanager.h"
#include <boost/filesystem.hpp>
#include <boost/thread/thread.hpp>
#include <boost/algorithm/string.hpp>
#include "bjnrenderer.h" 
#include "uiwidget.h"
#ifdef FB_WIN_D3D
#include "win\D3D\IDxProxyWidget.h"
#endif

using namespace bjn_sky;


typedef void(__stdcall *fnNotify)(int window);
typedef void(__stdcall *MessageCallback)(const char* msg);

class CallApi : public talk_base::MessageHandler
{
    public:
        enum CallStatus
        {
            CONNECTED,
            DISCONNECTED,
        };

		enum DevicesUpdated
		{
			DEVNONE = 0,
			DEVVID = 1,
			DEVMIC = 2,
			DEVSPEAKER = 4,
			DEVALL = 7
		};

		class Device
		{
		public:
			std::string name;
			std::string id;
			bool operator==(const Device& b2)
			{
				if ((name == b2.name) && (id == b2.id)) {
					return true;
				}
				return false;
			}
		};

		CallApi();
        ~CallApi();
        void shutdown();
        void hideWindows();
        void toggleSelfView(bool showSelfView);
		void registerMessageCallBack(MessageCallback callback);
		void registerShowWindowCallback(fnNotify callback);
#ifdef FB_WIN_D3D
		void registerDxWidgetCallback(fnInitWidget cbInit, fnUpdateView cbUpdate);
#endif
		void showRemoteVideo(int width, int height, int posX, int posY, HWND remoteViewWindow);
        void Init(HWND hwnd, std::string name, std::string uri, std::string turnservers, bool forceTurn);
        void startSip();
        void runIoService();
        void initMediaEngine();
        void startCall();
        void hangupCall();
        void muteAudio(bool mute);
        void muteVideo(bool mute);
        void deinitSip();
        void tokenRequest();
        void tokenRelease();
        void RegisterIoThread();
        void setupMediaLogging(std::string &filePath);
        void enableMeetingFeatures(std::string features);
		static talk_base::Thread* MainThread;
        virtual void OnMessage(talk_base::Message* msg);
        fnNotify showWindow;
		MessageCallback mCallback;
        void initializeWidget(int widget, int width, int height, int x, int y);
		LRESULT HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

    private:
        DISALLOW_COPY_AND_ASSIGN(CallApi);
		static const unsigned short LOCAL_STREAM_INDEX = 0;
#ifdef FB_WIN_D3D
		fnInitWidget _initDxWidget;
		fnUpdateView _updateView;
		HWND _hWnd;
#endif
        
		skinnySipManager* mSipManager;
        talk_base::Thread* mAppThread;
        boost::thread mThread;
        boost::thread mSipThread;
        boost::thread ioThread;
        talk_base::MessageHandler* mMessageHandler;
        Device mCurAudioCaptureDevice;
        Device mCurAudioPlayoutDevice;
        Device mCurVideoCaptureDevice;
        std::vector<Device> mVideoCaptureDevices;
        std::vector<Device> mAudioCaptureDevices;
        std::vector<Device> mAudioPlayoutDevices;
        std::string mUri;
        std::string mUserAgent;
        std::string mName;
        std::string mVideoCaptureDevName;
        std::string mAudioCaptureDevName;
        std::string mAudioPlayoutDevName;
        std::string mTurnServers;
        std::string mCallGuid;
        std::string mScreenDeviceName;
        std::string mScreenDeviceId;
        int         mScreenWidth;
        int         mScreenHeight;
		bool mForceTurn;

        bjn_sky::MeetingFeatureOptions mMeetingFeatures;

        BjnRenderer *mWnd[2];
        UIWidget    *mUiWidget[2];
		int mPipHeight;
		int mPipWidth;
		int mPipX;
		int mPipY;

        PJThreadDescRegistration mMainThreadHandle;
        PJThreadDescRegistration mIoServiceThreadHandle;
        boost::asio::io_service  mIoService;
};
#endif //__CALL_API_H_

