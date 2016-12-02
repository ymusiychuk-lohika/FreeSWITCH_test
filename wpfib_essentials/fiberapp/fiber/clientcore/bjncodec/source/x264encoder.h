//
//  h264encoder.h
//  Acid
//
//  Created by Emmanuel Weber on 8/20/12.
//
//

#ifndef Acid_x264encoder_h
#define Acid_x264encoder_h

#include <pjmedia/vid_codec_util.h>
#include "basevideocodec.h"
extern "C" {
#include <x264.h>
}

#include <time.h>

#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)
//
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// class h264encoder

class h264encoder : public basevideoencoder
{
public:
    enum PacketizationMode
    {
        Mode0 = 0,
        Mode1 = 1,
        Mode2 = 2,
    };

    h264encoder(pjmedia_vid_codec* codec,
                const pj_str_t *logPath,
                pj_pool_t*         pool);
    virtual ~h264encoder();

    pj_status_t open(pjmedia_vid_codec_param *attr);

    pj_status_t encodeBegin(
                const pjmedia_vid_encode_opt* opt,
                const pjmedia_frame*          input,
                unsigned                      out_size,
                pjmedia_frame*                output,
                pj_bool_t*                    has_more,
                unsigned*                     est_packets);

    pj_status_t encodeMore(
                unsigned                      out_size,
                pjmedia_frame*                output,
                pj_bool_t*                    has_more);

private:
    pj_status_t setSize(unsigned int width, unsigned int height);
    pj_status_t setBitrate(unsigned bitrate);
    pj_status_t internalSetBitrate(unsigned bitrate);
    pj_status_t setNumTemporalLayers(unsigned layers);
    pj_status_t setMaxNumReferenceFrames();
    pj_status_t setReferenceFrameSelection(bool set);
    pj_status_t setPacketizationMode(PacketizationMode mode);
    uint32_t estimatePacketsInFrame();

    static void logCallbackX264(
                void * priv, int level, const char *fmt, va_list arg);

    int getFrameType(void);

    pj_status_t fillNextNal(
                    unsigned       out_size,
                    pjmedia_frame* output,
                    pj_bool_t*     has_more);

    pj_status_t fillNalsInStapA(
                    unsigned       out_size,
                    pjmedia_frame* output,
                    pj_bool_t*     has_more);

    void setupDynamicTable(void);

    pj_status_t handleRpsi(uint64_t picID);
    pj_status_t handlePli();

private:
    x264_t*         m_codec;
    x264_param_t    m_context;
    x264_nal_t*     m_pCurNals;
    int             m_CurNalsNum;
    int             m_CurNalsIndex;
    x264_picture_t  inputFrame;

    PacketizationMode  m_iPacketizationMode;

#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*              m_encfp;
#endif
};

//
////////////////////////////////////////////////////////////////////////////////

#endif
