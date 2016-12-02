//
//  h264vtdecoder.h
//  Acid
//
//
//

#ifndef Acid_h264vtdecoder_h
#define Acid_h264vtdecoder_h

#include <pjmedia/vid_codec_util.h>
#include <pjmedia-codec/h264_packetizer.h>

#include "basevideocodec.h"

#if defined(ENABLE_H264_OSX_HWACCL)

#include <VideoToolbox/VideoToolbox.h>
#include <CoreMedia/CMTime.h>

#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)

////////////////////////////////////////////////
//CNALUnitDecoder class
class CNALUnitDecoder
{
public:
    //-----------------------------------------------------------------------------
    // Name: CNALUnitDecoder
    // Desc: Constructor
    //-----------------------------------------------------------------------------

    CNALUnitDecoder()
    : m_pNAL(0),
    m_stNAL(0),
    m_ByteOffset(0),
    m_BitOffset(0)
    {
    }

    //-----------------------------------------------------------------------------
    // Name: CNALUnitDecoder
    // Desc: Copy constructor
    //-----------------------------------------------------------------------------

    CNALUnitDecoder(const CNALUnitDecoder& rhs)
    {
        *this = rhs;
    }

    //-----------------------------------------------------------------------------
    // Name: CNALUnitDecoder
    // Desc: Initialization constructor
    //-----------------------------------------------------------------------------

    CNALUnitDecoder(uint8_t* pNAL, uint32_t stNAL)
    : m_pNAL(pNAL),
    m_stNAL(stNAL),
    m_ByteOffset(0),
    m_BitOffset(0)
    {
    }

public:

    //-----------------------------------------------------------------------------
    // Name: operator=
    // Desc: assignment operator
    //-----------------------------------------------------------------------------

    CNALUnitDecoder& operator=(const CNALUnitDecoder& rhs)
    {
        m_pNAL = rhs.m_pNAL;
        m_stNAL = rhs.m_stNAL;
        m_ByteOffset = rhs.m_ByteOffset;
        m_BitOffset = rhs.m_BitOffset;
        return *this;
    }

public:
    //-----------------------------------------------------------------------------
    // Name: Exp_GolombValue
    // Desc: Calculate an Exp Glomb value
    //-----------------------------------------------------------------------------

    uint32_t __attribute__((noinline)) Exp_GolombValue(void)
    {
        uint32_t leadingZeroBits = 0;

        for (uint32_t b = 0; !b; leadingZeroBits++)
        {
            b = read_bits(1);
        }
        leadingZeroBits--;

        return (1 << leadingZeroBits) -1 + read_bits(leadingZeroBits);
    }

    //-----------------------------------------------------------------------------
    // Name: read_bits
    // Desc: Read the specified number of bits
    //-----------------------------------------------------------------------------

    uint32_t __attribute__((noinline)) read_bits(uint32_t nBits)
    {
        uint32_t lResult = 0;

        while (nBits--)
        {
            lResult <<= 1;
            if (isBitSet(m_BitOffset))
                lResult |= 0x01;
            if (m_BitOffset == 7)
                m_BitOffset = 0, m_ByteOffset++;
            else
                m_BitOffset++;
        }
        return lResult;
    }

    //-----------------------------------------------------------------------------
    // Name: isBitSet
    // Desc: Test for non-0 bit
    //-----------------------------------------------------------------------------

    bool __attribute__((noinline)) isBitSet(uint32_t nBitPos) const
    {
        uint8_t aBitMask[] = {128, 64, 32, 16, 8, 4, 2, 1};
        return ((m_pNAL[m_ByteOffset] & aBitMask[nBitPos]) != 0);
    }

    //-----------------------------------------------------------------------------
    // Name: reset
    // Desc: Reset the bit stream to the beginning
    //-----------------------------------------------------------------------------

    void reset(void)
    {
        m_ByteOffset = m_BitOffset = 0;
    }

private:
    uint8_t*      m_pNAL;
    uint32_t      m_stNAL;
    uint32_t      m_ByteOffset;
    uint32_t      m_BitOffset;
};


////////////////////////////////////////////////////////////////////////////////
// class h264vtdecoder

class h264vtdecoder : public basevideodecoder
{
public:
    h264vtdecoder(pjmedia_vid_codec *codec,
                const pj_str_t *logPath,
                pj_pool_t *pool);

    virtual ~h264vtdecoder();

    pj_status_t open(pjmedia_vid_codec_param *attr);

    pj_status_t bjn_codec_decode(pj_size_t pkt_count,
                                 pjmedia_frame packets[],
                                 unsigned out_size,
                                 pjmedia_frame *output);

    void handleOutput(CVImageBufferRef imageBuffer,
                      OSStatus decompStatus,
                      VTDecodeInfoFlags infoFlags);

private:
    pj_status_t init(void);
    pj_status_t close(void);
    pj_status_t reset(void);
    pj_status_t configureDecompressionSession(void);
    pj_status_t createCMSampleBuffer(uint8_t *buffer, 
                                     size_t bufferSize, 
                                     CMSampleBufferRef *sampleBuffer);
    void postHwCodecDisableEvent();
    bool packetParseForFirstPacket(uint32_t nalType, uint32_t packetLen, CNALUnitDecoder &nalDecoder, bool &bFirstPacket);
    bool checkNalForFirstPacket(nal_type nalType, bool& bFirstPacket, CNALUnitDecoder nalDecoder);

private:
    VTDecompressionSessionRef   m_session;
    CMVideoFormatDescriptionRef m_FormatDesc;

    pjmedia_vid_codec*          m_vidCodec;
    pjmedia_rect_size           m_expectedSize;
    pjmedia_format              m_curFormat;

    pj_uint8_t*                 m_OutBuf;
    unsigned int                m_OutSize;

    pjmedia_h264_packetizer*    m_pktz;

    int                         m_decWidth;
    int                         m_decHeight;
    bool                        m_IsKeyFrame;
    bool                        m_bCurFrameIncomplete;
    bool                        m_IsFrameDropped;
#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*                       m_decfp;
#endif
};

//
////////////////////////////////////////////////////////////////////////////////
#endif //ENABLE_H264_OSX_HWACCL
#endif
