//
//  basevideocodec.h
//  Acid
//
//  Created by Emmanuel Weber on 8/27/12.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef Acid_basevideocodec_h
#define Acid_basevideocodec_h

extern "C" {
#include <stdint.h>
}

#include <pj/log.h>
#include <limits.h>
#include <algorithm>

#define DEFAULT_FRAMERATE   30
#define DEFAULT_BITRATE     512000
#define DEFAULT_MAX_KBPS    2048
#define DEFAULT_MAX_WIDTH   1280
#define DEFAULT_MAX_HEIGHT  720

/*ScreenCodec default parameters*/
#define DEFAULT_BITS_PERCOLORCOMP   6
#define DEFAULT_JPEG_QUALITY        50
#define DEFAULT_ENCODING_SCHEMES    6
#define DEFAULT_TILE_SIZE           128
#define DEFAULT_QUADTREE_TILE_SIZE  256
#define DEFAULT_MAX_DEPTH           3

#define USE_RGB32_MODE_SCREENCODEC              1
#define BSCR_SESSION_HEADER_SIZE                19
#define BSCR_SESSION_QUADTREE_HEADER_SIZE       24

#define BSCR_QUAD_TREE_MODE "quad-tree-mode"

#if (defined(__APPLE__) && !(TARGET_OS_IPHONE)) || defined (WIN32)
#define ENABLE_SCREENCODEC
#endif

#if (defined(__APPLE__) && !(TARGET_OS_IPHONE))
#include <AvailabilityMacros.h>
#if defined(MAC_OS_X_VERSION_10_9) && MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_9
#define ENABLE_H264_OSX_HWACCL
#define H264_HW_CODEC_MIN_BITRATE_UPSCALE    336
#define H264_HW_CODEC_MIN_BITRATE_DOWNSCALE  250
#define USE_H264_HIGH_PROFILE
#endif
#endif

#if defined(WIN32)
#define ENABLE_H264_WIN_HWACCL
#if defined(ENABLE_H264_WIN_HWACCL)
#define NOMINMAX
#define USE_H264_HIGH_PROFILE
#include<Windows.h>
#include <Winsock2.h>
#endif
#endif

///////////////////////////////////////////////////////////////////////////////
// enum Channel type

enum ChannelType
{
    RELIABLE        = 0,//tcp
    UNRELIABLE      = 1,//udp
};

///////////////////////////////////////////////////////////////////////////////
// struct dynamicQP

struct dynamicQP
{
    uint32_t rate;
    uint32_t minQP;
    uint32_t maxQP;
};

///////////////////////////////////////////////////////////////////////////////
// struct dynamicCap

struct dynamicCap
{
    uint32_t rate;
    uint32_t width;
    uint32_t height;
};

///////////////////////////////////////////////////////////////////////////////
// structure dynamicEncoderParameters

enum ProfileType{
    H264_BASELINE         = 0,
    H264_HIGH             = 1,
};
struct dynamicEncParameters
{
    int bitsPerColorComp;
    int jpegQuality;
    int encodingSchemes;
};

////////////////////////////////////////////////////////////////////////////////
// class basevideoencoder

class basevideoencoder
{
public:
    basevideoencoder()
    : m_fastUpdateRequested(false)
    , m_IFrameInterval(INT_MAX)
    , m_PFramesSinceLastIFrame(0)
    , m_MaxWidth(1280)
    , m_MaxHeight(720)
    , m_CPUwidth(1280)
    , m_CPUheight(720)
    , m_MaxFrameRate(DEFAULT_FRAMERATE)
    , m_MaxBitRateKbps(DEFAULT_MAX_KBPS)
    , m_Rotated(false)
    , m_pResizePlane(0)
    , m_bDynamicSize(true)
    , m_pDynamicCapTable(0)
    , m_pIncreaseDynamicCapTable(0)
    , m_MaxDynamicCapEntries(0)
    , m_MaxAllowedEntry(0)
    , m_EnableResolutionHysteresis(false)
    , m_HasFrameSizeEverDecreased(false)
    , m_bReferenceFrameSelection(false)
    , m_vfi(0)
    , m_ChannelType(UNRELIABLE)
    , m_mediaIdx(VIDEO_MED_IDX)
    , m_width(0)
    , m_height(0)
    , m_framerate(DEFAULT_FRAMERATE)
    , m_MaxPayloadSize(PJMEDIA_MAX_VID_PAYLOAD_SIZE)
    , m_DynamicMaxAllowedEntry(0)
    , m_isMtgFtrCamRestartEnabled(false)
    , m_isHWEncodingSupported(false)
    , m_hasBegunEncoding(false)
    , m_EnableScreenShareStaticMode(false)
    , m_UseImprovedTemporalScalability(false)
    , m_ReduceTemporalScalabilityCPU(false)
    {
        pj_bzero(&m_vafp, sizeof(m_vafp));
        pj_bzero(&m_vafpo, sizeof(m_vafpo));
    }

    virtual ~basevideoencoder()
    {
    }

    virtual pj_status_t open(pjmedia_vid_codec_param *attr) = 0;

    virtual pj_status_t encodeBegin(
                            const pjmedia_vid_encode_opt* opt,
                            const pjmedia_frame*          input,
                            unsigned                      out_size,
                            pjmedia_frame*                output,
                            pj_bool_t*                    has_more,
                            unsigned*                     est_packets) = 0;

    virtual pj_status_t encodeMore(
                           unsigned                      out_size,
                           pjmedia_frame*                output,
                           pj_bool_t*                    has_more) = 0;

    virtual void setMaxSize(unsigned int width, unsigned int height, unsigned int fps, unsigned int CpuWidth, unsigned int CpuHeight) {
        m_MaxWidth     = width;
        m_MaxHeight    = height;
        m_MaxFrameRate = fps;
        m_CPUwidth = CpuWidth;
        m_CPUheight = CpuHeight;
    }
    virtual void setProfile(unsigned profile){

    }
    virtual void setIsCamRestartMtgFtrEnabled(bool enable)
    {
        m_isMtgFtrCamRestartEnabled = enable;
    }

    bool isCamRestartMtgFtrEnabled() const
    {
        return m_isMtgFtrCamRestartEnabled;
    }

    virtual void setIsHWEncodingSupported(bool enable)
    {
        m_isHWEncodingSupported = enable;
    }

    bool isHWEncodingSupported() const
    {
        return m_isHWEncodingSupported;
    }

    uint32_t getWidth(void) const
    {
        return m_width;
    }

    uint32_t getHeight(void) const
    {
        return m_height;
    }

    uint32_t getFramerate(void)const
    {
        return m_framerate;
    };

    virtual pj_status_t setDynamicSize(bool set)
    {
        m_bDynamicSize = set;
        PJ_LOG(4, (m_LogName, "Encoder set dynamic size to: %s ", set? "TRUE" : "FALSE"));
        return PJ_SUCCESS;
    }

    void setResolutionHysteresis(bool useIt)
    {
        PJ_LOG(4, (m_LogName, "%sabling resolution hysteresis", useIt ? "En" : "Dis"));
        m_EnableResolutionHysteresis = useIt;
    }

    void setScreenShareStaticMode(bool useIt)
    {
        PJ_LOG(4, (m_LogName, "%sabling screen share static mode", useIt ? "En" : "Dis"));
        m_EnableScreenShareStaticMode = useIt;
    }

    void setImprovedTemporalScalability(bool useIt)
    {
        PJ_LOG(4, (m_LogName, "%sabling improved temporal scalability code", useIt ? "En" : "Dis"));
        m_UseImprovedTemporalScalability = useIt;
    }

    void setReduceTemporalScalabilityCPU(bool useIt)
    {
        PJ_LOG(4, (m_LogName, "%sabling reduced temporal scalability CPU feature", useIt ? "En" : "Dis"));
        m_ReduceTemporalScalabilityCPU = useIt;
    }

protected:
    virtual pj_status_t setSize(unsigned int width, unsigned int height)
    {
        return PJ_SUCCESS;
    };

    pj_status_t setNumTemporalLayers(unsigned layers)
    {
        return PJ_SUCCESS;
    };

//-----------------------------------------------------------------------------
// Name: setReferenceFrameSelection()
// Desc: Enable/disable reference frame selection
//-----------------------------------------------------------------------------

    virtual pj_status_t setReferenceFrameSelection(bool set)
    {
        m_bReferenceFrameSelection = set;
        PJ_LOG(4, (m_LogName, "Encoder set reference frame selection to: %s ", set? "TRUE" : "FALSE"));
        return PJ_SUCCESS;
    };

    //----------------------------------------------------------------------------//
    // Name: allocateResizePlanes()
    // Desc: Allocate planes needed for resizing
    //----------------------------------------------------------------------------//

    virtual pj_status_t allocateResizePlanes(int width,int height)
    {
        pj_status_t status;

        if(m_pResizePlane)
        {
            delete[] m_pResizePlane;
            m_pResizePlane = 0;
        }

        if(m_vafp.size.w == width && m_vafp.size.h == height)
        {
            memcpy(&m_vafpo,&m_vafp,sizeof(m_vafpo));
        }
        else
        {
            PJ_LOG(3, (m_LogName, "Scaling from %dx%d to %dx%d",
                       m_vafp.size.w,m_vafp.size.h,width, height));

            pj_bzero(&m_vafpo, sizeof(m_vafpo));
            m_vafpo.size.w   = width;
            m_vafpo.size.h   = height;
            m_vafpo.buffer = NULL;
            status = (*m_vfi->apply_fmt)(m_vfi, &m_vafpo);
            if (status != PJ_SUCCESS)
                return status;

            pj_size_t planeSize = m_vafpo.plane_bytes[1]+m_vafpo.plane_bytes[0]+m_vafpo.plane_bytes[2];
            m_pResizePlane = new unsigned char[planeSize];
        }
        return PJ_SUCCESS;
    };

    //----------------------------------------------------------------------------//
    // Name: rotate()
    // Desc: Reconfigure size of encoder if there was a rotation
    //----------------------------------------------------------------------------//

    virtual pj_status_t rotate(bool rotated)
    {
        pj_status_t status = PJ_SUCCESS;
        if(m_Rotated != rotated)
        {
            m_Rotated = rotated;

            PJ_LOG(3, (m_LogName, "Rotation is %s",(const char*)(m_Rotated?"enabled":"disabled") ) );

            m_vafp.size.w    = rotated ? m_MaxHeight : m_MaxWidth;
            m_vafp.size.h    = rotated ? m_MaxWidth  : m_MaxHeight;

            status = (*m_vfi->apply_fmt)(m_vfi, &m_vafp);
            if(status == PJ_SUCCESS)
            {
                status = setSize(m_vafp.size.w,m_vafp.size.h);
            }
        }
        return status;
    };

    //-----------------------------------------------------------------------------------------------
    // Name: getDynamicSize()
    // Desc: Dynamic size calculation based on rate.
    //
    // rateFactor: Adjust the rates in the bandwidth table by this amount.  A value of 1 means no
    //             adjustment.  A value between 0 and 1 means to reduce the rates from the table.
    //             A value larger than 1 means to increase the rates from the table by that multiple.
    //             Examples:
    //             1) Given a table rate of 100 and rateFactor of 1.15 means to use 115 instead of
    //             100 for that table entry.
    //             2) Given a table rate of 250 and a rateFactor of 0.90 means to use 225 instead of
    //             250 for that table entry.
    //-----------------------------------------------------------------------------------------------

    virtual void getDynamicSize(
                        unsigned  rateKbps,
                        unsigned& frameWidth,
                        unsigned& frameHeight,
                        unsigned& frameRate,
                        float     rateFactor)
    {
        if (m_bDynamicSize)
        {
            unsigned proposedWidth, proposedHeight;
            unsigned currentWidth = getWidth();
            unsigned currentHeight = getHeight();

            getDynamicSize(m_pDynamicCapTable, rateKbps, proposedWidth, proposedHeight, frameRate, rateFactor);
            if (m_pIncreaseDynamicCapTable && m_HasFrameSizeEverDecreased
                && (proposedWidth*proposedHeight) > (currentWidth*currentHeight))
            {
                pj_time_val diff;
                pj_gettimeofday(&diff);
                PJ_TIME_VAL_SUB(diff, m_TimeOfLastResizeDown);
                if (diff.sec < SECONDS_BEFORE_RESIZE_UP)
                {
                    // Not enough time has elapsed since the last time we decreased the frame size, so keep it the same
                    PJ_LOG(4, (m_LogName, "An increase to %ux%u has been proposed, but the last resize down was %ld seconds"
                        " ago. Not enough time has elapsed. Holding frame size the same.", proposedWidth, proposedHeight, diff.sec));
                    proposedWidth = currentWidth;
                    proposedHeight = currentHeight;
                }
                else
                {
                    // It's been long enough to allow an upsize, but use the alternate bitrate table to determine the size
                    getDynamicSize(m_pIncreaseDynamicCapTable, rateKbps, proposedWidth, proposedHeight, frameRate, rateFactor);
                }
            }
            else if (m_pIncreaseDynamicCapTable && (proposedWidth*proposedHeight) < (currentWidth*currentHeight))
            {
                if (!m_HasFrameSizeEverDecreased && m_hasBegunEncoding)
                {
                    m_HasFrameSizeEverDecreased = true;
                    PJ_LOG(4, (m_LogName, "First frame size decrease."));
                }

                // We are decreasing the frame size, so mark the time so we don't jump back up too soon
                pj_gettimeofday(&m_TimeOfLastResizeDown);
                PJ_LOG(4, (m_LogName, "Reset the time of last downward resolution resize"));
            }

            //In case the aspect ratio from screen capture is different
            if(m_mediaIdx == CONTENT_MED_IDX)
            {
                unsigned tempWidth = m_vafp.size.w * proposedHeight / m_vafp.size.h;
                if( tempWidth != proposedWidth)
                {
                    proposedWidth = tempWidth;
                }
            }
            frameWidth = proposedWidth;
            frameHeight = proposedHeight;
        }
        else
        {
            // We do not change the size of the stream
            frameWidth  = getWidth();
            frameHeight = getHeight();
            frameRate   = getFramerate();
        }
    }

private:
    void getDynamicSize(
        const dynamicCap *capTable,
        unsigned  rateKbps,
        unsigned &frameWidth,
        unsigned &frameHeight,
        unsigned &frameRate,
        float     rateFactor)
    {
        if (capTable)
        {
            int i;
            int min = 1;
            m_DynamicMaxAllowedEntry = std::max<int>(min, m_DynamicMaxAllowedEntry);
            for(i=min; i < m_DynamicMaxAllowedEntry; i++)
            {
                if(static_cast<uint32_t>(capTable[i].rate * rateFactor) > rateKbps)
                {
                    break;
                }
            }
            i--;

            if(m_Rotated)
            {
                frameWidth   = capTable[i].height;
                frameHeight  = capTable[i].width;
            }
            else
            {
                frameWidth   = capTable[i].width;
                frameHeight  = capTable[i].height;
            }
            /*  if(rateKbps <= 384) // TODO model from encoder
             {
             frameRate   = std::min<uint32_t>(m_MaxFrameRate,15);
             }
             else*/
            {
                frameRate   = m_MaxFrameRate;
            }
        }
        else
        {
            // We do not change the size of the stream
            frameWidth  = getWidth();
            frameHeight = getHeight();
            frameRate   = getFramerate();
        }
    }

protected:
    bool                     m_fastUpdateRequested;
    int                      m_IFrameInterval;
    int                      m_PFramesSinceLastIFrame;
    unsigned int             m_MaxWidth;
    unsigned int             m_MaxHeight;
    int                      m_MaxFrameRate;
    unsigned int             m_MaxBitRateKbps;
    bool                     m_Rotated;
    unsigned char*           m_pResizePlane;

    // Support for dynamic resizing
    bool               m_bDynamicSize;
    dynamicCap*        m_pDynamicCapTable;         // The dynamic capability table to used
    const dynamicCap*  m_pIncreaseDynamicCapTable; // If the bitrate increases, an alternate table to test whether
                                                   // to maintain a frame size
    uint32_t           m_MaxDynamicCapEntries;     // How many entries in the table
    uint32_t           m_MaxAllowedEntry;          // The highest cap allowed by static cap
    bool               m_EnableResolutionHysteresis;
    bool               m_HasFrameSizeEverDecreased;
    pj_time_val        m_TimeOfLastResizeDown;
    bool               m_bReferenceFrameSelection;
    static const long  SECONDS_BEFORE_RESIZE_UP = 15L;
    char               m_LogName[32];

    const pjmedia_video_format_info* m_vfi;
    pjmedia_video_apply_fmt_param    m_vafp;
    pjmedia_video_apply_fmt_param    m_vafpo;

    ChannelType                      m_ChannelType;
    media_index                      m_mediaIdx;
    uint32_t                         m_width;
    uint32_t                         m_height;
    uint32_t                         m_CPUwidth;
    uint32_t                         m_CPUheight;
    uint32_t                         m_framerate;
    uint32_t                         m_MaxPayloadSize;
    int                              m_DynamicMaxAllowedEntry;
    bool                             m_isMtgFtrCamRestartEnabled;
    bool                             m_isHWEncodingSupported;
    bool                             m_hasBegunEncoding;
    bool                             m_EnableScreenShareStaticMode;
    bool                             m_UseImprovedTemporalScalability;
    bool                             m_ReduceTemporalScalabilityCPU;
};

//
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// class basevideodecoder

class basevideodecoder
{
public:
    basevideodecoder()
    : m_decBuf(0)
    , m_bCurFrameIFrame(false)
    , m_bNeedKeyframe(false)
    , m_bReferenceFrameSelection(false)
    , m_vfi(0)
    {
        m_decBufSize = 1280*768; // If we exceed this we have a problem.
        m_decBuf = new pj_uint8_t[m_decBufSize];

        pj_bzero(&m_vafp, sizeof(m_vafp));
        m_LogName[0] = 0; // the derived class should initialize this...
    }

    virtual ~basevideodecoder()
    {}

    virtual pj_status_t open(pjmedia_vid_codec_param *attr) = 0;

    virtual pj_status_t bjn_codec_decode(pj_size_t pkt_count,
                                 pjmedia_frame packets[],
                                 unsigned out_size,
                                 pjmedia_frame *output) = 0;
    //-----------------------------------------------------------------------------
    // Name: setReferenceFrameSelection()
    // Desc: Enable/disable reference frame selection
    //-----------------------------------------------------------------------------
    virtual void setReferenceFrameSelection(bool set)
    {
        m_bReferenceFrameSelection = set;
        PJ_LOG(4, (m_LogName, "Decoder set reference frame selection to: %s ", set? "TRUE" : "FALSE"));
    }

    virtual void modifyDecoderInputBufferSize(unsigned int size)
    {
        if(m_decBuf)
        {
            delete[] m_decBuf;
        }
        m_decBufSize = size;
        m_decBuf = new pj_uint8_t[m_decBufSize];
    }

protected:
    void requestKeyFrame(const pj_timestamp &ts, pjmedia_vid_codec *decoder)
    {
        pjmedia_event event;
        pjmedia_event_init(&event, PJMEDIA_EVENT_KEYFRAME_MISSING, &ts, decoder);
        pjmedia_event_publish(
            (pjmedia_event_mgr*)NULL,
            (void*)decoder, &event,
            (pjmedia_event_publish_flag)0);
    }

    pj_uint8_t*                         m_decBuf;
    unsigned                            m_decBufSize;
    bool                                m_bCurFrameIFrame;
    bool                                m_bNeedKeyframe;
    bool                                m_bReferenceFrameSelection;
    char                                m_LogName[32];

    const pjmedia_video_format_info*    m_vfi;
    pjmedia_video_apply_fmt_param       m_vafp;
};

//
////////////////////////////////////////////////////////////////////////////////

#endif
