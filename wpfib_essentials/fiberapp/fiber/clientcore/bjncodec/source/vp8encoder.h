//
//  vp8encoder.h
//

#ifndef Acid_vp8encoder_h
#define Acid_vp8encoder_h

#include <pjmedia/vid_codec.h>
#include "basevideocodec.h"
#include "temporallayers.h"
#include "vp8streamparser.h"
extern "C" {
#include <vpx/vpx_codec.h>
#include <vpx/vpx_encoder.h>
}
#include "moving_average.h"
#include "fbr_cpu_monitor.h"
#include <string>
#include <fpslimiter.h>

#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)
//#define ENABLE_VIDEO_ENCODE_STATS 1

//
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// class vp8encoder

class vp8encoder : public basevideoencoder
{
public:
    vp8encoder(pjmedia_vid_codec* codec,
                const pj_str_t *logPath,
                pj_pool_t*         pool);
    virtual ~vp8encoder();

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
    bool init(void);
    bool close(void);
    bool reset(void);
    bool InitAndSetControlSettings(bool bPresentationChannel);
    uint32_t MaxIntraTarget(uint32_t optimalBuffersize);

    bool sliProcess();
    int encodedKeyFrame(uint64_t curTimestamp);
    int encodeFlagSetup(bool send_refresh, uint64_t curTimestamp);
    void initMovingAverageDuration(void);
    bool calcNewTimestampAndDuration(uint64_t curTimestamp);

#if ENABLE_OUTPUT_VIDEO_RECORDING
    void writeNumOfFrames();
    void writeIVFHeader(uint32_t frameLen, uint32_t curTimestamp);
#endif
    pj_status_t setSize(unsigned int width, unsigned int height);
    pj_status_t setBitrate(unsigned bitrate);
    pj_status_t setNumTemporalLayers(unsigned layers);
    pj_status_t setRoundTripTime(uint32_t rtt)
    {
        m_uRoundTripTime = rtt;
        return PJ_SUCCESS;
    };

    uint32_t getBitrate(void)const
    {
        return m_bitrate;
    }

    int getFrameType(void);

    void setupDynamicTable(void);

    void setupPresentationQuality(void);

    pj_status_t handleRpsi(uint64_t picID);
    pj_status_t handleSli(uint16_t start,uint16_t num,uint64_t picID);
    pj_status_t adjustDynamicSize();

    void resetAndResize(unsigned int width, unsigned int height);
    void postEncoderEvent(pjmedia_event_type eventType, pj_uint32_t format_id);
    pj_status_t adjustDynamicQP();
    void getDynamicQP(unsigned rateKbps, unsigned& minQP, unsigned& maxQP, float rateFactor);
private:
    vpx_codec_ctx_t*         m_encoder;
    vpx_codec_enc_cfg_t*     m_cfg;
    vpx_image_t*             m_raw;
    bool                     m_bInitialized;

    uint16_t                 m_pictureID;
    uint16_t                 m_pictureIDLastSentRef;
    uint16_t                 m_pictureIDLastAcknowledgedRef;
    uint16_t                 m_frameCounterSinceLastRef;
    int32_t                  m_sliPictureID;
    int32_t                  m_keyFramePictureId;
    uint64_t                 m_refFrameTS;//reference frame time stamp.
    bool                     m_haveReceivedAcknowledgement;
    bool                     m_lastAcknowledgedIsGolden;
    bool                     m_nextRefIsGolden;
    uint16_t                 m_pictureIDNoRefLast;//The picture ID of the frame which not ref the last frame, but the golden or altref,
                                                  //The frame encoded after receving and handling SLI.
    uint16_t                 m_noRPSIRefNum;//counter for the long term reference frame being sent without RPSI
    bool                     m_encBufIsKeyframe;
    bool                     m_bProtect;

    uint32_t                 m_uRoundTripTime;
    uint32_t                 m_bitrate;
    //in case the bitrate being set is lower than 64kbps, we will use 64kbps. But we need this value to decide the dynamic content size.
    uint32_t                 m_incomingBitrate;

    //Per frame information
    uint32_t                 m_encFrameLen;
    uint64_t                 m_timeStamp;
    uint64_t                 m_lastTimeStamp;
    uint64_t                 m_rtpTimeStamp;
    uint64_t                 m_duration;


    TemporalLayers           m_temporalLayers;
    //Indicate if the current frame is a long term reference frame or not
    //it also includes the frame that refer to long term reference frame because of SLI message
    bool                     m_bLongTermReferenceFrame;

    uint8_t                  m_minQuantizer;
    uint8_t                  m_maxQuantizer;
    uint8_t                  m_minRDCQuantizer;
    uint8_t                  m_maxRDCQuantizer;
    RtpFormatVp8*            m_packetizer;

    // Structures for calculating moving average duration
    MovingAverage<uint64_t>  m_durationBuffer;
    FpsLimiter               m_fpsLimiter;

    pjmedia_vid_codec*       m_vidCodec;
    pjmedia_format           m_curFormat;

    // Support for dynamic qp range
    bool                     m_bDynamicQP;
    dynamicQP*               m_pDynamicQPTable;        // The dynamic QP table to used
    uint32_t                 m_maxDynamicQPEntries;    // How many entries in the table
    bool                     m_postEncFmtChanged;

#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*                    m_encfp;
    bool                     m_bFirstFrame;
    uint32_t                 m_frameCounter;
#endif

#if ENABLE_VIDEO_ENCODE_STATS
    FILE*                    m_encStatsfp;
#endif

};

//
////////////////////////////////////////////////////////////////////////////////

#endif
