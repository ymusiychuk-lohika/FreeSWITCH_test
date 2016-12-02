//
//  omxdecoder.h
//  Acid
//
//  Created by Deepak Kotur on 9/05/12.
//
//

#ifndef Acid_omxh264decoder_h
#define Acid_omxh264decoder_h

#include <pjmedia/vid_codec_util.h>
#include <pjmedia-codec/h264_packetizer.h>

extern "C" {
#include <H264SwDecApi.h>
}

#include "basevideocodec.h"

////////////////////////////////////////////////////////////////////////////////
// class omxdecoder

class omxdecoder : public basevideodecoder
{
public:
    omxdecoder(pjmedia_vid_codec *codec,
                const pj_str_t *logPath,
                pj_pool_t *pool);
                
    virtual ~omxdecoder();
    
    pj_status_t open(pjmedia_vid_codec_param *attr);
    
    pj_status_t bjn_codec_decode(pj_size_t pkt_count,
                                 pjmedia_frame packets[],
                                 unsigned out_size,
                                 pjmedia_frame *output);
    
    
private:
   
    
private:
    pj_mutex_t*              mutex;
    pjmedia_vid_codec*       vid_codec;
    pjmedia_rect_size        expectedSize;
    pjmedia_format           curFormat;

    
    pj_uint8_t*              dec_buf;
    unsigned			     dec_buf_size;
    pj_timestamp		     last_dec_keyframe_ts;

   
    pjmedia_h264_packetizer* pktz;
    
    const pjmedia_video_format_info* vfi;
    pjmedia_video_apply_fmt_param    vafp;
    
    H264SwDecApiVersion decVer;
    H264SwDecInst decInst;
    H264SwDecInfo decInfo;
    u32 disableOutputReordering;
    H264SwDecInput decInput;
    H264SwDecOutput decOutput;
    H264SwDecPicture decPicture;
    u32 cropDisplay;
    long picDecodeNumber;
    long picDisplayNumber;
    u32 picSize;
    long numErrors;
    long dec_count;
    long accm_time;
    bool format_change_event;
};

//
////////////////////////////////////////////////////////////////////////////////

#endif
