#include "skinnysipmanager.h"
#include <pjmedia-videodev/videodev_imp.h>
#include <pj/assert.h>
#include <pj/log.h>
#include <pj/os.h>

#include "bjn_render_dev.h"
#define THIS_FILE		"bjn_render_dev.cpp"

#include "bjnrenderer.h"
#include "trace.h"
#include "tick_util.h"
#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "module_common_types.h"
#include <pjmedia/ref_frame.h>

using namespace webrtc;

#define DEFAULT_CLOCK_RATE	90000
#define DEFAULT_WIDTH		1280
#define DEFAULT_HEIGHT		720
#define DEFAULT_FPS         30
#define IS_RGB32_FRAME(a) ((a) == PJMEDIA_FORMAT_RGB32)

#define MAX_REN_DEVICES 3
const char *bjn_ren_device_list[MAX_REN_DEVICES] = {
    "RenderLocalView",
    "RenderRemoteView",
    "RenderContentView",
};

typedef struct bjn_ren_fmt_info {
	pjmedia_format_id fmt_id;
} bjn_ren_fmt_info;

static bjn_ren_fmt_info bjn_ren_fmts[] = {
		{PJMEDIA_FORMAT_I420}, {PJMEDIA_FORMAT_RGB32}
};

/* bjn_ren_ device info */
struct bjn_ren_dev_info {
	pjmedia_vid_dev_info info;
};

/* bjn_ren_ factory */
struct bjn_ren_factory {
	pjmedia_vid_dev_factory base;
	pj_pool_t *pool;
	pj_pool_factory *pf;

    unsigned dev_count;
    struct bjn_ren_dev_info *dev_info;
    bjn_sky::skinnySipManager* sipManager;
};

/* Video stream. */
struct bjn_ren_stream {
	pjmedia_vid_dev_stream base; /**< Base stream	    */
	pjmedia_vid_dev_param param; /**< Settings	    */
	pj_pool_t *pool; /**< Memory pool.       */
    pj_mutex_t		       *mutex;

	pjmedia_vid_dev_cb vid_cb; /**< Stream callback.   */
	void *user_data; /**< Application data.  */

	struct bjn_ren_factory *sf;
	pj_bool_t is_running;
	pj_timestamp last_ts;

	void* _renderWindow;
    uint32_t external_renderer_width_;
    uint32_t external_renderer_height_;
    uint32_t external_renderer_fmt_id_;

    I420VideoFrame *dest_frame;
    BASEVideoFrame *src_frame;
};

/* Simple class to manage pjmutex locks! */
class PJMutexLock {
public:
    PJMutexLock(pj_mutex_t* pmutex) : m_pmutex(pmutex) {
        pj_mutex_lock(m_pmutex);
    }
    ~PJMutexLock() {
        pj_mutex_unlock(m_pmutex);
    }

private:
    pj_mutex_t* m_pmutex;
    PJMutexLock(const PJMutexLock&);
    PJMutexLock& operator=(const PJMutexLock&);
};

/* Prototypes */
static pj_status_t bjn_ren_factory_init(pjmedia_vid_dev_factory *f);
static pj_status_t bjn_ren_factory_destroy(pjmedia_vid_dev_factory *f);
static pj_status_t bjn_ren_factory_refresh(pjmedia_vid_dev_factory *f);
static unsigned bjn_ren_factory_get_dev_count(pjmedia_vid_dev_factory *f);
static pj_status_t bjn_ren_factory_get_dev_info(pjmedia_vid_dev_factory *f,
		unsigned index, pjmedia_vid_dev_info *info);
static pj_status_t bjn_ren_factory_default_param(pj_pool_t *pool,
		pjmedia_vid_dev_factory *f, unsigned index,
		pjmedia_vid_dev_param *param);
static pj_status_t bjn_ren_factory_create_stream(pjmedia_vid_dev_factory *f,
		pjmedia_vid_dev_param *param, const pjmedia_vid_dev_cb *cb,
		void *user_data, pjmedia_vid_dev_stream **p_vid_strm);

static pj_status_t bjn_ren_stream_get_param(pjmedia_vid_dev_stream *strm,
		pjmedia_vid_dev_param *param);
static pj_status_t bjn_ren_stream_get_cap(pjmedia_vid_dev_stream *strm,
		pjmedia_vid_dev_cap cap, void *value);
static pj_status_t bjn_ren_stream_set_cap(pjmedia_vid_dev_stream *strm,
		pjmedia_vid_dev_cap cap, const void *value);
static pj_status_t bjn_ren_stream_put_frame(pjmedia_vid_dev_stream *strm,
		const pjmedia_frame *frame);
static pj_status_t bjn_ren_stream_start(pjmedia_vid_dev_stream *strm);
static pj_status_t bjn_ren_stream_stop(pjmedia_vid_dev_stream *strm);
static pj_status_t bjn_ren_stream_destroy(pjmedia_vid_dev_stream *strm);

/* Operations */
static pjmedia_vid_dev_factory_op factory_op = { &bjn_ren_factory_init,
		&bjn_ren_factory_destroy, &bjn_ren_factory_get_dev_count,
		&bjn_ren_factory_get_dev_info, &bjn_ren_factory_default_param,
		&bjn_ren_factory_create_stream, &bjn_ren_factory_refresh };

static pjmedia_vid_dev_stream_op stream_op = { &bjn_ren_stream_get_param,
		&bjn_ren_stream_get_cap, &bjn_ren_stream_set_cap, &bjn_ren_stream_start,
		NULL, &bjn_ren_stream_put_frame, &bjn_ren_stream_stop,
		&bjn_ren_stream_destroy };
static RawVideoType getRawVideoTypeFromFormatId(unsigned int id);

/****************************************************************************
 * Factory operations
 */
/*
 * Init bjn_render video driver.
 */
pjmedia_vid_dev_factory* pjmedia_bjn_vid_render_factory(pj_pool_factory *pf,
            bjn_sky::skinnySipManager* sipManager) {
    struct bjn_ren_factory *f;
    pj_pool_t *pool;

    pool = pj_pool_create(pf, "BJN video renderer", 1000, 1000, NULL);
    f = PJ_POOL_ZALLOC_T(pool, struct bjn_ren_factory);
    f->pf = pf;
    f->pool = pool;
    f->base.op = &factory_op;
    f->sipManager = sipManager;

	return &f->base;
}

/* API: init factory */
static pj_status_t bjn_ren_factory_init(pjmedia_vid_dev_factory *f) {
	struct bjn_ren_factory *sf = (struct bjn_ren_factory*) f;
	struct bjn_ren_dev_info *ddi;
	unsigned i, j;

	sf->dev_count = MAX_REN_DEVICES;
	sf->dev_info = (struct bjn_ren_dev_info*) pj_pool_calloc(sf->pool,
			sf->dev_count, sizeof(struct bjn_ren_dev_info));

	for (i = 0; i < sf->dev_count; i++) {
		ddi = &sf->dev_info[i];
        pj_bzero(ddi, sizeof(*ddi));
        strncpy(ddi->info.name, bjn_ren_device_list[i], sizeof(ddi->info.name));
    	ddi->info.name[sizeof(ddi->info.name) - 1] = '\0';
        ddi->info.fmt_cnt = PJ_ARRAY_SIZE(bjn_ren_fmts);
        strncpy(ddi->info.driver, "BJN", sizeof(ddi->info.driver));
		ddi->info.driver[sizeof(ddi->info.driver) - 1] = '\0';
		ddi->info.dir = PJMEDIA_DIR_RENDER;
		ddi->info.has_callback = PJ_FALSE;
		ddi->info.caps = PJMEDIA_VID_DEV_CAP_FORMAT;
		ddi->info.caps |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;
		ddi->info.caps |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS;

		for (j = 0; j < ddi->info.fmt_cnt; j++) {
			pjmedia_format *fmt = &ddi->info.fmt[j];
			pjmedia_format_init_video(fmt, bjn_ren_fmts[j].fmt_id, DEFAULT_WIDTH,
					DEFAULT_HEIGHT, DEFAULT_FPS, 1);
		}
	}

	PJ_LOG(4, (THIS_FILE, "BJN renderer initialized, fmt_count = %d",ddi->info.fmt_cnt));

	return PJ_SUCCESS;
}

/* API: destroy factory */
static pj_status_t bjn_ren_factory_destroy(pjmedia_vid_dev_factory *f) {
	struct bjn_ren_factory *sf = (struct bjn_ren_factory*) f;
	pj_pool_t *pool = sf->pool;

	sf->pool = NULL;
	pj_pool_release(pool);

	return PJ_SUCCESS;
}

/* API: refresh the list of devices */
static pj_status_t bjn_ren_factory_refresh(pjmedia_vid_dev_factory *f) {
	PJ_UNUSED_ARG(f);
	return PJ_SUCCESS;
}

/* API: get number of devices */
static unsigned bjn_ren_factory_get_dev_count(pjmedia_vid_dev_factory *f) {
	struct bjn_ren_factory *sf = (struct bjn_ren_factory*) f;
	return sf->dev_count;
}

/* API: get device info */
static pj_status_t bjn_ren_factory_get_dev_info(pjmedia_vid_dev_factory *f,
		unsigned index, pjmedia_vid_dev_info *info) {
	struct bjn_ren_factory *sf = (struct bjn_ren_factory*) f;

	PJ_ASSERT_RETURN(index < sf->dev_count, PJMEDIA_EVID_INVDEV);

	pj_memcpy(info, &sf->dev_info[index].info, sizeof(*info));

	return PJ_SUCCESS;
}

/* API: create default device parameter */
static pj_status_t bjn_ren_factory_default_param(pj_pool_t *pool,
		pjmedia_vid_dev_factory *f, unsigned index,
		pjmedia_vid_dev_param *param) {
	struct bjn_ren_factory *sf = (struct bjn_ren_factory*) f;
	struct bjn_ren_dev_info *di = &sf->dev_info[index];

	PJ_ASSERT_RETURN(index < sf->dev_count, PJMEDIA_EVID_INVDEV);
    PJ_LOG(4, (THIS_FILE, "In function %s", __FUNCTION__));

	PJ_UNUSED_ARG(pool);

	pj_bzero(param, sizeof(*param));
	param->dir = PJMEDIA_DIR_RENDER;
	param->rend_id = index;
	param->cap_id = PJMEDIA_VID_INVALID_DEV;

	/* Set the device capabilities here */
	param->flags = PJMEDIA_VID_DEV_CAP_FORMAT | PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;
	param->fmt.type = PJMEDIA_TYPE_VIDEO;
	param->clock_rate = DEFAULT_CLOCK_RATE;
	pj_memcpy(&param->fmt, &di->info.fmt[0], sizeof(param->fmt));

	return PJ_SUCCESS;
}

static pj_status_t destroy_stream(struct bjn_ren_stream *strm){
	return PJ_SUCCESS;
}


/* API: Put frame from stream */
static pj_status_t bjn_ren_stream_put_frame(pjmedia_vid_dev_stream *strm,
		const pjmedia_frame *frame) {
	struct bjn_ren_stream *stream = (struct bjn_ren_stream*) strm;

    if (stream == NULL)
        return PJ_EINVAL;
    pjmedia_ref_frame *ref_frame = NULL;
    
    // bool isVideoFrameRefCountEnabled = stream->sf->sipManager->GetMeetingFeatures().mVideoFrameRefCount;

    // PJ_LOG(6, (THIS_FILE, "bjn_ren_stream_put_frame: Sipmanager Refcount = %s", isVideoFrameRefCountEnabled ? "ENABLED" : "DISABLED"));
    
    if (true) // frame->is_ref_counted == PJ_TRUE)
    {
        ref_frame = (pjmedia_ref_frame *) frame;
        if (ref_frame == NULL)
            return PJ_EINVAL;
    }
    stream->last_ts.u64 = frame->timestamp.u64;
    pj_mutex_lock(stream->mutex);
    if (!stream->is_running) {
        pj_mutex_unlock(stream->mutex);
        return PJ_EINVALIDOP;
    }

    if (false) // frame->is_ref_counted == PJ_FALSE)
    {
        if (frame->size == 0 || frame->buf == NULL) {
            pj_mutex_unlock(stream->mutex);
            return PJ_SUCCESS;
        }
    }
    if(stream->_renderWindow == NULL) {
        // We have nothing to show things in yet
        pj_mutex_unlock(stream->mutex);
        return PJ_SUCCESS;
    }

    pjmedia_video_format_detail *vfd;
    vfd = pjmedia_format_get_video_format_detail(&stream->param.fmt, PJ_TRUE);

    // w * h * 1.5 = frame size normally because I420
    unsigned width = vfd->size.w;
    unsigned height = vfd->size.h;

    long long nowMs = TickTime::MillisecondTimestamp();
    BJNExternalRenderer *external_renderer_ = (BJNExternalRenderer *) stream->_renderWindow;
    if (stream->external_renderer_width_ != width ||
        stream->external_renderer_height_ != height ||
        stream->external_renderer_fmt_id_ != stream->param.fmt.id) {
        stream->external_renderer_width_ = width;
        stream->external_renderer_height_ = height;
        stream->external_renderer_fmt_id_ = stream->param.fmt.id;
        external_renderer_->FrameSizeChange(stream->external_renderer_width_,
                                            stream->external_renderer_height_,
                                            stream->param.rend_id,
                                            getRawVideoTypeFromFormatId(stream->param.fmt.id));
    }

    int size_y = width * height;
    int half_width = (width + 1) / 2;
    int half_height = (height + 1) / 2;

    if (true //(frame->is_ref_counted == PJ_TRUE) 
		&& (ref_frame->obj == NULL))
    { 
        PJ_LOG(6, (THIS_FILE, "REFCOUNT: bjn_ren_stream_put_frame: Rendering frame with ts=%llu obj=0x%X for %s",
           ref_frame ? ref_frame->base.timestamp.u64 : 0, ref_frame ? ref_frame->obj : 0,
           stream->param.rend_id == 0 ? "self view" : "other view"));
    }
    /*  SKY-3139: The condition ((frame->is_ref_counted == PJ_TRUE) && (ref_frame->obj == NULL)) 
     *  is here until ref counting is added to the decode pipeline. Once added, it will be else case. 
     *  Right now, ref_frame->obj will be NULL when the frame came thru the decode branch (vs. self-view)
     */
    if ( false //(frame->is_ref_counted == PJ_FALSE)
		||  (
				true // (frame->is_ref_counted == PJ_TRUE) 
					&& (ref_frame->obj == NULL)))
    {
        int size_uv = half_width * half_height;

        if(IS_RGB32_FRAME(stream->param.fmt.id))
        {
            //Check if the stream had a I420 Frame before, if yes then release it and
            //create a new RGB Frame
            if (stream->src_frame)
            {
                if(!stream->src_frame->is_RGB_Frame())
                {
                    I420VideoFrame *pI420Frame = dynamic_cast<webrtc::I420VideoFrame*> (stream->src_frame);
                    if(!pI420Frame)
                        delete pI420Frame;
                    stream->src_frame = NULL;
                }
            }

            //Create RGB frame if not created before
            if (NULL == stream->src_frame)
                stream->src_frame = new webrtc::RGBVideoFrame();

            RGBVideoFrame* prgb_frame = dynamic_cast<webrtc::RGBVideoFrame*> (stream->src_frame);
            if(!prgb_frame)
                return PJ_EINVALIDOP;

            prgb_frame->CreateEmptyFrame(width,height,kVideoARGB);
            prgb_frame->CreateFrame((uint8_t *)frame->buf,width,height,kVideoARGB);
        }
        else
        {
            if (stream->src_frame)
            {
                //Check if the stream had a RGB Frame before, if yes then release it and
                //create a new I420 Frame
                if(stream->src_frame->is_RGB_Frame())
                {
                    webrtc::RGBVideoFrame *pRGBVideoFrame = dynamic_cast<webrtc::RGBVideoFrame*>(stream->src_frame);
                    delete pRGBVideoFrame;
                    stream->src_frame = NULL;
                }
            }

            if(!stream->src_frame)
            {
                stream->src_frame = new webrtc::I420VideoFrame();
            }

            webrtc::I420VideoFrame *pI420VideoFrame = dynamic_cast<webrtc::I420VideoFrame*>(stream->src_frame);
            pI420VideoFrame->CreateFrame(size_y, (const uint8_t *)frame->buf,
                    size_uv, (const uint8_t *)frame->buf + size_y,
                    size_uv, (const uint8_t *)frame->buf + size_y + size_uv,
                    width, height, width,
                    half_width, half_width);
        }
    }
    else if (ref_frame->obj != NULL) /* && frame->is_ref_counted == PJ_TRUE */ 
    {
        stream->src_frame = (I420VideoFrame *)ref_frame->obj;
    }

    if(stream->param.rend_id == 0)
    {
        //Mirroring is supported for I420 Frame only
        if(!IS_RGB32_FRAME(stream->param.fmt.id))
        {
            if(!stream->dest_frame)
            {
                stream->dest_frame = new webrtc::I420VideoFrame();
            }
            stream->dest_frame->CreateEmptyFrame(width, height,
                                                width, half_width, half_width);
            MirrorI420LeftRight((const webrtc::I420VideoFrame*)stream->src_frame, stream->dest_frame);

            webrtc::BASEVideoFrame* tempVideoFrame = dynamic_cast<webrtc::BASEVideoFrame *>((stream->dest_frame));
            external_renderer_->DeliverFrame(tempVideoFrame);

            // This is a relatively narrow place to do the release, but that assumes I know how to add another reference for the
            // encoder branch of the tee.  Right now, I don't know where to do that, so I will just hold the
            // ref longer and release it after both operations are done.

            if (true // (frame->is_ref_counted == PJ_TRUE) 
				&& (ref_frame->obj != NULL))
            {
                // NULL out the src_frame pointer so that when the renderer stream is torn down, we won't try to delete it.
                stream->src_frame = NULL;
            }
        }
        else
        {
            PJ_LOG(4, (THIS_FILE, "Mirroring of RGB frame is not supported"));
        }
    }
    else
    {
        webrtc::BASEVideoFrame* tempVideoFrame = dynamic_cast<webrtc::BASEVideoFrame *>((stream->src_frame));
        external_renderer_->DeliverFrame(tempVideoFrame);
        if (true // (frame->is_ref_counted == PJ_TRUE) 
			&& (ref_frame->obj != NULL))
        {
            // NULL out the src_frame pointer so that when the renderer stream is torn down, we won't try to delete it.
            stream->src_frame = NULL;
        }
    }

    pj_mutex_unlock(stream->mutex);
    return PJ_SUCCESS;
}


/* API: create stream */
static pj_status_t bjn_ren_factory_create_stream(pjmedia_vid_dev_factory *f,
		pjmedia_vid_dev_param *param, const pjmedia_vid_dev_cb *cb,
		void *user_data, pjmedia_vid_dev_stream **p_vid_strm) {
	struct bjn_ren_factory *sf = (struct bjn_ren_factory*) f;
	pj_pool_t *pool;
	struct bjn_ren_stream *strm;
	pj_status_t status;
    PJ_LOG(4, (THIS_FILE, "In function %s", __FUNCTION__));
	PJ_ASSERT_RETURN(param->dir == PJMEDIA_DIR_RENDER, PJ_EINVAL);

	/* Create and Initialize stream descriptor */
	pool = pj_pool_create(sf->pf, "bjn-render-dev", 1000, 1000, NULL);
	PJ_ASSERT_RETURN(pool != NULL, PJ_ENOMEM);

	strm = PJ_POOL_ZALLOC_T(pool, struct bjn_ren_stream);
	pj_memcpy(&strm->param, param, sizeof(*param));

    pjmedia_video_format_detail *vfd;
    vfd = pjmedia_format_get_video_format_detail(&strm->param.fmt, PJ_TRUE);
	PJ_LOG(4, (THIS_FILE, "Apply stream params %dx%d @ %x in %x", vfd->size.w,
			vfd->size.h,
			strm, pool));
	strm->pool = pool;
	strm->sf = sf;
	pj_memcpy(&strm->vid_cb, cb, sizeof(*cb));
	strm->user_data = user_data;
	// strm->param.vidframe_ref_count = PJ_FALSE;

    /* Create mutex. */
    status = pj_mutex_create_simple(strm->pool, "render_stream",
				    &strm->mutex);
	/* Done */
	strm->base.op = &stream_op;
	*p_vid_strm = &strm->base;

	return PJ_SUCCESS;
}

/* API: Get stream info. */
static pj_status_t bjn_ren_stream_get_param(pjmedia_vid_dev_stream *s,
		pjmedia_vid_dev_param *pi) {
	struct bjn_ren_stream *strm = (struct bjn_ren_stream*) s;

	PJ_ASSERT_RETURN(strm && pi, PJ_EINVAL);

	pj_memcpy(pi, &strm->param, sizeof(*pi));

	if (bjn_ren_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW,
			&pi->window) == PJ_SUCCESS) {
		pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW;
	}
	if (bjn_ren_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION,
			&pi->window_pos) == PJ_SUCCESS) {
		pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_POSITION;
	}
	if (bjn_ren_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE,
			&pi->disp_size) == PJ_SUCCESS) {
		pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE;
	}
	if (bjn_ren_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE,
			&pi->window_hide) == PJ_SUCCESS) {
		pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE;
	}
	if (bjn_ren_stream_get_cap(s, PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS,
			&pi->window_flags) == PJ_SUCCESS) {
		pi->flags |= PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW_FLAGS;
	}

	return PJ_SUCCESS;
}

/* API: get capability */
static pj_status_t bjn_ren_stream_get_cap(pjmedia_vid_dev_stream *s,
		pjmedia_vid_dev_cap cap, void *pval) {
	struct bjn_ren_stream *strm = (struct bjn_ren_stream*) s;

	PJ_ASSERT_RETURN(s && pval, PJ_EINVAL);

	if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW) {
		*(void**)pval = strm->_renderWindow;
		return PJ_SUCCESS;
	} else if (cap == PJMEDIA_VID_DEV_CAP_FORMAT) {
		// TODO
	} else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE) {
		// TODO
    } else if(cap == PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE) {
        *(pj_bool_t *)pval = PJ_FALSE;
    }

	return PJMEDIA_EVID_INVCAP;

}

/* API: set capability */
static pj_status_t bjn_ren_stream_set_cap(pjmedia_vid_dev_stream *s,
    pjmedia_vid_dev_cap cap, const void *pval) {
        struct bjn_ren_stream *strm = (struct bjn_ren_stream*) s;

        PJ_ASSERT_RETURN(s, PJ_EINVAL);
        PJ_LOG(4, (THIS_FILE, "In function %s with cap:%d val:%p for index %d", __FUNCTION__, cap, pval, strm->param.rend_id));
        if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_WINDOW) {
            // Lock is required because stream objects are updated
            PJMutexLock lock(strm->mutex);
            strm->external_renderer_width_ = 0;
            strm->external_renderer_height_ = 0;
            // Only do that if we have no window set currently
            if(!strm->_renderWindow || !pval) {
                if(!pval) {
                    destroy_stream(strm);
                }
                strm->_renderWindow = (void *)pval;
                return PJ_SUCCESS;
            }
            return PJ_EEXISTS;
        } else if (cap == PJMEDIA_VID_DEV_CAP_FORMAT) {
            PJ_LOG(4, (THIS_FILE, "try to change format...."));
            pjmedia_format *fmt = (pjmedia_format *) pval;
            pjmedia_video_format_detail *vfd;
            vfd = pjmedia_format_get_video_format_detail(fmt, PJ_TRUE);
            pjmedia_format_init_video(&strm->param.fmt, fmt->id, vfd->size.w,
                vfd->size.h, vfd->fps.num, vfd->fps.denum);
            return PJ_SUCCESS;
        } else if (cap == PJMEDIA_VID_DEV_CAP_OUTPUT_RESIZE) {
            PJMutexLock lock(strm->mutex);
            pjmedia_rect_size *new_disp_size = (pjmedia_rect_size *)pval;
            if(new_disp_size){
                PJ_LOG(4, (THIS_FILE, "RESCALE %dx%d", new_disp_size->w, new_disp_size->h));
                strm->param.disp_size.w = new_disp_size->w;
                strm->param.disp_size.h = new_disp_size->h;
            }
            return PJ_SUCCESS;
        } else if(cap == PJMEDIA_VID_DEV_CAP_OUTPUT_HIDE) {
            return PJ_SUCCESS;
        }

        return PJMEDIA_EVID_INVCAP;
}

/* API: Start stream. */
static pj_status_t bjn_ren_stream_start(pjmedia_vid_dev_stream *strm) {
	struct bjn_ren_stream *stream = (struct bjn_ren_stream*) strm;

	PJ_LOG(4, (THIS_FILE, "Starting BJN rendering video stream with resolution %dx%d",
               stream->param.fmt.det.vid.size.w,stream->param.fmt.det.vid.size.h));

    // stream->param.vidframe_ref_count = stream->sf->sipManager->GetMeetingFeatures().mVideoFrameRefCount ? PJ_TRUE: PJ_FALSE;

    pj_mutex_lock(stream->mutex);
	stream->is_running = PJ_TRUE;
    pj_mutex_unlock(stream->mutex);

	return PJ_SUCCESS;
}

/* API: Stop stream. */
static pj_status_t bjn_ren_stream_stop(pjmedia_vid_dev_stream *strm) {
	struct bjn_ren_stream *stream = (struct bjn_ren_stream*) strm;

	PJ_LOG(4, (THIS_FILE, "Stop BJN renderer"));

    pj_mutex_lock(stream->mutex);
	stream->is_running = PJ_FALSE;
    pj_mutex_unlock(stream->mutex);
	return PJ_SUCCESS;
}

/* API: Destroy stream. */
static pj_status_t bjn_ren_stream_destroy(pjmedia_vid_dev_stream *strm) {
    struct bjn_ren_stream *stream = (struct bjn_ren_stream*) strm;

    PJ_ASSERT_RETURN(stream != NULL, PJ_EINVAL);
    if (stream->src_frame)
    {
        PJ_LOG(6, (THIS_FILE, "REFCOUNT: bjn_ren_stream_destroy: Deleting stream->src_frame=0x%x", stream->src_frame));
        if(!stream->src_frame->is_RGB_Frame())
        {
            I420VideoFrame *pI420Frame = dynamic_cast<webrtc::I420VideoFrame*> (stream->src_frame);
            delete pI420Frame;
            stream->src_frame = NULL;
        }
        else
        {
            RGBVideoFrame *pRGBFrame = dynamic_cast<webrtc::RGBVideoFrame*> (stream->src_frame);
            delete pRGBFrame;
            stream->src_frame = NULL;
        }
    }

    if(stream->dest_frame)
    {
        PJ_LOG(6, (THIS_FILE, "REFCOUNT: bjn_ren_stream_destroy: Deleting stream->dest_frame=0x%x", stream->dest_frame));
        if(!stream->dest_frame->is_RGB_Frame())
        {
            I420VideoFrame *pI420Frame = dynamic_cast<webrtc::I420VideoFrame*> (stream->dest_frame);
            delete pI420Frame;
            stream->dest_frame = NULL;
        }
        else
        {
            RGBVideoFrame *pRGBFrame = dynamic_cast<webrtc::RGBVideoFrame*> (stream->dest_frame);
            delete pRGBFrame;
            stream->dest_frame = NULL;
        }
    }

    destroy_stream(stream);

    pj_mutex_lock(stream->mutex);
    stream->is_running = PJ_FALSE;
    pj_mutex_unlock(stream->mutex);


    pj_mutex_destroy(stream->mutex);
    stream->mutex = NULL;
    pj_pool_release(stream->pool);

    return PJ_SUCCESS;
}

static RawVideoType getRawVideoTypeFromFormatId(unsigned int id)
{
	switch(id)
	{
		case PJMEDIA_FORMAT_I420: return kVideoI420;
		case PJMEDIA_FORMAT_RGB24: return kVideoRGB24;
		case PJMEDIA_FORMAT_RGB32: return kVideoARGB;
		default: break;
	}
	return kVideoI420;
}
