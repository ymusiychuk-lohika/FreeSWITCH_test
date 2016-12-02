//
//  bscrdecoder.h
//  Fiber
//

#ifndef Fiber_bscrdecoder_h
#define Fiber_bscrdecoder_h

#include <pjmedia/vid_codec.h>
#include <pjmedia/vid_codec_util.h>

extern "C" {
#include <ArithmeticTypes.h>
#include <ScreenDecoder_Api.h>
}
#include "basevideocodec.h"
//#define interface ()

#define ENABLE_INPUT_FILE_DUMP      (0)
#define ENABLE_OUTPUT_FILE_DUMP     (0)

////////////////////////////////////////////////////////////////////////////////
// class bscrdecoder
// Input/Output struct for encoder
typedef struct
{
    tUint8      *mInBuffPtr;        /* pointer to memory to supply input RGB frame to encoder*/
    tUint32      mInDataSize;        /* Size of input data (in bytes) */

    tUint8      *mOutBuffPtr;       /* pointer to memory to return encoded output */
    tInt32      mOutDataSize;       /* size of encoded daat (in bytes) */
}TBjnDecInOutStruct;


////////////////////////////////////////////////////////////////////////////////
// class bscrdecoder

class bscrdecoder : public basevideodecoder
{
public:
    bscrdecoder(pjmedia_vid_codec *codec,
                const pj_str_t *logPath,
                pj_pool_t *pool);
                
    virtual ~bscrdecoder();
    
    pj_status_t open(pjmedia_vid_codec_param *attr);
    
    pj_status_t bjn_codec_decode(pj_size_t pkt_count,
                                 pjmedia_frame packets[],
                                 unsigned out_size,
                                 pjmedia_frame *output);

private:
    TBjnDecInOutStruct*      m_decinoutstruct;
    TBjnScrDecoder*          m_decoder;
    pjmedia_vid_codec*       m_vidCodec;
    bool                     m_isSessionInitialized;
    pjmedia_format           m_curFormat;
    unsigned                 m_decFrameLen;
    int32_t                  m_curPicID;
    int32_t                  m_prevPicID;
    //bool                     m_bCurFrameIncomplete;
    //bool                     m_bReferenceFrame;//Check if a frame is a reference frame or not
    bool                     m_bDisplay;
    bool                     m_isFirstDecodedFrame;
    uint32_t                 m_framesSinceLastKeyFrameRequest;
    uint32_t                 m_minKeyFrameInterval;
    //Session heade size
    uint32_t                 m_sessionHeaderSize;
    //Quad-tree mode
    bool                     m_quadTreeMode;

#if ENABLE_INPUT_FILE_DUMP
    FILE*                    m_encfp;
#endif
#if ENABLE_OUTPUT_FILE_DUMP
    FILE*                    m_rgbfp;
#endif

    pj_status_t putPacketsIntoBuffer(pj_size_t pkt_count, pjmedia_frame packets[]);
    bool isSessionHeaderFrame(pjmedia_frame* frame);
    unsigned int  getInputFrameSize();
};

//
////////////////////////////////////////////////////////////////////////////////

#endif
