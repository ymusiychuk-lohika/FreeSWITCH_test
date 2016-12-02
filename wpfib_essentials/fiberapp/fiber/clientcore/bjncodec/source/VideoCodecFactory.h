//
//  VideoCodecFactory.h
//  Acid
//
//  Created by Emmanuel Weber on 5/14/12.
//  Copyright (c) 2012 Bluejeans. All rights reserved.
//

#ifndef Acid_VideoCodecFactory_h
#define Acid_VideoCodecFactory_h

#include <iostream>
#include <map>
#include <pjmedia-codec/types.h>
#include <pjmedia/vid_codec.h>
#include <pjmedia-videodev/videodev.h>
#include "basevideocodec.h"
#if defined(ENABLE_H264_OSX_HWACCL)
#include "talk/base/macutils.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_RECV_WIDTH  "max-recv-width"
#define MAX_RECV_HEIGHT "max-recv-height"

#define ABS_SEND_TIME_HEADER_SIZE    8

/* BJN codecs factory */
struct bjn_factory {
    pjmedia_vid_codec_factory base;
    pjmedia_vid_codec_mgr*    mgr;
    pj_pool_factory*          pf;
    pj_pool_t*                pool;
    int                       max_enc_width;
    int                       max_enc_height;
    int                       max_enc_fps;
    int                       startup_enc_bps;
    int                       max_enc_bps;
    int                       min_qp;
    int                       max_qp;
    int                       min_qp_content;
    int                       max_qp_content;
    int                       min_qp_RDC;
    int                       max_qp_RDC;
    int                       contentHighResBitrate;
    int                       videoFreeRangeQPBitrate;
    int                       refresh_rate;
    int                       max_dec_width;
    int                       max_dec_height;
    int                       max_dec_fps;
    int                       max_dec_bps;
    bool                      use_bjnscreencodec;
    bool                      use_h264_hw_accl;
    bool                      is_h264_hw_codec_supported;
    pj_str_t                  logPath;
    char                      width[16];
    char                      height[16];
    dynamicEncParameters      dynamic_enc_params;
    bool                      enable_camera_restart;
    bool                      enable_dynamic_content_size;
    bool                      enable_resolution_hysteresis;
    bool                      use_vi_bwmgr2;
    bool                      enable_screen_share_static_mode;
    bool                      use_improved_temporal_scalability;
    bool                      reduce_temporal_scalability_cpu;
};

pjmedia_vid_codec_factory * pjmedia_codec_bjn_vid_init(
                                       pjmedia_vid_codec_mgr* mgr,
                                       pj_pool_factory*       pf,
                                       int                    max_enc_width,
                                       int                    max_enc_height,
                                       int                    max_enc_fps,
                                       int                    startup_enc_bps,
                                       int                    max_enc_bps,
                                       int                    min_qp,
                                       int                    max_qp,
                                       int                    min_qp_content,
                                       int                    max_qp_content,
                                       int                    min_qp_RDC,
                                       int                    max_qp_RDC,
                                       int                    contentHighResBitrate,
                                       int                    videoFreeRangeQPBitrate,
                                       int                    refresh_rate,
                                       int                    max_dec_widht,
                                       int                    max_dec_heigh,
                                       int                    max_dec_fps,
                                       int                    max_dec_bps,
                                       bool                   use_bjnscreencodec,
                                       bool                   use_h264_hw_accl,
                                       bool                   enable_camera_restart,
                                       bool                   enable_dynamic_content_size,
                                       bool                   use_vi_bwmgr2,
                                       bool                   enable_screen_share_static_mode,
                                       bool                   use_improved_temporal_scalability,
                                       bool                   reduce_temporal_scalability_cpu,
                                       const char *           logPath);

pj_status_t pjmedia_codec_bjn_vid_deinit(pjmedia_vid_codec_factory *factory);

pjmedia_vid_dev_factory* pjmedia_ios_factory(pj_pool_factory *pf);
    
#ifdef __cplusplus
}
#endif
        
#endif
