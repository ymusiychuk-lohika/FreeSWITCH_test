//
//  h264qsencoder.h
//
//
#pragma once

#ifndef h264qsencoder_h
#define h264qsencoder_h

#include <pjmedia/vid_codec_util.h>
#include <pjmedia-codec/h264_packetizer.h>
#include <moving_average.h>
#include <fpslimiter.h>

#include "basevideocodec.h"

#if defined(ENABLE_H264_WIN_HWACCL)
#include<Windows.h>
#include<mfxcommon.h>
#include<mfxvideo++.h>
#include <time.h>
#include <vector>

#include "fbr_cpu_monitor.h"
#include "h264qsutils.h"
#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)
//
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// class h264qsencoder

class h264qsencoder : public basevideoencoder
{
public:
    enum PacketizationMode
    {
        Mode0 = 0,
        Mode1 = 1,
        Mode2 = 2,
    };

    h264qsencoder(pjmedia_vid_codec* codec,
                const pj_str_t *logPath,
                pj_pool_t*         pool);
    virtual ~h264qsencoder();

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

    void setProfile(unsigned profile);

private:
    pj_status_t init(void);
    pj_status_t close(void);
    pj_status_t reset(void);
    pj_status_t configureEncodeSession(void);
    pj_status_t setSize(unsigned int width, unsigned int height);
    pj_status_t setBitrate(unsigned bitrate);
    pj_status_t setPacketizationMode(PacketizationMode mode);
    uint32_t    estimatePacketsInFrame();
    uint32_t    getDynamicMaxBitrate(unsigned int width, unsigned int height);

    int forceKeyFrame(void);
    void setupDynamicTable(void);
    void initMovingAverageDuration(void);
    bool calcNewTimestampAndDuration(uint64_t curTimestamp);
    pj_status_t handleRpsi(uint64_t picID);
    pj_status_t handlePli();
    pj_status_t setEncoderfps(uint32_t fps);

private:
    MFXVideoSession             m_session;
    MFXVideoENCODE*             m_encoder;
    mfxVersion                  m_sdkVersion;
    mfxFrameAllocResponse       m_mfxResponseVPPOutEnc;
    mfxVideoParam               m_encParams;
    mfxExtBuffer*               m_encExtBuffers[1];
    mfxExtCodingOption          m_extEncParams;
    pjmedia_vid_codec*          m_vidCodec;
    mfxFrameSurface1**          m_pVPPSurfacesVPPOutEnc;
    pj_pool_t*                  m_pool;
    mfxU16                      m_nSurfNumVPPOutEnc;
    mfxBitstream                m_bitstream;
    VideoMemAllocator           m_videoAllocator;
    VideoProcessor              m_vpp;
    //m_GetFreeSurfaceIndexFail tracks failure rate of GetFreeSurfaceIndex()
    FailureRate                 m_GetFreeSurfaceIndexFail;
    unsigned int                m_attemptToRecover;
    uint32_t                    m_MaxPayloadSize;
    uint32_t                    m_bitrate;
    uint32_t                    m_slicemaxsize;
    uint64_t                    m_timestamp;
    uint64_t                    m_lastTimeStamp;
    bool                        m_IsFirstOutputFrame;
    bool                        m_bInitialized;
    bool                        m_IsKeyFrame;
    pjmedia_h264_packetizer*    m_pktz;
    PacketizationMode           m_iPacketizationMode;
    unsigned                    m_profile;
    MovingAverage<uint64_t>     m_durationBuffer;
    FpsLimiter                  m_fpsLimiter;
    uint64_t                    m_duration;
    int32_t                     m_sourceFramerate;
#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*                       m_encfp;
#endif


};

//
////////////////////////////////////////////////////////////////////////////////
#endif
#endif
