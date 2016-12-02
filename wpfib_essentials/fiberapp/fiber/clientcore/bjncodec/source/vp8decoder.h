//
//  vp8decoder.h
//  Acid
//

#ifndef Acid_vp8decoder_h
#define Acid_vp8decoder_h

#include <pjmedia/vid_codec.h>
#include <pjmedia/vid_codec_util.h>

extern "C" {
#include <vpx/vpx_codec.h>
#include <vpx/vpx_encoder.h>
}
#include "basevideocodec.h"
//#define interface ()

#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)
#define ENABLE_OUTPUT_YUV_VIDEO_RECORDING   (0)

////////////////////////////////////////////////////////////////////////////////
// class vp8decoder

class vp8decoder : public basevideodecoder
{
public:
    vp8decoder(pjmedia_vid_codec *codec,
                const pj_str_t *logPath,
                pj_pool_t *pool);
                
    virtual ~vp8decoder();
    
    pj_status_t open(pjmedia_vid_codec_param *attr);
    
    pj_status_t bjn_codec_decode(pj_size_t pkt_count,
                                 pjmedia_frame packets[],
                                 unsigned out_size,
                                 pjmedia_frame *output);

private:
    pjmedia_vid_codec*       m_vidCodec;
    vpx_codec_ctx_t          m_decoder;
    bool                     m_codecInitialized;
    pjmedia_format           m_curFormat;
    unsigned                 m_decFrameLen;
    int32_t                  m_curPicID;
    int32_t                  m_prevPicID;
    bool                     m_bCurFrameIncomplete;
    bool                     m_bReferenceFrame;//Check if a frame is a reference frame or not
    int                      m_tID;//temporal layer ID, -1 means number of temporal layer less than 2.
    int                      m_tl0PicIdx;
    int                      m_prevTl0PicIdx;
    bool                     m_bDisplay;
    bool                     m_isFirstDecodedFrame;

#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*                        m_encfp;
    bool                         m_bFirstFrame;
    uint32_t                     m_frameCounter;

    void writeNumOfFrames();
    void writeIVFHeader(uint32_t frameLen, uint32_t timeStamp);
#endif
#if ENABLE_OUTPUT_YUV_VIDEO_RECORDING
    FILE*                        m_YUVFilePtr;
#endif

    pj_status_t putPacketsIntoBuffer(pj_size_t pkt_count, pjmedia_frame packets[]);
    bool ContinuousPictureId() const;
    int calculateMissingFrame();
    void setFlagsFromReferenceFrameAndTemporalScalability(bool &bSLI, bool &bNonRefDisplay, bool &bDisplay, bool &bNeedKeyFrame);
};

//
////////////////////////////////////////////////////////////////////////////////

#endif
