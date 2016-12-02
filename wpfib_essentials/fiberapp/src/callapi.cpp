//#define 
#define _WIN32_WINNT 0x0600
#define WINVER 0x0600
#include <iostream>
#include "sys/types.h"
#include <signal.h>
#include <sstream>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <boost/enable_shared_from_this.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/algorithm/string.hpp> 
#include <boost/date_time/posix_time/posix_time.hpp>
#include "talk/base/win32socketserver.h"

#include "skinnysipmanager.h"
#include "callapi.h"
#include "bjndevices.h"
#include "bjnrendererfactory.h"
#include "openlog.h"
#ifdef OSX
#include "talk/base/maccocoasocketserver.h"
#endif

using namespace bjn_sky;

// signal handling
bool  terminateCall = false;
static void sigHandler(int signal_num)
{
    switch(signal_num)
    {
        case SIGINT:
            LOG(LS_INFO) << "Recieved SIGINT";
            break;
        default:
            LOG(LS_INFO) << "Recieved signal:" << (signal_num);
    }
	terminateCall = true;
}

CallApi::CallApi()
: mUserAgent("Prism")
, mVideoCaptureDevName("Automatic Device")
, mAudioCaptureDevName("Automatic Device")
, mAudioPlayoutDevName("Automatic Device")
, showWindow(NULL)
{
	new_log::openNewLog();
}

void CallApi::initializeWidget(int widget, int width, int height, int x, int y){
	mPipWidth = width;
	mPipHeight = height;
	mPipX = x;
	mPipY = y;
	mUiWidget[widget]->Show(true, x, y, width, height, 0);
}

void CallApi::showRemoteVideo(int width, int height, int posX, int posY, HWND remoteViewWindow) {
	int med_idx = 1;
	mUiWidget[med_idx] = UIWidget::CreateUIWidget(remoteViewWindow);

#ifdef FB_WIN_D3D
	mWnd[med_idx] = BjnRendererFactory::CreateBjnRenderer(med_idx);
#else
	mWnd[med_idx] = BjnRendererFactory::CreateBjnRenderer(false);
#endif
#ifdef FB_WIN_D3D
	auto *dxProxy = dynamic_cast<IDxProxyWidget*>(mUiWidget[med_idx]);
	assert(dxProxy);
	if (dxProxy)
	{
		dxProxy->RegisterMainWindow(_hWnd);
		dxProxy->RegisterWidget(_initDxWidget, _updateView);
	}
#endif

	mUiWidget[med_idx]->SetRenderer(mWnd[med_idx]);
	mWnd[med_idx]->SetUIWidget(mUiWidget[med_idx]);
	
	//mUiWidget[med_idx]->Show(true, posX, posY, width, height, 0);
	if (showWindow) showWindow(med_idx);
	mSipManager->postUpdateRemoteRenderer(mWnd[med_idx], med_idx);
	mSipManager->postAddRemoteRenderer(med_idx);
}

void CallApi::registerMessageCallBack(MessageCallback callback) {
	mCallback = callback;
}

void CallApi::registerShowWindowCallback(fnNotify callback) {
	showWindow = callback;
}

#ifdef FB_WIN_D3D
void CallApi::registerDxWidgetCallback(fnInitWidget cbInit, fnUpdateView cbUpdate)
{
	_initDxWidget = cbInit;
	_updateView = cbUpdate;

	assert(cbInit && cbUpdate);
}
#endif

void CallApi::Init(HWND hwnd, std::string name, std::string uri, std::string turnservers, bool forceTurn)
{
	terminateCall = false;
	mName = name;
	mUri = uri;
	mTurnServers = turnservers;
	mForceTurn = forceTurn;
	
	//Create self view widget
	mUiWidget[0] = UIWidget::CreateUIWidget(NULL);
#ifdef FB_WIN_D3D
	mWnd[0] = BjnRendererFactory::CreateBjnRenderer(0);
#else 
	mWnd[0] = BjnRendererFactory::CreateBjnRenderer(false);
#endif

#ifdef FB_WIN_D3D
	_hWnd = hwnd;
	auto *dxProxy = dynamic_cast<IDxProxyWidget*>(mUiWidget[0]);
	assert(dxProxy);
	assert(_initDxWidget);
	assert(_updateView);
	if (dxProxy)
	{
		dxProxy->RegisterMainWindow(_hWnd);
		dxProxy->RegisterWidget(_initDxWidget, _updateView);
	}
#endif

	mUiWidget[0]->SetRenderer(mWnd[0]);
	mWnd[0]->SetUIWidget(mUiWidget[0]);
	

	mScreenDeviceName = BJN::DevicePropertyManager::getScreenSharingDeviceFriendlyName(0);
	mScreenDeviceId = BJN::DevicePropertyManager::getScreenSharingDeviceUniqueID(0, 0);
	
	LOG(LS_INFO) << "Creating CallApi";
	
#if WIN32
	// Need to pump messages on our main thread on Windows.
	talk_base::Win32Thread w32_thread;
	talk_base::ThreadManager::SetCurrent(&w32_thread);
#endif
	talk_base::AutoThread appAutothread;
	mAppThread = talk_base::Thread::Current();
	mSipThread = boost::thread(&CallApi::startSip, this);
	boost::thread ioThread = boost::thread(&CallApi::runIoService, this);
	
	mAppThread->Run();
	ioThread.join();
}

CallApi::~CallApi()
{
    talk_base::ThreadManager::SetCurrent(NULL);
    mSipThread.join();
}

void CallApi::toggleSelfView(bool showSelfView) {
	mUiWidget[0]->Show(showSelfView, mPipX, mPipY, mPipWidth, mPipHeight, 0);
}

void CallApi::hideWindows() {
	if (!mUri.empty()) //TOOD: [AB] need to check that Init was called and Widget was created!!!!
	{
		mUiWidget[0]->Show(false, 0, 0, 0, 0, 0);
		mUiWidget[1]->Show(false, 0, 0, 0, 0, 0);
	}
}


void CallApi::startSip()
{
	std::string configXML;
	SipManagerConfig config(1200, configXML);
	signal(SIGINT, sigHandler);

#if FB_WIN
	talk_base::Win32Thread sip_w32_thread;
	talk_base::ThreadManager::SetCurrent(&sip_w32_thread);
	//BJN::LogCurrentPowerScheme();
#endif
	talk_base::AutoThread sigAutoThread;
	talk_base::Thread* mSIPThread = talk_base::Thread::Current();
	mSipManager = new skinnySipManager(mSIPThread, this, mAppThread, config);
	//TODO BLUE sip_manager_->setSingleStream(mSingleStream);
	if (!mMeetingFeatures.empty()) {
		mSipManager->preCacheMeetingFeaturesList(mMeetingFeatures);
	}
	LOG(LS_INFO) << "Executing SIP thread. SIPManager address " << (mSipManager);
	mSIPThread->Run();
	LOG(LS_INFO) << "Stopped SIP thread. Deleting SIPmanager " << (mSipManager);
	delete (mSipManager);
	mSipManager = NULL;
	mAppThread->Quit();
}

void CallApi::shutdown() {
	terminateCall = true;
	ioThread.join();
}

void CallApi::initMediaEngine()
{
    mCurAudioCaptureDevice.name = mAudioCaptureDevName;
    mCurAudioCaptureDevice.id = "-1";
    mCurAudioPlayoutDevice.name = mAudioPlayoutDevName;
    mCurAudioPlayoutDevice.id = "-1";
    mCurVideoCaptureDevice.name = mVideoCaptureDevices[mVideoCaptureDevices.size() - 1].name;
    mCurVideoCaptureDevice.id = mVideoCaptureDevices[mVideoCaptureDevices.size() - 1].id;
    LocalMediaInfo *mediaInfo = new LocalMediaInfo();
    mediaInfo->setAudioCaptureName(mCurAudioCaptureDevice.name);
    mediaInfo->setAudiocapDeviceID(mCurAudioCaptureDevice.id);
    mediaInfo->setAudioPlayoutName(mCurAudioPlayoutDevice.name);
    mediaInfo->setAudioplayoutDeviceID(mCurAudioPlayoutDevice.id);
    mediaInfo->setVideoCaptureName(mCurVideoCaptureDevice.name);
    mediaInfo->setVideocapDeviceID(mCurVideoCaptureDevice.id);
    mSipManager->postInitializeMediaEngine(*mediaInfo);
}

void CallApi::enableMeetingFeatures(std::string features)
{
	boost::split(mMeetingFeatures, features, boost::is_any_of(", "), boost::token_compress_on);
}

void CallApi::startCall()
{
    LOG(LS_INFO)<<"Making SIP call name: "<<mName<<", sipUri: "<<mUri;
	std::vector<bjn_sky::TurnServerInfo> turnServers;
    
	//Parse turn servers info
	try
	{
		std::stringstream ss;
		ss << std::string(mTurnServers);
		boost::property_tree::ptree pt;
		boost::property_tree::read_json(ss, pt);

		for (auto& array_element : pt) {
			bjn_sky::TurnServerInfo *turnServer = new bjn_sky::TurnServerInfo();
			for (auto& property : array_element.second) {
				std::string name = (std::string)property.first;
				std::string value = (std::string)property.second.data();
				if (boost::iequals(name, "address"))
					turnServer->address = value;
				else if (boost::iequals(name, "password"))
					turnServer->password = value;
				else if (boost::iequals(name, "port"))
					turnServer->port = boost::lexical_cast<int>(value);
				else if (boost::iequals(name, "protocol"))
					turnServer->protocol = value;
				else if (boost::iequals(name, "username"))
					turnServer->username = value;
				else if (boost::iequals(name, "hostname"))
					turnServer->fqdn = value;
				else if (boost::iequals(name, "priority"))
					turnServer->priority = boost::lexical_cast<uint16_t>(value);
			}
			turnServers.push_back(*turnServer);
			turnServer = new bjn_sky::TurnServerInfo();
		}
	}
	catch (std::exception const& e)
	{
		LOG(LS_INFO) << "Failed to parse turn server info: " << e.what();
		std::cerr << e.what() << std::endl;
	}

    mSipManager->postToClient(MSG_ENABLE_MEETING_FEATURES, mMeetingFeatures);
    mSipManager->postMakeCall(mName, mUri, turnServers, mForceTurn, "", "");
}

void CallApi::hangupCall()
{
    if(mSipManager)
    {
        mSipManager->postClose();
    }
}

void CallApi::deinitSip()
{
    LOG(LS_INFO) << __FUNCTION__;
    if (mSipManager)
    {
        mSipManager->postExitSipUA();
    }
}

void CallApi::muteAudio(bool mute)
{
	LOG(LS_INFO) << "Posting audio mute message with state: " << mute;
	mSipManager->postMute(!mute);
}

void CallApi::muteVideo(bool mute)
{
	LOG(LS_INFO) << "Posting video mute message with state: " << mute;
	mSipManager->postMuteVideo(!mute);
	if (mute)
		mSipManager->postCaptureDevOff();
	else
		mSipManager->postCaptureDevOn();
}

void CallApi::RegisterIoThread()
{
    mIoServiceThreadHandle.Register("IoServiceThread");
}

void CallApi::OnMessage(talk_base::Message* pmsg)
{
    LOG(LS_INFO)<<"Got APP message: "<<pmsg->message_id;
    switch(pmsg->message_id) 
    {
        case bjn_sky::APP_MSG_INIT_COMPLETE : {
            mMainThreadHandle.Register("AppMainThread");
            mIoService.post(boost::bind(&CallApi::RegisterIoThread, this));
            break;
        }
        case bjn_sky::APP_MSG_NOTIFY_LOCAL_DEVICES_CHANGE:
        case bjn_sky::APP_MSG_DEVICE_ENHANCEMENTS:
        case bjn_sky::APP_MSG_SPK_FAIL:
        case bjn_sky::APP_MSG_FAULTY_SPEAKER_NOTIFY:
        case bjn_sky::APP_MSG_FAULTY_MICROPHONE_NOTIFY:
        case bjn_sky::APP_MSG_NOTIFY_WARNING:            
            break;
        case bjn_sky::APP_MSG_NOTIFY_LOCAL_DEVICES:
            {
                talk_base::TypedMessageData<LocalDevices> *data = static_cast<talk_base::TypedMessageData<LocalDevices> *>(pmsg->pdata);
                LocalDevices devices = data->data();
                LOG(LS_INFO)<<"Got APP_MSG_NOTIFY_LOCAL_DEVICES";
                Device dev;

                for (std::vector<cricket::Device>::iterator audCapIt = devices.audioCaptureDevices.begin(); audCapIt != devices.audioCaptureDevices.end(); ++audCapIt)
                {
                    dev.name = audCapIt->name;
                    dev.id = audCapIt->id;
                    mAudioCaptureDevices.push_back(dev);
                }
                for (std::vector<cricket::Device>::iterator audPlayIt = devices.audioPlayoutDevices.begin(); audPlayIt != devices.audioPlayoutDevices.end(); ++audPlayIt)
                {
                    dev.name = audPlayIt->name;
                    dev.id = audPlayIt->id;
                    mAudioPlayoutDevices.push_back(dev);
                }
                for (std::vector<cricket::Device>::iterator vidCapIt = devices.videoCaptureDevices.begin(); vidCapIt != devices.videoCaptureDevices.end(); ++vidCapIt)
                {
                    dev.name = vidCapIt->name;
                    dev.id = vidCapIt->id;
                    mVideoCaptureDevices.push_back(dev);
                }
                /*int idx = 0;
                mSipManager->FindVideoDeviceIdx(mScreenDeviceId.c_str(),
                                                mScreenDeviceName.c_str(),
                                                &idx, PJMEDIA_DIR_CAPTURE);
                pjmedia_vid_dev_param param;
                pjmedia_vid_dev_default_param(NULL, idx, &param);
                mScreenWidth   = param.fmt.det.vid.size.w;
                mScreenHeight = param.fmt.det.vid.size.h;*/
            }
            initMediaEngine();
            break;
        case bjn_sky::APP_MSG_CREATE_LOCAL_STREAM:
            {
                LOG(LS_INFO)<<"Got APP_MSG_CREATE_LOCAL_STREAM";
				if (showWindow) showWindow(LOCAL_STREAM_INDEX);
                mSipManager->postCaptureDevOn();
                mSipManager->postAddLocalRenderer(mWnd[0]);
                delete pmsg->pdata;
                startCall();
            }
            break;
        case bjn_sky::APP_MSG_PC_CREATE_REMOTE_STREAM:
            {
                LOG(LS_INFO)<<"Got APP_MSG_PC_CREATE_REMOTE_STREAM";
                talk_base::TypedMessageData<CreateRemoteStream> *data =
                static_cast<talk_base::TypedMessageData<CreateRemoteStream> *>(pmsg->pdata);
                CreateRemoteStream msg_param = data->data();
				//Create the remote stream
				if (msg_param.med_idx == 2) {
					//Show content in main window
					mSipManager->postRemoveRemoteRenderer(1);
					mSipManager->postUpdateRemoteRenderer(mWnd[1], 2);
					mSipManager->postAddRemoteRenderer(2);

					//Show remote in pip
					mSipManager->postRemoveLocalRenderer();
					mSipManager->postUpdateRemoteRenderer(mWnd[0], 1);
					mSipManager->postAddRemoteRenderer(1);
					mCallback("CONTENT_STARTED");
				}
				else if (msg_param.med_idx == 1) {
					showRemoteVideo(0, 0, 0, 0, NULL);
					mCallback("SHOWREMOTE");
				}
				delete data;
            }
            break;             
        case bjn_sky::APP_MSG_PC_CALL_STATUS:
            {
                LOG(LS_INFO)<<"Call setup successful";
                talk_base::TypedMessageData<int> *data = static_cast<talk_base::TypedMessageData<int> *>(pmsg->pdata);
                int errorCode = data->data();
                if (errorCode == ERROR_NONE) std::cout<<"Video Call setup successful " << std::endl;
				mCallback("CONNECTED");
                delete data;
            }
            break;
        case bjn_sky::APP_MSG_PC_REMOVE_REMOTE_STREAM:
            {
                LOG(LS_INFO)<<"Got APP_MSG_PC_REMOVE_REMOTE_STREAM";
                talk_base::TypedMessageData<int> *data = static_cast<talk_base::TypedMessageData<int> *>(pmsg->pdata);
				int med_idx = data->data(); 
				if (med_idx == 2) {
					//Show self in pip
					mSipManager->postRemoveRemoteRenderer(1);
					mSipManager->postAddLocalRenderer(mWnd[0]);

					//Show remote in main window
					mSipManager->postRemoveRemoteRenderer(2);
					mSipManager->postUpdateRemoteRenderer(mWnd[1], 1);
					mSipManager->postAddRemoteRenderer(1);
					mCallback("CONTENT_STOPPED");
				}
                delete data;
            }
            break;            
        case bjn_sky::APP_MSG_PRESENTATION_REQ_RESP:
            {
                talk_base::TypedMessageData<const bool> *data = static_cast<talk_base::TypedMessageData<const bool> *>(pmsg->pdata);
                if(data->data()) {
                    std::cout << "Starting screen share" << std::endl;
                    mSipManager->postPresentationStart(mScreenDeviceName, mScreenDeviceId, 0, 0);
                } else {
                    std::cout << "Denied screen share" << std::endl;
                }
                delete data;
            }
            break;            
        case bjn_sky::APP_MSG_PRESENTATION_IND:
            {
                talk_base::TypedMessageData<const bool> *data = static_cast<talk_base::TypedMessageData<const bool> *>(pmsg->pdata);
                delete data;
            }
            break;
        case bjn_sky::APP_MSG_NOTIFY_CALL_DISCONNECT : 
            {
                talk_base::TypedMessageData<DisconnectParam> *data = static_cast<talk_base::TypedMessageData<DisconnectParam> *>(pmsg->pdata);
                DisconnectParam disconnectParam = data->data();
				mCallback("DISCONNECTED");
				terminateCall = true;
                delete data;
            }
            break;
		case bjn_sky::APP_MSG_VIDEO_MUTE_STATE: {
			talk_base::TypedMessageData<bool> *data = static_cast<talk_base::TypedMessageData<bool> *>(pmsg->pdata);
			bool muted = data->data();
			if (muted)
				mCallback("AUDIO_MUTED");
			else
				mCallback("AUDIO_UNMUTED");
			delete data;
			break;
		}
		case bjn_sky::APP_MSG_AUDIO_MUTE_STATE: {
			talk_base::TypedMessageData<bool> *data = static_cast<talk_base::TypedMessageData<bool> *>(pmsg->pdata);
			bool muted = data->data();
			if (muted)
				mCallback("VIDEO_MUTED");
			else
				mCallback("VIDEO_UNMUTED");
			delete data;
			break;
		}
        default:
            LOG(LS_WARNING)<<"Got unknown message"<<pmsg->message_id;
            break;
    }
}

void CallApi::tokenRequest()
{
    mSipManager->postTokenReq();
}

void CallApi::tokenRelease()
{
    std::cout << "Stoping screen share" << std::endl;
    mSipManager->postTokenRelease();
    mSipManager->postPresentationStop();
}

class Input : public boost::enable_shared_from_this<Input>
{
public:
    typedef boost::shared_ptr<Input> Ptr;

public:
    static void create(boost::asio::io_service& io_service, CallApi *capi)
    {
        Ptr input(new Input(io_service, capi));
        input->read();
    }

private:
    explicit Input(boost::asio::io_service& io_service, CallApi *capi)
        : _input(io_service)
        , _capi(capi)
    {
       #ifdef BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE 	
			HANDLE handle = ::CreateFileA( "test.txt",
									GENERIC_READ,
									FILE_SHARE_READ,
									NULL,
									OPEN_ALWAYS,
									FILE_FLAG_OVERLAPPED,
									NULL );
			_input.assign(handle);
		#else
			_input.assign(STDIN_FILENO);
		#endif
    }

    void read()
    {
        boost::asio::async_read(
                _input,
                boost::asio::buffer(&_command, sizeof(_command)),
                boost::bind(
                    &Input::read_handler,
                    shared_from_this(),
                    boost::asio::placeholders::error,
                    boost::asio::placeholders::bytes_transferred
                    )
                );
    }

    void read_handler(const boost::system::error_code& error, const size_t bytes_transferred)
    {
        if (error) {
            std::cerr << "read error: " << boost::system::system_error(error).what() << std::endl;
            return;
        }

        if (_command != '\n') {
            switch(_command) {
               case 'S': //Start presentation
                   _capi->tokenRequest();
               break;
               case 'K': //Stop presentation
                   _capi->tokenRelease();
               break;
            }
        }

        this->read();
    }
private:
#ifdef BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE 	
	boost::asio::windows::stream_handle _input;
#else
	boost::asio::posix::stream_descriptor _input;
#endif
	char _command;
    CallApi *_capi;
};

void deadline_handler(const boost::system::error_code&, boost::asio::deadline_timer* t)
{
	if (terminateCall) {
        t->get_io_service().stop();
        return;
    }

    t->expires_at(t->expires_at() + boost::posix_time::seconds(1));
    t->async_wait(boost::bind(deadline_handler, boost::asio::placeholders::error, t));
}

void CallApi::runIoService()
{
    boost::asio::deadline_timer t(mIoService, boost::posix_time::seconds(1));
    t.async_wait(boost::bind(deadline_handler, boost::asio::placeholders::error, &t));
    Input::create(mIoService, this);
    
    mIoService.run();
    
    hangupCall();
    deinitSip();
}

LRESULT CallApi::HandleWindowMessage(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#if (defined FB_WIN) && (!defined FB_WIN_D3D)
	for (int i = 0; i < 2; i++) {
		if (hwnd == mUiWidget[i]->GetWindowHandle())
		{
			return mUiWidget[i]->HandleWindowMessage(uMsg, wParam, lParam);
		}
	}
#endif
	return NULL;
}
