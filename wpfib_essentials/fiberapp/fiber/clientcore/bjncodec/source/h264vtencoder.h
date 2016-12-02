//
//  h264vtencoder.h
//  Acid
//
//

#ifndef Acid_h264vtencoder_h
#define Acid_h264vtencoder_h

#include <pjmedia/vid_codec_util.h>
#include "basevideocodec.h"
#include "fbr_cpu_monitor.h"

#if defined(ENABLE_H264_OSX_HWACCL)
#include <VideoToolbox/VideoToolbox.h>
#include <VideoToolbox/VTVideoEncoderList.h>
#include <CoreMedia/CMTime.h>
#include <time.h>
#include <vector>

#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)
// max allowed duration between frames(ms)
#define MAX_TS_DIFF     200

typedef struct
{
    uint32_t payloadSize;
    uint8_t* payloadPtr;
}nal_t;
//
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// class h264vtencoder

class h264vtencoder : public basevideoencoder
{
public:
    enum PacketizationMode
    {
        Mode0 = 0,
        Mode1 = 1,
        Mode2 = 2,
    };

    h264vtencoder(pjmedia_vid_codec* codec,
                const pj_str_t *logPath,
                pj_pool_t*         pool);
    virtual ~h264vtencoder();

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

    void handleOutput(
                CMSampleBufferRef              m_SampleBuffer,
                OSStatus                       compStatus,
                VTEncodeInfoFlags              infoFlags,
                uint64_t                       timestamp);
    static pj_bool_t isHWEncodingSupported(int width,int height,CMVideoCodecType codecType);
    void setProfile(unsigned profile);


private:
    pj_status_t init(void);
    pj_status_t close(void);
    pj_status_t reset(void);
    pj_status_t configureCompressionSession(void);
    pj_status_t setSize(unsigned int width, unsigned int height);
    pj_status_t setBitrate(unsigned bitrate);
    pj_status_t internalSetBitrate(unsigned bitrate);
    pj_status_t setPacketizationMode(PacketizationMode mode);
    uint32_t estimatePacketsInFrame();

    static void logCallbackX264(
                void * priv, int level, const char *fmt, va_list arg);

    int forceKeyFrame(void);

    pj_status_t fillNextNal(
                    unsigned       out_size,
                    pjmedia_frame* output,
                    pj_bool_t*     has_more);

    pj_status_t fillNalsInStapA(
                    unsigned       out_size,
                    pjmedia_frame* output,
                    pj_bool_t*     has_more);

    pj_status_t fillNalsInFUA(
                    unsigned       out_size,
                    pjmedia_frame* output,
                    pj_bool_t*     has_more);

    void setupDynamicTable(void);

    pj_status_t handleRpsi(uint64_t picID);
    pj_status_t handlePli();
    void postEncoderEvent(pjmedia_event_type eventType, pj_uint32_t format_id);

private:
    VTCompressionSessionRef     m_session;
    pjmedia_vid_codec*          m_vidCodec;
    int                         m_CurNalsNum;
    int                         m_CurNalsIndex;
    size_t                      m_RemPayloadLen;
    uint32_t                    m_bitrate;
    uint32_t                    m_slicemaxsize;
    uint64_t                    m_timestamp;
    uint64_t                    m_lastTimeStamp;
    uint8_t*                    m_CurFU;
    uint8_t                     m_FUIndicator;
    uint8_t                     m_FUHeader;
    bool                        m_IsFirstFrame;
    bool                        m_IsFirstOutputFrame;
    bool                        m_IsEncClosing;
    bool                        m_bInitialized;
    bool                        m_IsKeyFrame;
    bool                        m_InAFUA;
    PacketizationMode           m_iPacketizationMode;
    pj_event_t                  *encoder_event;     /* to signal encoder event to callback */
    pj_event_t                  *callback_event;     /* to signal callback event to encoder  */
    unsigned                    m_profile;
    std::vector <nal_t>         m_CurNals;
    bool                        m_postEncFmtChanged;
#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*                       m_encfp;
#endif


};

//
////////////////////////////////////////////////////////////////////////////////
#endif
#endif
