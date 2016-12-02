/*
 * Copyright (C) 2010 Regis Montoya (aka r3gis - www.r3gis.fr)
 * This file is part of pjsip_android.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "skinnysipmanager.h"
#include "webrtc_capture_dev.h"

#include <pjmedia-videodev/videodev_imp.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/os.h>
#include <pj/rand.h>

#define THIS_FILE "webrtc_capture_dev.cpp"

#include "modules/utility/source/frame_scaler.h"
#include "modules/video_capture/include/video_capture_factory.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "webrtc/common_types.h"
#include <limits>

#include "bjndevices.h"
#include "config_handler.h"
#ifdef FB_MACOSX
#include "Mac/macUtils.h"
#elif FB_X11
#include "X11/linuxUtils.h"
#elif FB_WIN
#include "Win/winUtils.h"
#endif

#include <pjmedia/ref_frame.h>
#include "system_wrappers/interface/scoped_refptr.h"
#include "system_wrappers/interface/ref_count.h"

using namespace webrtc;

#define DEFAULT_SCREEN_SHARE_WIDTH 1920
#define DEFAULT_SCREEN_SHARE_HEIGHT 1080

#define MAX_CAP_SUPPORTED 5
#define ANY_FPS           -1
#define MAX_FPS           30

struct filter_cam {
    char vend_id[MAX_GUID_LENGTH];
    char prod_id[MAX_GUID_LENGTH];
    char device_name[MAX_GUID_LENGTH];
};

#define MAX_LIST_SIZE 1
filter_cam filter_list[MAX_LIST_SIZE] = {
    {"vid_046d", "pid_0828", "logitech B990"},
};

struct capabilitySupported {
    int width;
    int height;
    int maxFPS;
};

#define NON_HD_CAP_IDX 1 //This is a index to a non hd capability in following array.
//Capability arrary skinny want to support.
capabilitySupported capability_supported[MAX_CAP_SUPPORTED] = {
    {1280, 720, MAX_FPS},
    {640,  480, ANY_FPS},
    {640,  360, ANY_FPS},
    {320,  240, ANY_FPS},
    {320,  180, ANY_FPS},
};

/* webrtc_cap_ device info */
struct webrtc_cap_dev_info {
	pjmedia_vid_dev_info info;
	char webrtc_id[256];
	char product_id[256];
	VideoCaptureCapability _capability[PJMEDIA_VID_DEV_INFO_FMT_CNT]; /**< The capability to use for this stream */
};

/* webrtc_cap_ factory */
struct webrtc_cap_factory {
	pjmedia_vid_dev_factory base;
	pj_pool_t *pool;
	pj_pool_factory *pf;

	unsigned dev_count;
	struct webrtc_cap_dev_info *dev_info;
	VideoCaptureModule::DeviceInfo* _deviceInfo;
    bjn_sky::skinnySipManager* sipManager;
};

/* Video stream. */
struct webrtc_cap_stream {
    pjmedia_vid_dev_stream base; /**< Base stream	    */
    pjmedia_vid_dev_param param; /**< Settings	    */
    pj_pool_t *pool; /**< Memory pool.       */

    pjmedia_vid_dev_cb vid_cb; /**< Stream callback.   */
    void *user_data; /**< Application data.  */

    pj_bool_t		     cap_thread_initialized;
    pj_bool_t			 window_available; /** < True if a window preview is available and if start stream is useful*/
    pj_bool_t			 capturing; /** < True if we should be capturing frames for this stream */

    VideoCaptureModule* _videoCapture;
    VideoCaptureCapability* _capability; /**< The capability to use for this stream */
    VideoCaptureDataCallback* _captureDataCb;
    FrameScaler *_frame_scaler;
    webrtc_cap_factory *sf;
    char *webrtc_id;
};

/* Prototypes */
static pj_status_t webrtc_cap_factory_init(pjmedia_vid_dev_factory *f);
static pj_status_t webrtc_cap_factory_destroy(pjmedia_vid_dev_factory *f);
static pj_status_t webrtc_cap_factory_refresh(pjmedia_vid_dev_factory *f);
static unsigned webrtc_cap_factory_get_dev_count(pjmedia_vid_dev_factory *f);
static pj_status_t webrtc_cap_factory_get_dev_info(pjmedia_vid_dev_factory *f,
		unsigned index, pjmedia_vid_dev_info *info);
static pj_status_t webrtc_cap_factory_default_param(pj_pool_t *pool,
		pjmedia_vid_dev_factory *f, unsigned index,
		pjmedia_vid_dev_param *param);
static pj_status_t webrtc_cap_factory_create_stream(pjmedia_vid_dev_factory *f,
		pjmedia_vid_dev_param *param, const pjmedia_vid_dev_cb *cb,
		void *user_data, pjmedia_vid_dev_stream **p_vid_strm);

static pj_status_t webrtc_cap_stream_get_param(pjmedia_vid_dev_stream *strm,
		pjmedia_vid_dev_param *param);
static pj_status_t webrtc_cap_stream_get_cap(pjmedia_vid_dev_stream *strm,
		pjmedia_vid_dev_cap cap, void *value);
static pj_status_t webrtc_cap_stream_set_cap(pjmedia_vid_dev_stream *strm,
		pjmedia_vid_dev_cap cap, const void *value);
static pj_status_t webrtc_cap_stream_start(pjmedia_vid_dev_stream *strm);
static pj_status_t webrtc_cap_stream_stop(pjmedia_vid_dev_stream *strm);
static pj_status_t webrtc_cap_stream_destroy(pjmedia_vid_dev_stream *strm);
static void getDesireWH(int captured_width,int captured_height,webrtc_cap_dev_info *ddi);
extern "C" {
pj_int32_t ref_frame_add_ref(void *ref_frame_obj);
pj_int32_t ref_frame_dec_ref(void *ref_frame_obj);
}

const char* getRawTypeString(int type)
{
	switch(type)
	{
		case kVideoI420: return "I420";
		case kVideoYV12: return "YV12";
		case kVideoYUY2: return "YUY2";
		case kVideoUYVY: return "UYVY";
		case kVideoIYUV: return "IYUV";
		case kVideoARGB: return "ARGB";
		case kVideoRGB24: return "RGB24";
		case kVideoRGB565: return "RGB565";
		case kVideoARGB4444: return "ARGB4444";
		case kVideoARGB1555: return "ARGB1555";
		case kVideoMJPEG: return "MJPG";
		case kVideoNV12: return "NV12";
		case kVideoNV21: return "NV21";
		case kVideoBGRA: return "BGRA";
		default: break;
	}
	return "????";
}

bool isRawTypeYUV(int type)
{
	switch(type)
	{
		case kVideoI420:
		case kVideoYV12:
		case kVideoYUY2:
		case kVideoUYVY:
		case kVideoIYUV:
		case kVideoNV12:
		case kVideoNV21:
			return true;
		default: break;
	}
	return false;
}

void calculateDesiredWH(int captured_width, int captured_height, int& width, int& height)
{
    if(captured_width <= DEFAULT_SCREEN_SHARE_WIDTH && captured_height <= DEFAULT_SCREEN_SHARE_HEIGHT) {
        width = captured_width;
        height = captured_height;
    } else {
        //These calculations needs to be tewaked for small differences in ration like 1.77777778 and 1.776 and creates color conversion effects.
        double default_screen_share_aspect_ration = static_cast<double>(DEFAULT_SCREEN_SHARE_WIDTH) / DEFAULT_SCREEN_SHARE_HEIGHT;
        double recv_frame_aspect_ratio = static_cast<double>(captured_width)/captured_height;
        if(recv_frame_aspect_ratio == default_screen_share_aspect_ration) {
            width = DEFAULT_SCREEN_SHARE_WIDTH;
            height = DEFAULT_SCREEN_SHARE_HEIGHT;
        } else if(recv_frame_aspect_ratio > default_screen_share_aspect_ration) {
            width = DEFAULT_SCREEN_SHARE_WIDTH;
            height = 1 + static_cast<unsigned short>(DEFAULT_SCREEN_SHARE_WIDTH / recv_frame_aspect_ratio);
            height &= ~1;
            
        } else {
            width = 1 + static_cast<unsigned short>(DEFAULT_SCREEN_SHARE_HEIGHT * recv_frame_aspect_ratio);
            height = DEFAULT_SCREEN_SHARE_HEIGHT;
            width &= ~1;
        }
    }
}

void getDesireWH(int captured_width,int captured_height,webrtc_cap_dev_info *ddi)
{
    int width, height;
    calculateDesiredWH(captured_width, captured_height, width, height);
    ddi->_capability[0].width = width;
    ddi->_capability[0].height = height;
}

int adjustHeightToKeepRatio(int width,int ratiow,int ratioh)
{
   int h = ((ratioh * width) / ratiow);
   //align to 8 byte iff its a height is odd number.
   return ((h % 2) == 0 ? h : h&~7);
}

/* Operations */
static pjmedia_vid_dev_factory_op factory_op = {
		&webrtc_cap_factory_init,
		&webrtc_cap_factory_destroy,
		&webrtc_cap_factory_get_dev_count,
		&webrtc_cap_factory_get_dev_info,
		&webrtc_cap_factory_default_param,
		&webrtc_cap_factory_create_stream,
		&webrtc_cap_factory_refresh
};

static pjmedia_vid_dev_stream_op stream_op = {
		&webrtc_cap_stream_get_param,
		&webrtc_cap_stream_get_cap,
		&webrtc_cap_stream_set_cap,
		&webrtc_cap_stream_start,
		NULL,
		NULL,
		&webrtc_cap_stream_stop,
		&webrtc_cap_stream_destroy
};

/****************************************************************************
 * Factory operations
 */
/*
 * Init webrtc_cap_ video driver.
 */
pjmedia_vid_dev_factory* pjmedia_webrtc_vid_capture_factory(
		pj_pool_factory *pf,
        bjn_sky::skinnySipManager* sipManager)
{
	struct webrtc_cap_factory *f;
	pj_pool_t *pool;

	pool = pj_pool_create(pf, "webrtc camera", 1000, 1000, NULL);
	f = PJ_POOL_ZALLOC_T(pool, struct webrtc_cap_factory);
	f->pf = pf;
	f->pool = pool;
	f->base.op = &factory_op;
    f->sipManager = sipManager;

	return &f->base;
}

/* API: init factory */
static pj_status_t webrtc_cap_factory_init(pjmedia_vid_dev_factory *f) {
    PJ_LOG(4, (THIS_FILE, "Init webrtc Capture factory"));
    return webrtc_cap_factory_refresh(f);
}

/* API: destroy factory */
static pj_status_t webrtc_cap_factory_destroy(pjmedia_vid_dev_factory *f) {
    struct webrtc_cap_factory *cf = (struct webrtc_cap_factory*) f;
    delete cf->_deviceInfo;
    pj_pool_t *pool = cf->pool;
    cf->pool = NULL;
    pj_pool_release(pool);
    return PJ_SUCCESS;
}

static pj_status_t webrtc_refresh_screen_device(webrtc_cap_stream *stream, int width, int height,
                                                int &desired_width, int &desired_height) {
    struct webrtc_cap_dev_info *ddi;
    unsigned d;
    webrtc_cap_factory *cf = stream->sf;
    PJ_LOG(4, (THIS_FILE, "Refresh screen share device"));

	for (d = 0; d < cf->dev_count; d++) {
		ddi = &cf->dev_info[d];
		PJ_LOG(4, (THIS_FILE, "Found %2d > %s aka %s", d, ddi->info.name, ddi->info.driver));
        if(strcmp(ddi->webrtc_id, stream->webrtc_id) == 0) {
            getDesireWH(width,height,ddi);
            desired_width = ddi->_capability[0].width;
            desired_height = ddi->_capability[0].height;

            PJ_LOG(4, (THIS_FILE, "New Capability for screen device: Type %d - Codec %d, %dx%d @%dHz" ,
               ddi->_capability[0].rawType,
               ddi->_capability[0].codecType,
               ddi->_capability[0].width,
               ddi->_capability[0].height,
               ddi->_capability[0].maxFPS));
            pjmedia_format *fmt = &ddi->info.fmt[0];
            if(ddi->_capability[0].codecType == kVideoCodecUnknown)
            {
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_I420,
                        ddi->_capability[0].width,
                        ddi->_capability[0].height,
                        ddi->_capability[0].maxFPS,
                        1);

                fmt = &ddi->info.fmt[1];
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_RGB32,
                    ddi->_capability[1].width,
                    ddi->_capability[1].height,
                    ddi->_capability[1].maxFPS,
                    1);

                ddi->info.fmt_cnt = 2;
            }
            break;
        }
	}
	return PJ_SUCCESS;
}

bool populateSupportedCapability(webrtc_cap_factory *cf, int num_cam_capability,
                                 struct webrtc_cap_dev_info *ddi, int fmt_cnt,
                                 int width, int height, int fps)
{
	PJ_LOG(4, (THIS_FILE, "Looking for format %dx%d @%dHz" ,
            width,
            height,
            fps));

    webrtc::VideoCaptureCapability tmp_cap;
    bool capAdded   = false;
    int i = 0;
    for (i = 0; i < num_cam_capability; i++)
    {
        cf->_deviceInfo->GetCapability(ddi->webrtc_id, i, tmp_cap);

        if(tmp_cap.rawType == kVideoMJPEG) {
#if defined(FB_X11)
            if(false == cf->sipManager->GetMeetingFeatures().mMjpegLinux) {
                PJ_LOG(4, (THIS_FILE, "Ignoring MJPEG format for camera"));
                continue;
            }
#endif

            for(int i =0; i < MAX_LIST_SIZE; i++) {
                if(strstr(ddi->webrtc_id, filter_list[i].vend_id) &&
                   strstr(ddi->webrtc_id, filter_list[i].prod_id)) {
                       PJ_LOG(4, (THIS_FILE, "Ignoring MJPEG format for camera %s", filter_list[i].device_name));
                       continue;
                }
            }
        }

        PJ_LOG(4, (THIS_FILE, "Type %s - Codec %d, %dx%d @%dHz" ,
            getRawTypeString(tmp_cap.rawType),
            tmp_cap.codecType,
            tmp_cap.width,
            tmp_cap.height,
            tmp_cap.maxFPS));

        if(BJN::DevicePropertyManager::isScreenSharingDevice(ddi->webrtc_id)) {
            /*ScreenShare can support both I420 and RGB capture formats*/
            if(fmt_cnt > 1) return false;
            ddi->_capability[fmt_cnt] = tmp_cap;
            capAdded = true;
            getDesireWH(tmp_cap.width,tmp_cap.height,ddi);
            break;
        }
        else if(tmp_cap.width == width && tmp_cap.height == height && 
                (tmp_cap.maxFPS == fps || fps == ANY_FPS))
        {
			if(capAdded)
			{
				if(isRawTypeYUV(ddi->_capability[fmt_cnt].rawType) && !isRawTypeYUV(tmp_cap.rawType))
					continue;
			}
            ddi->_capability[fmt_cnt] = tmp_cap;
            capAdded = true;
			if(tmp_cap.rawType == kVideoI420 || tmp_cap.rawType == kVideoYV12)
			{
				PJ_LOG(4, (THIS_FILE, "%s is favored, skipping other formats" ,
					getRawTypeString(tmp_cap.rawType)));
				break;
			}
        }
    }

    if(capAdded)
    {
        PJ_LOG(4, (THIS_FILE, "Capability Added: Type %s - Codec %d, %dx%d @%dHz" ,
            getRawTypeString(ddi->_capability[fmt_cnt].rawType),
            ddi->_capability[fmt_cnt].codecType,
            ddi->_capability[fmt_cnt].width,
            ddi->_capability[fmt_cnt].height,
            ddi->_capability[fmt_cnt].maxFPS));

        pjmedia_format *fmt = &ddi->info.fmt[fmt_cnt];
        if(ddi->_capability[fmt_cnt].codecType == kVideoCodecUnknown)
        {
            // WebRTC automatically transform once from rawType to I420
            // So we don't need to convert here
            // BTW, for now we ignore optimized video codecs since seems touchy to add to pjsip
            // And anyway no device in my hands supports that for now
            if(BJN::DevicePropertyManager::isScreenSharingDevice(ddi->webrtc_id) && fmt_cnt)
            {
                PJ_LOG(4, (THIS_FILE, "Populating RGB32 as the supported format"));
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_RGB32,
                ddi->_capability[fmt_cnt].width,
                ddi->_capability[fmt_cnt].height,
                ddi->_capability[fmt_cnt].maxFPS,
                1);
            }
            else
            {
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_I420,
                ddi->_capability[fmt_cnt].width,
                ddi->_capability[fmt_cnt].height,
                ddi->_capability[fmt_cnt].maxFPS,
                1);
            } 
        }
    }
    else
    {
        PJ_LOG(4, (THIS_FILE, "No Capability Added for %dX%d @%d Hz", width, height, fps));
    }
    return capAdded;
}

bool captureRestart(webrtc_cap_factory *cf, std::string productId)
{
    std::string key =  BJN::getKeyFromCameraProductId(productId);

    const Config_Parser_Handler& cph = cf->sipManager->getConfigParser();
    const Config_Parser_Handler::RestartCaptureMap& deviceMap = cph.getRestartCaptureMap();
    
    Config_Parser_Handler::RestartCaptureMap::const_iterator it = deviceMap.find(key);
    
    if(deviceMap.end() != it) {
        return it->second;
    }

    return true;
}

bool populateCapabilityFromDatabase(webrtc_cap_factory *cf, std::string name,
                                    std::string uniqueId, std::string productId,
                                    struct webrtc_cap_dev_info *ddi)
{
    bool success = false;
#ifdef FB_MACOSX
    
    const Config_Parser_Handler& cph = cf->sipManager->getConfigParser();
    const Config_Parser_Handler::VideoDevicePropertyMap& deviceMap = cph.getVideoDevicePropertiesMap();
    
    Config_Parser_Handler::VideoDevicePropertyMap::const_iterator it = deviceMap.find(productId);
    
    if(deviceMap.end() != it)
    {
        PJ_LOG(4,(THIS_FILE, "found device in database name: %s, id: %s",name.c_str(), productId.c_str()));
        
        for(Config_Parser_Handler::VideoDevicePropertiesList::const_iterator propIt = it->second.begin();
            it->second.end() != propIt;
            ++propIt)
        {
            if(propIt->width !=0 && propIt->height != 0 && propIt->fps != 0)
            {
                PJ_LOG(4, (THIS_FILE, "Adding capbility from database: %dx%d@%d fps",propIt->width, propIt->height, propIt->fps));
                ddi->_capability[ddi->info.fmt_cnt].width = propIt->width;
                ddi->_capability[ddi->info.fmt_cnt].height = propIt->height;
                ddi->_capability[ddi->info.fmt_cnt].maxFPS = propIt->fps;
                ddi->_capability[ddi->info.fmt_cnt].rawType = kVideoI420;
                ddi->_capability[ddi->info.fmt_cnt].codecType = kVideoCodecUnknown;
                pjmedia_format *fmt = &ddi->info.fmt[ddi->info.fmt_cnt];
                int fps = (ddi->_capability[ddi->info.fmt_cnt].maxFPS > MAX_FPS) ? MAX_FPS
                    : ddi->_capability[ddi->info.fmt_cnt].maxFPS;
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_I420,
                                      ddi->_capability[ddi->info.fmt_cnt].width,
                                      ddi->_capability[ddi->info.fmt_cnt].height,
                                      fps, 1);
                ddi->info.fmt_cnt++;
                success = true;
            }
        }
        
    }
    
#endif
    
    return success;
}

/* API: refresh the list of devices */
static pj_status_t webrtc_cap_factory_refresh(pjmedia_vid_dev_factory *f) {
    struct webrtc_cap_factory *cf = (struct webrtc_cap_factory*) f;
	struct webrtc_cap_dev_info *ddi;
	int i;
    unsigned d;
    PJ_LOG(4, (THIS_FILE, "Refresh webrtc Capture factory"));

    bjn_sky::VideoCodecParams params = cf->sipManager->getVideoCodecParams();
    bool HDCapable = false;
    PJ_LOG(4, (THIS_FILE, "CPU resolution capability %dX%d", params.enc.width, params.enc.height));
    if(params.enc.width > capability_supported[NON_HD_CAP_IDX].width || params.enc.height > capability_supported[NON_HD_CAP_IDX].height)
    {
        HDCapable = true;
        PJ_LOG(4, (THIS_FILE, "CPU params support HD"));
    }

    cf->_deviceInfo = VideoCaptureFactory::CreateDeviceInfo(0);
	cf->dev_count = cf->_deviceInfo->NumberOfDevices();

    cf->dev_info = (struct webrtc_cap_dev_info*) pj_pool_calloc(cf->pool, cf->dev_count, sizeof(struct webrtc_cap_dev_info));    

	for (d = 0; d < cf->dev_count; d++) {
		ddi = &cf->dev_info[d];
		pj_bzero(ddi, sizeof(*ddi));

		// Get infos from webRTC
		char name[256];
        unsigned cam_index = d;
        
		cf->_deviceInfo->GetDeviceName(
                                       cam_index,
                                       name,
                                       256,
                                       ddi->webrtc_id,
                                       sizeof(ddi->webrtc_id),
                                       ddi->product_id,
                                       sizeof(ddi->product_id));

		// Copy to pj struct
		pj_ansi_strncpy(ddi->info.name, (char *) name, sizeof(ddi->info.name));
		ddi->info.name[sizeof(ddi->info.name) - 1] = '\0';
#if defined(LINUX)
        if(BJN::DevicePropertyManager::isScreenSharingDevice(ddi->webrtc_id)) {
		    pj_ansi_strncpy(ddi->info.driver, (char *) ddi->webrtc_id, sizeof(ddi->info.driver));
        } else {
		    pj_ansi_strncpy(ddi->info.driver, (char *) name, sizeof(ddi->info.driver));
        }
#else
		pj_ansi_strncpy(ddi->info.driver, (char *) ddi->webrtc_id, sizeof(ddi->info.driver));
#endif
		ddi->info.driver[sizeof(ddi->info.driver) - 1] = '\0';

		ddi->info.dir = PJMEDIA_DIR_CAPTURE;
		ddi->info.has_callback = PJ_TRUE;
		ddi->info.caps = PJMEDIA_VID_DEV_CAP_FORMAT
                       | PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;

		PJ_LOG(4, (THIS_FILE, "Found %2d > %s aka %s", d, ddi->info.name, ddi->info.driver));

		pj_log_push_indent();

        if(true == cf->sipManager->GetMeetingFeatures().mMacCameraCaps) {
            PJ_LOG(4, (THIS_FILE, "Enabling the Mac Camera capability "
                                  "related changes"));
            cf->_deviceInfo->EnableMacCameraCapability(true);
        }

		ddi->info.fmt_cnt = 0;
        int nbrOfCaps = cf->_deviceInfo->NumberOfCapabilities(ddi->webrtc_id);
        PJ_LOG(4, (THIS_FILE, "Found %2d has %d capabilities", d, nbrOfCaps));
        if(nbrOfCaps > 0) {
            i = HDCapable ? 0 : 1; //If not HD capable cpu then ignore HD entry of supported.
            for(; i < MAX_CAP_SUPPORTED; i++) {
                bool added = populateSupportedCapability(cf, 
                                                            nbrOfCaps, 
                                                            ddi, 
                                                            ddi->info.fmt_cnt, 
                                                            capability_supported[i].width,
                                                            capability_supported[i].height,
                                                            capability_supported[i].maxFPS);
                if(added) ddi->info.fmt_cnt++;
                //In case of screen share an additional capability for RGB format is added
                if(BJN::DevicePropertyManager::isScreenSharingDevice(ddi->webrtc_id))
                {
                    bool added = populateSupportedCapability(cf,
                                                            nbrOfCaps,
                                                            ddi,
                                                            ddi->info.fmt_cnt,
                                                            capability_supported[i].width,
                                                            capability_supported[i].height,
                                                            capability_supported[i].maxFPS);
                    if(added) ddi->info.fmt_cnt++;
                }
            }
            /* If populateSupportedCapability not added any capabilities, add one capablity.
            *  Adding more than one capability wont help here because, the input resolution
            *  is set by input source, Currently we dont have any way to know the set capabilty
            *  and dont have any mechanism to show property page of filter for settings.
            *  From observation WDM Driver always haveing one capablity which is set by user.
            *  this change will work with WDM Drivers ex. AV/C Tape Recorder/Player,BlackMagic WDM Capture.
            */
            if(0 == ddi->info.fmt_cnt)
            {
                PJ_LOG(4,(THIS_FILE,"populateSupportedCapability failed to add capablity"));
                VideoCaptureCapability capability,requestedCapability;
                int32_t capabilityIndex;
                if(HDCapable)
                {
                    PJ_LOG(4,(THIS_FILE,"HDcapable Device: Finding closest match to 720p"));
                    requestedCapability.codecType = kVideoCodecUnknown;
                    requestedCapability.expectedCaptureDelay = 0;
                    requestedCapability.height = 720;
                    requestedCapability.interlaced = false;
                    requestedCapability.maxFPS = 30;
                    requestedCapability.rawType = kVideoI420;
                    requestedCapability.width = 1280;
                }
                else
                {
                    PJ_LOG(4,(THIS_FILE,"Not HDcapable Device: Finding closest match to 360p"));
                    requestedCapability.codecType = kVideoCodecUnknown;
                    requestedCapability.expectedCaptureDelay = 0;
                    requestedCapability.height = 360;
                    requestedCapability.interlaced = false;
                    requestedCapability.maxFPS = 30;
                    requestedCapability.rawType = kVideoI420;
                    requestedCapability.width = 640;
                }
                if ((capabilityIndex = cf->_deviceInfo->GetBestMatchedCapability(ddi->webrtc_id,requestedCapability, capability)) < 0)
                {
                    /*Ideally above function can't fail*/
                    PJ_LOG(4,(THIS_FILE,"GetBestMatchedCapability Failed: Adding first available capabilty"));
                    cf->_deviceInfo->GetCapability(ddi->webrtc_id, 0, capability);
                }
                PJ_LOG(4, (THIS_FILE, "Type %s - Codec %d, %dx%d @%dHz" ,
                    getRawTypeString(capability.rawType),
                    capability.codecType,
                    capability.width,
                    capability.height,
                    capability.maxFPS));
                ddi->_capability[ ddi->info.fmt_cnt] = capability;
                pjmedia_format *fmt = &ddi->info.fmt[ddi->info.fmt_cnt];
                if(ddi->_capability[ddi->info.fmt_cnt].codecType == kVideoCodecUnknown)
                {
                    int fps = (ddi->_capability[ddi->info.fmt_cnt].maxFPS > MAX_FPS) ? MAX_FPS : ddi->_capability[ddi->info.fmt_cnt].maxFPS;
                    pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_I420,
                        ddi->_capability[ddi->info.fmt_cnt].width,
                        ddi->_capability[ddi->info.fmt_cnt].height,
                        fps,
                        1);
                }
                ddi->info.fmt_cnt++;
            }
        }else if(populateCapabilityFromDatabase(cf, name, ddi->webrtc_id, ddi->product_id, ddi)){
            PJ_LOG(4,(THIS_FILE,"Capability added from database for device: %s", name));
        }else{
            //MAC does not have support for capabilities. So populate default one
            ddi->_capability[ddi->info.fmt_cnt].rawType = kVideoI420;
            ddi->_capability[ddi->info.fmt_cnt].codecType = kVideoCodecUnknown;
            ddi->_capability[ddi->info.fmt_cnt].maxFPS = 30;

            // Only do HD for iSight and other HD camera
            if(HDCapable && strstr(name," HD"))
            {
                PJ_LOG(4, (THIS_FILE, "Found HD in device name %s, assuming HD support",name));
                ddi->_capability[ddi->info.fmt_cnt].width = 1280;
                ddi->_capability[ddi->info.fmt_cnt].height = 720;
                ddi->_capability[ddi->info.fmt_cnt].maxFPS = 30;
                pjmedia_format *fmt = &ddi->info.fmt[ddi->info.fmt_cnt];
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_I420,
                                      ddi->_capability[ddi->info.fmt_cnt].width,
                                      ddi->_capability[ddi->info.fmt_cnt].height,
                                      ddi->_capability[ddi->info.fmt_cnt].maxFPS, 1);
                ddi->info.fmt_cnt++;
            }
            {
                ddi->_capability[ddi->info.fmt_cnt].width  = 640;
                ddi->_capability[ddi->info.fmt_cnt].height = 480;
                ddi->_capability[ddi->info.fmt_cnt].maxFPS = 30;
                pjmedia_format *fmt = &ddi->info.fmt[ddi->info.fmt_cnt];
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_I420,
                                      ddi->_capability[ddi->info.fmt_cnt].width,
                                      ddi->_capability[ddi->info.fmt_cnt].height,
                                      ddi->_capability[ddi->info.fmt_cnt].maxFPS, 1);
                ddi->info.fmt_cnt++;
            }
            {
                ddi->_capability[ddi->info.fmt_cnt].width  = 320;
                ddi->_capability[ddi->info.fmt_cnt].height = 240;
                ddi->_capability[ddi->info.fmt_cnt].maxFPS = 30;
                pjmedia_format *fmt = &ddi->info.fmt[ddi->info.fmt_cnt];
                pjmedia_format_init_video(fmt, PJMEDIA_FORMAT_I420,
                                      ddi->_capability[ddi->info.fmt_cnt].width,
                                      ddi->_capability[ddi->info.fmt_cnt].height,
                                      ddi->_capability[ddi->info.fmt_cnt].maxFPS, 1);
                ddi->info.fmt_cnt++;
            }
        }
		pj_log_pop_indent();
	}
	return PJ_SUCCESS;
}

/* API: get number of devices */
static unsigned webrtc_cap_factory_get_dev_count(pjmedia_vid_dev_factory *f) {
	struct webrtc_cap_factory *cf = (struct webrtc_cap_factory*) f;
	return cf->dev_count;
}

static unsigned webrtc_get_capturer_index(struct webrtc_cap_factory *cf, unsigned index)
{
    unsigned ret = 0;
    unsigned i = 0;
    struct webrtc_cap_dev_info *di = &cf->dev_info[index];
    bjn_sky::VideoCodecParams codecParams = cf->sipManager->getVideoCodecParams();
    for(i = 0; i < (di->info.fmt_cnt - 1); i++) {
        if(di->info.fmt[i].det.vid.size.w >= codecParams.enc.width && 
           di->info.fmt[i+1].det.vid.size.w < codecParams.enc.width) {
            ret = i;
            break;
        }
    }

    if(i == (di->info.fmt_cnt - 1) &&
       di->info.fmt[i].det.vid.size.w >= codecParams.enc.width) {
       ret = i;
    }

    return ret;
}

/* API: get device info */
static pj_status_t webrtc_cap_factory_get_dev_info(pjmedia_vid_dev_factory *f,
		unsigned index, pjmedia_vid_dev_info *info) {
    struct webrtc_cap_factory *cf = (struct webrtc_cap_factory*) f;
    struct webrtc_cap_dev_info *di = &cf->dev_info[index];

    PJ_ASSERT_RETURN(index < cf->dev_count, PJMEDIA_EVID_INVDEV);
    pj_memcpy(info, &cf->dev_info[index].info, sizeof(*info));
    
    if(!BJN::DevicePropertyManager::isScreenSharingDevice(di->webrtc_id))
    {
        for(int i = 0; i < di->info.fmt_cnt; i++) {
            info->fmt[i].det.vid.size.h = adjustHeightToKeepRatio(info->fmt[i].det.vid.size.w,16,9);
        }
    }
    if(!captureRestart(cf, di->product_id) && cf->sipManager->GetMeetingFeatures().mCameraRestart) {
        info->fmt_cnt = 1;
    }
    return PJ_SUCCESS;
}

/* API: create default device parameter */
static pj_status_t webrtc_cap_factory_default_param(pj_pool_t *pool,
		pjmedia_vid_dev_factory *f, unsigned index,
		pjmedia_vid_dev_param *param) {
    struct webrtc_cap_factory *cf = (struct webrtc_cap_factory*) f;
    struct webrtc_cap_dev_info *di = &cf->dev_info[index];

    PJ_ASSERT_RETURN(index < cf->dev_count, PJMEDIA_EVID_INVDEV);
    PJ_UNUSED_ARG(pool);

    pj_bzero(param, sizeof(*param));
    param->dir = PJMEDIA_DIR_CAPTURE;
    param->cap_id = index;
    param->rend_id = PJMEDIA_VID_INVALID_DEV;
    param->flags = PJMEDIA_VID_DEV_CAP_FORMAT |
            PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;
    param->native_preview = PJ_FALSE;

    int cap_index = webrtc_get_capturer_index(cf, index);
    param->clock_rate = di->info.fmt[cap_index].det.vid.fps.num * 1000;
    pj_memcpy(&param->fmt, &di->info.fmt[cap_index], sizeof(param->fmt));
    
    if(!BJN::DevicePropertyManager::isScreenSharingDevice(di->webrtc_id))
    {
        param->fmt.det.vid.size.h = adjustHeightToKeepRatio(param->fmt.det.vid.size.w,16,9);
    }
    
    // param->vidframe_ref_count = PJ_FALSE;
    return PJ_SUCCESS;
}

static void create_capturer(void *strm1)
{
    struct webrtc_cap_stream *strm = (struct webrtc_cap_stream *) strm1;
    strm->_videoCapture = VideoCaptureFactory::Create(0, strm->webrtc_id);
    if(strm->_videoCapture) {
        strm->_videoCapture->AddRef();
    }
}

/* API: create stream */
static pj_status_t webrtc_cap_factory_create_stream(pjmedia_vid_dev_factory *f,
		pjmedia_vid_dev_param *param,
		const pjmedia_vid_dev_cb *cb,
		void *user_data,
		pjmedia_vid_dev_stream **p_vid_strm) {
    PJ_LOG(4, (THIS_FILE, "In function %s", __FUNCTION__));
	struct webrtc_cap_factory *cf = (struct webrtc_cap_factory*) f;
	pj_pool_t *pool;
	struct webrtc_cap_stream *strm;
	const char* webrtc_id;
	pj_status_t status = PJ_SUCCESS;

	PJ_ASSERT_RETURN(f && param && p_vid_strm, PJ_EINVAL);
    PJ_ASSERT_RETURN(param->fmt.type == PJMEDIA_TYPE_VIDEO &&
			param->fmt.detail_type == PJMEDIA_FORMAT_DETAIL_VIDEO &&
			param->dir == PJMEDIA_DIR_CAPTURE,
			PJ_EINVAL);


	/* Create and Initialize stream descriptor */
	pool = pj_pool_create(cf->pf, "webrtc-capture-dev", 512, 512, NULL);
	PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

	strm = PJ_POOL_ZALLOC_T(pool, struct webrtc_cap_stream);
	pj_memcpy(&strm->param, param, sizeof(*param));
	strm->pool = pool;
	pj_memcpy(&strm->vid_cb, cb, sizeof(*cb));
	strm->user_data = user_data;
    strm->sf = cf;

	webrtc_cap_dev_info *ddi = &cf->dev_info[param->cap_id];

	webrtc_id = ddi->webrtc_id;
    strm->webrtc_id = ddi->webrtc_id;
    if(BJN::DevicePropertyManager::isScreenSharingDevice(webrtc_id)) {
        if(strm->_frame_scaler == NULL) {
           strm->_frame_scaler = new FrameScaler();
        }
    } else {
        if(strm->_frame_scaler != NULL) {
            delete strm->_frame_scaler;
            strm->_frame_scaler = NULL;
        }
    }

#ifdef FB_MACOSX
    run_func_on_main_thread(strm, create_capturer);
#else
    create_capturer(strm);
#endif
	if(!strm->_videoCapture) {
		PJ_LOG(4, (THIS_FILE, "%s : Impossible to create !!!", webrtc_id));
		status = PJ_ENOMEM;
	    return status;
	}

	PJ_LOG(4, (THIS_FILE, "Create for %s with idx %d", webrtc_id, param->cap_id));
	// WARNING : we should NEVER create a capability here because webRTC here is not necessarily
	// Launched in main thread, and as consequence loader may not contains org.webrtc packages.
	// So we can't use the utility tool from webrtc

	strm->_capability = NULL;
    int cap_index = webrtc_get_capturer_index(cf, param->cap_id);
    strm->_capability = &ddi->_capability[cap_index];

    PJ_LOG(4, (THIS_FILE, "Cap choosen %dX%d@%d: ", strm->_capability->width, strm->_capability->height, strm->_capability->maxFPS));
    strm->capturing = PJ_FALSE;
    strm->window_available = PJ_TRUE;

	/* Done */
	strm->base.op = &stream_op;
	*p_vid_strm = &strm->base;

	return status;
}

/* API: Get stream info. */
static pj_status_t webrtc_cap_stream_get_param(pjmedia_vid_dev_stream *s,
		pjmedia_vid_dev_param *pi) {
	struct webrtc_cap_stream *strm = (struct webrtc_cap_stream*) s;

	PJ_ASSERT_RETURN(strm && pi, PJ_EINVAL);

	pj_memcpy(pi, &strm->param, sizeof(*pi));

	return PJ_SUCCESS;
}

/* API: get capability */
static pj_status_t webrtc_cap_stream_get_cap(pjmedia_vid_dev_stream *s,
		pjmedia_vid_dev_cap cap, void *pval) {
	struct webrtc_cap_stream *strm = (struct webrtc_cap_stream*) s;

	PJ_UNUSED_ARG(strm);

	PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

	if (cap == PJMEDIA_VID_DEV_CAP_INPUT_SCALE) {
		return PJMEDIA_EVID_INVCAP;
//	return PJ_SUCCESS;
	} else if(cap == PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW){
		// Actually useless
		*(void **)pval = NULL;
		return PJ_SUCCESS;
	} else {
		return PJMEDIA_EVID_INVCAP;
	}
}

/* API: set capability */
static pj_status_t webrtc_cap_stream_set_cap(pjmedia_vid_dev_stream *s,
		pjmedia_vid_dev_cap cap, const void *pval) {
	struct webrtc_cap_stream *strm = (struct webrtc_cap_stream*) s;

	PJ_ASSERT_RETURN(s, PJ_EINVAL);
    PJ_LOG(4, (THIS_FILE, "In function %s with cap:%d val:%p", __FUNCTION__, cap, pval));

	if (cap == PJMEDIA_VID_DEV_CAP_INPUT_SCALE) {
		return PJ_SUCCESS;
	} else if(cap == PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW){
		// Consider that as the fact window has been set and we could try to re-attach ourself
		if(!strm->window_available){
			PJ_LOG(4, (THIS_FILE, "We had no window available previously and now one is avail"));
			//we can start right now :)
			strm->window_available = PJ_TRUE;
			// We are capturing frames, but previously we had no window
			// Just start capture now
			if(strm->capturing){
				PJ_LOG(4, (THIS_FILE, "We should start capturing right now"));
				webrtc_cap_stream_start(s);
			}
		}
		return PJ_SUCCESS;
	} else if (cap == PJMEDIA_VID_DEV_CAP_SET_VIEW) {
        pjmedia_vid_dev_switch_param* switch_prm = (pjmedia_vid_dev_switch_param*) pval;
        if (switch_prm) {
            pjsua_cap_dev_view* view_config = (pjsua_cap_dev_view*) switch_prm->data;
            if (view_config) {
                pj_uint32_t appId = view_config->app_pid;
                //strm->param.capture_view = appId;
                if (strm && strm->_videoCapture)
                {
                    ScreenCaptureConfig config;
                    config.screenIndex = 0;
                    config.screenId = 0;
                    config.appPid = appId;
                    config.winScreenCaptureUsingDD = view_config->win_screen_capture_using_dd;
                    config.macScreenCaptureDeferred = view_config->mac_screen_capture_deferred;
                    config.winCaptureUsingDDComplete = view_config->win_capture_using_dd_complete;
                    config.winAppCaptureGlobalHooks = view_config->win_app_capture_global_hooks;
                    config.macScreenCaptureDelta = view_config->mac_screen_capture_delta;
                    strm->_videoCapture->ConfigureCaptureParams(config);
                }
            }
            else {
                PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_SET_VIEW invalid view_config"));
            }
        }
        else {
            PJ_LOG(4, (THIS_FILE, "PJMEDIA_VID_DEV_CAP_SET_VIEW invalid switch_prm"));
        }
        return PJ_SUCCESS;
    }

	return PJMEDIA_EVID_INVCAP;
}

extern "C" {
pj_int32_t ref_frame_add_ref(void *ref_frame_obj)
{
    pj_assert(ref_frame_obj);
    pjmedia_ref_frame *ref_frame = static_cast<pjmedia_ref_frame *>(ref_frame_obj);
    pj_assert(ref_frame);

    pj_int32_t ret = static_cast<BASEVideoFrame *>(ref_frame->obj)->AddRef();
    PJ_LOG(6, (THIS_FILE, "REFCOUNT: ref_frame_add_ref: ts=%llu new count=%u, obj=0x%X", ref_frame->base.timestamp.u64, ret, ref_frame->obj));
    return ret;
}

// If this function returns 0, the ref_frame_obj pointer has become invalid and must not be accessed anymore!
pj_int32_t ref_frame_dec_ref(void *ref_frame_obj)
{
    pj_assert(ref_frame_obj);
    pjmedia_ref_frame *ref_frame = static_cast<pjmedia_ref_frame *>(ref_frame_obj);
    pj_assert(ref_frame);

    pj_int32_t ret = static_cast<BASEVideoFrame *>(ref_frame->obj)->Release();
    PJ_LOG(6, (THIS_FILE, "REFCOUNT: ref_frame_dec_ref: ts=%llu, new count=%u, obj=0x%X", ref_frame->base.timestamp.u64, ret, ref_frame->obj));

    // The pjmedia_ref_frame object's lifetime is the same as the underlying BASEVideoFrame:
    // When the BASEVideoFrame is destroyed, destroy the containing pjmedia_ref_frame object.
    if (ret == 0)
    {
        PJ_LOG(6, (THIS_FILE, "REFCOUNT: ref_frame_dec_ref: Deleting frame with ts=%llu, obj=0x%X", ref_frame->base.timestamp.u64, ref_frame->obj));
        delete ref_frame;
    }

    return ret;
}
}


class PjVideoCaptureDataCallback : public VideoCaptureDataCallback {
private:
    webrtc_cap_stream *stream;
    PJDynamicThreadDescRegistration _cap_thread;
    int _desired_width;
    int _desired_height;
    VideoFrame _captureFrame;
    I420VideoFrame _croppedFrame;
    bool _first_frame;
    uint32_t _changed_width;
    uint32_t _changed_height;
    bool _reset_in_process;
    bool _logged_cropping_error;

    // Convert the frame between types
    bool ConvertToVideoFrame(BASEVideoFrame &inFrame, VideoFrame &outFrame);
public:
	PjVideoCaptureDataCallback(webrtc_cap_stream*);
	virtual ~PjVideoCaptureDataCallback();
	void OnIncomingCapturedFrame(const int32_t id,
                                 BASEVideoFrame *videoFrame);
	void OnCaptureDelayChanged(const int32_t id,
	                           const int32_t delay);
};

PjVideoCaptureDataCallback::PjVideoCaptureDataCallback(webrtc_cap_stream* strm)
    : VideoCaptureDataCallback()
    , stream(strm)
    , _desired_width(strm->param.fmt.det.vid.size.w)
    , _desired_height(strm->param.fmt.det.vid.size.h)
    , _first_frame(true)
    , _changed_width(0)
    , _changed_height(0)
    , _reset_in_process(false)
    , _logged_cropping_error(false)
{
}

PjVideoCaptureDataCallback::~PjVideoCaptureDataCallback()
{
}

void PjVideoCaptureDataCallback::OnIncomingCapturedFrame(int id, BASEVideoFrame *videoFrame)
{
    int crop_x = 0, crop_y = 0;
    scoped_refptr<I420VideoFrame> croppedFrame = NULL;
    pjmedia_frame frame = {PJMEDIA_FRAME_TYPE_VIDEO};
    pjmedia_ref_frame *ref_frame = NULL;

    _cap_thread.Register("webrtc_cap");

    if(stream->_frame_scaler) {
        bool reset_stream = false;
        if(_first_frame) {
            int initial_width, initial_height;
            calculateDesiredWH(videoFrame->width(), videoFrame->height(), initial_width, initial_height);
            if ((initial_width != _desired_width) || (initial_height != _desired_height))
            {
                reset_stream = true;
            }
            _changed_width = videoFrame->width();
            _changed_height = videoFrame->height();
            _first_frame = false;
        }
        if(_changed_width != videoFrame->width() || _changed_height != videoFrame->height()) {
            _changed_width = videoFrame->width();
            _changed_height = videoFrame->height();
            reset_stream = true;
        }
        if (reset_stream)
        {
            webrtc_refresh_screen_device(stream, _changed_width, _changed_height, _desired_width, _desired_height);
            pjmedia_event event;
            event.data.fmt_changed.new_fmt.det.vid.size.w = _desired_width;
            event.data.fmt_changed.new_fmt.det.vid.size.h = _desired_height;
            event.type = PJMEDIA_EVENT_ORIENT_CHANGED;
            pjmedia_event_publish(NULL, stream, &event, PJMEDIA_EVENT_PUBLISH_POST_EVENT);
            // no point in delivering any frames as we will be torn down
            _reset_in_process = true;
        }
        else
        {
            stream->_frame_scaler->ResizeFrameIfNeeded(videoFrame, _desired_width, _desired_height);
        }
    }
    else
    {
        if(videoFrame->width() >= DEFAULT_SCREEN_SHARE_WIDTH && videoFrame->height() >= DEFAULT_SCREEN_SHARE_HEIGHT)
        {
            FrameScaler scaler;
            scaler.ResizeFrameIfNeeded(videoFrame, _desired_width, _desired_height);
        }
        else if (stream->_videoCapture->WebRTCFrameCroppingEnabled())
        {
            if (!_logged_cropping_error && (videoFrame->width() != _desired_width || videoFrame->height() != _desired_height))
            {
                PJ_LOG(2, (THIS_FILE, "OnIncomingCapturedFrame: Encountered unexpected case where frame needs to be cropped from %dx%d to %dx%d. ",
                            " Cropping will not be done.", videoFrame->width(), videoFrame->height(), _desired_width, _desired_height));
                _logged_cropping_error = true;
            }
        }
        else
        {
            crop_x = (videoFrame->width() - _desired_width) / 2;
            crop_y = (videoFrame->height() - _desired_height) / 2;
        }
    }

    if (_reset_in_process)
    {
        PJ_LOG(2, (THIS_FILE, "Capture reset is in progress, do nothing"));
        return;
    }

    if (false // (!videoFrame->is_ref_counted)
                 && (!ConvertToVideoFrame(*videoFrame, _captureFrame)))
    {
        PJ_LOG(6, (THIS_FILE, "OnIncomingCapturedFrame: ConvertToVideoFrame failed"));
        return;
    }

    if(!stream->_videoCapture->WebRTCFrameCroppingEnabled() && (crop_x || crop_y) && !videoFrame->is_RGB_Frame())
    {
        int half_width = (_desired_width + 1) / 2;

        if (true) // videoFrame->is_ref_counted)
        {
            croppedFrame = new RefCountImpl<I420VideoFrame>;
            croppedFrame->CreateEmptyFrame(_desired_width, _desired_height,
                    _desired_width, half_width, half_width);

            if (!ConvertToVideoFrame(*videoFrame, _captureFrame))
                return;

            const int conversionResult = ConvertToI420(webrtc::RawVideoTypeToCommonVideoVideoType(videoFrame->get_frame_type()),
                _captureFrame.Buffer(),
                crop_x, crop_y,
                videoFrame->width(),
                videoFrame->height(),
                _captureFrame.Length(),
                kRotateNone,
                croppedFrame);
            if (conversionResult < 0)
            {
                PJ_LOG(4, (THIS_FILE, "Failed to crop I420 frame"));
                return;
            }

        }
        else
        {
            _croppedFrame.CreateEmptyFrame(_desired_width, _desired_height,
                    _desired_width, half_width, half_width);

            const int conversionResult = ConvertToI420(webrtc::RawVideoTypeToCommonVideoVideoType(videoFrame->get_frame_type()),
                _captureFrame.Buffer(),
                crop_x, crop_y,
                videoFrame->width(),
                videoFrame->height(),
                _captureFrame.Length(),
                kRotateNone,
                &_croppedFrame);
            if (conversionResult < 0)
            {
                PJ_LOG(4, (THIS_FILE, "Failed to crop I420 frame"));
                return;
            }

            int length = ExtractBuffer(_croppedFrame, _captureFrame.Size(), _captureFrame.Buffer());
            if (length < 0) {
                PJ_LOG(4, (THIS_FILE, "Failed to extract frame buffer after cropping."));
                return;
            }
            _captureFrame.SetLength(length);

            frame.size = length;
            frame.buf = _captureFrame.Buffer();
        }
    }
   /* else if (!videoFrame->is_ref_counted)
    {
        frame.size = _captureFrame.Length();
        frame.buf = _captureFrame.Buffer();
    }*/

    if (true) //videoFrame->is_ref_counted)
    {
        ref_frame = new pjmedia_ref_frame;
        pj_assert(ref_frame);
        pj_bzero(ref_frame, sizeof(pjmedia_ref_frame));

        ref_frame->base.type = PJMEDIA_FRAME_TYPE_VIDEO;
        ref_frame->base.size = 0;
        ref_frame->base.buf = NULL;
        ref_frame->base.bit_info = 0;
        // ref_frame->base.is_ref_counted = PJ_TRUE;

        ref_frame->base.timestamp.u64 = videoFrame->render_time_ms();
        if (!stream->_videoCapture->WebRTCFrameCroppingEnabled() && (crop_x || crop_y))
        {
            ref_frame->obj = croppedFrame;
        }
        else
        {
            ref_frame->obj = videoFrame;
        }
        // stream->param.vidframe_ref_count = PJ_TRUE;
    }
    /*else
    {
        frame.bit_info = 0;
        frame.timestamp.u64 = videoFrame->render_time_ms();
        frame.is_ref_counted = PJ_FALSE;
        stream->param.vidframe_ref_count = PJ_FALSE;
    }*/
    if (stream->vid_cb.capture_cb){
        if (true) //videoFrame->is_ref_counted)
        {
            (*stream->vid_cb.capture_cb)(&stream->base, stream->user_data, &ref_frame->base);
            if (!stream->_videoCapture->WebRTCFrameCroppingEnabled() && (crop_x || crop_y) && !videoFrame->is_RGB_Frame())
            {
                PJ_LOG(6, (THIS_FILE, "REFCOUNT: Capture thread: croppedFrame about to be released as it falls out of scope: "
                            "ts=%llu, obj=0x%X", ref_frame->base.timestamp.u64, ref_frame->obj));
            }
        }
        else
        {
            (*stream->vid_cb.capture_cb)(&stream->base, stream->user_data, &frame);
        }
    }
}

bool PjVideoCaptureDataCallback::ConvertToVideoFrame(BASEVideoFrame &inFrame, VideoFrame &outFrame)
{
    int length;
    webrtc::I420VideoFrame *pI420VideoFrame = NULL;
    webrtc::RGBVideoFrame  *pRGBVideoFrame  = NULL;

    VideoType commonVideoType = webrtc::RawVideoTypeToCommonVideoVideoType(inFrame.get_frame_type());
    int requiredLength = CalcBufferSize((commonVideoType), inFrame.width(), inFrame.height());
    if (-1  == requiredLength)
    {
        PJ_LOG(4, (THIS_FILE, "Failed to CalcBufferSize, color Format is unrecognised"));
        return false;
    }
    outFrame.VerifyAndAllocate(requiredLength);
    if (!outFrame.Buffer())
    {
        PJ_LOG(4, (THIS_FILE, "Failed to allocate frame buffer."));
        return false;
    }

    if(inFrame.is_RGB_Frame())
    {
        pRGBVideoFrame = dynamic_cast<webrtc::RGBVideoFrame*>(&inFrame);
        if(!pRGBVideoFrame)
        {
            PJ_LOG(4, (THIS_FILE, "Failed to retrieve RGB Video frame buffer."));
            return false;
        }
        length = std::min(outFrame.Size(), pRGBVideoFrame->frame_size());
        pj_memcpy(outFrame.Buffer(),pRGBVideoFrame->buffer(),length);
    }
    else
    {
        pI420VideoFrame = dynamic_cast<webrtc::I420VideoFrame*>(&inFrame);
        if(!pI420VideoFrame)
        {
            PJ_LOG(4, (THIS_FILE, "Failed to retrieve I420 Video frame buffer."));
            return false;
        }
        length = ExtractBuffer(pI420VideoFrame, outFrame.Size(), outFrame.Buffer());
    }

    if (length < 0) {
        PJ_LOG(4, (THIS_FILE, "Failed to extract frame buffer."));
        return false;
    }
    outFrame.SetLength(length);

    return true;
}

void PjVideoCaptureDataCallback::OnCaptureDelayChanged(const int32_t id,
        const int32_t delay){
	// WARNING : if we want to log, we must be attached to thread !
	//PJ_LOG(4, (THIS_FILE, "Delay changed : %d", id));
}

static pj_status_t start_capturer(void *strm)
{
    pj_status_t status = PJ_SUCCESS;
    struct webrtc_cap_stream *stream = (struct webrtc_cap_stream *) strm;
    webrtc_cap_factory *cf = stream->sf;
    if(!stream->_videoCapture->CaptureStarted()) {
        PJ_LOG(4, (THIS_FILE, "Starting webrtc capture video : %dx%d @%d",
                   stream->_capability->width, stream->_capability->height,
                   stream->_capability->maxFPS));

        stream->_captureDataCb = new PjVideoCaptureDataCallback(stream);
        if(BJN::DevicePropertyManager::isScreenSharingDevice(stream->webrtc_id)) {
            if (PJMEDIA_FORMAT_RGB32 == stream->param.fmt.id) {
                PJ_LOG(4, (THIS_FILE, "Is a screen sharing device : %dx%d @%d Format Id %d",
                       stream->_capability->width, stream->_capability->height,
                       stream->_capability->maxFPS, stream->param.fmt.id));

                stream->_videoCapture->setCaptureFormatRGB(true);
            }
        }
        else {
            // Only do frame cropping for non-screen sharing devices
            bool WebRTCFrameCroppingEnabled = cf->sipManager->GetMeetingFeatures().mWebRTCFrameCropping;
            stream->_videoCapture->EnableFrameCropping(WebRTCFrameCroppingEnabled);

            PJ_LOG(4, (THIS_FILE, "Setting Desired Size to %ux%u. WebRTCFrameCropping is %s.", stream->param.fmt.det.vid.size.w,
                stream->param.fmt.det.vid.size.h, WebRTCFrameCroppingEnabled ? "enabled" : "disabled"));
            stream->_videoCapture->SetDesiredSize(stream->param.fmt.det.vid.size.w, stream->param.fmt.det.vid.size.h);
        }
        /*stream->_videoCapture->EnableVideoFrameRefCounting(
                cf->sipManager->GetMeetingFeatures().mVideoFrameRefCount);*/
        stream->_videoCapture->RegisterCaptureDataCallback(*stream->_captureDataCb);
        status = stream->_videoCapture->StartCapture(*stream->_capability);
    }
    return status;
}

/* API: Start stream. */
static pj_status_t webrtc_cap_stream_start(pjmedia_vid_dev_stream *strm) {
	struct webrtc_cap_stream *stream = (struct webrtc_cap_stream*) strm;
    pj_status_t status = PJ_SUCCESS;
    PJ_LOG(4, (THIS_FILE, "stream start"));
	if(stream->window_available) {
        status = start_capturer(stream);
	}

	stream->capturing = (status == PJ_SUCCESS ? PJ_TRUE : PJ_FALSE);
	return status;
}

static void stop_capturer(void *strm)
{
    struct webrtc_cap_stream *stream = (struct webrtc_cap_stream *) strm;
    if(stream->_videoCapture->CaptureStarted()){
        int32_t status = stream->_videoCapture->DeRegisterCaptureDataCallback();
        status = stream->_videoCapture->StopCapture();
        if (stream->_captureDataCb)
        {
            // base class has a protected destructor, we can only delete the derived class
            PjVideoCaptureDataCallback* dataCB
                = dynamic_cast<PjVideoCaptureDataCallback*>(stream->_captureDataCb);
            delete dataCB;
            stream->_captureDataCb = NULL;
        }
        PJ_LOG(4, (THIS_FILE, "Stop webrtc capture %d", status));
    }
}
/* API: Stop stream. */
static pj_status_t webrtc_cap_stream_stop(pjmedia_vid_dev_stream *strm) {
	struct webrtc_cap_stream *stream = (struct webrtc_cap_stream*) strm;
#ifdef FB_MACOSX
    run_func_on_main_thread(stream, stop_capturer);
#else
    stop_capturer(stream);
#endif
	stream->capturing = PJ_FALSE;
	return PJ_SUCCESS;
}

static void destroy_capturer(void *strm)
{
    struct webrtc_cap_stream *stream = (struct webrtc_cap_stream *) strm;
    int32_t remains = stream->_videoCapture->Release();
    PJ_LOG(4, (THIS_FILE, "Remaining : %d", remains));
}

/* API: Destroy stream. */
static pj_status_t webrtc_cap_stream_destroy(pjmedia_vid_dev_stream *strm) {
	struct webrtc_cap_stream *stream = (struct webrtc_cap_stream*) strm;

	PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);

	PJ_LOG(4, (THIS_FILE, "Destroy webrtc capture"));
	if(stream->capturing){
		webrtc_cap_stream_stop(strm);
	}

#ifdef FB_MACOSX
    run_func_on_main_thread(stream, destroy_capturer);
#else
    destroy_capturer(stream);
#endif
	pj_pool_release(stream->pool);

	return PJ_SUCCESS;
}

