//
//  h264qsdecoder.h
//
//
//  Created by Tarun Chawla on 18 Dec 2015.
//  Copyright (c) 2015 Bluejeans. All rights reserved.
//
#ifndef h264qsdecoder_h
#define h264qsdecoder_h

#include <pjmedia/vid_codec_util.h>
#include <pjmedia-codec/h264_packetizer.h>
#include "h264.h"
#include "basevideocodec.h"
#include <pjmedia/errno.h>
#include <time.h>

#if defined(ENABLE_H264_WIN_HWACCL)
#include <mfxdefs.h>
#include <mfxvideo++.h>
#include "h264qsutils.h"
#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)
//#define ENABLE_VIDEO_MEMORY

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

    uint32_t Exp_GolombValue(void)
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

    uint32_t read_bits(uint32_t nBits)
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

    bool isBitSet(uint32_t nBitPos) const
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
// class h264qsdecoder

class h264qsdecoder : public basevideodecoder
{
public:
    h264qsdecoder(pjmedia_vid_codec *codec,
        const pj_str_t *logPath,
        pj_pool_t *pool);

    virtual ~h264qsdecoder();

    pj_status_t open(pjmedia_vid_codec_param *attr);

    pj_status_t bjn_codec_decode(pj_size_t pkt_count,
        pjmedia_frame packets[],
        unsigned out_size,
        pjmedia_frame *output);

private:
    pj_status_t init(void);
    pj_status_t close(void);
    pj_status_t reset(void);
    bool packetParseForFirstPacket(uint32_t nalType, uint32_t packetLen, CNALUnitDecoder &nalDecoder, bool &bFirstPacket);
    bool checkNalForFirstPacket(nal_type nalType, bool& bFirstPacket, CNALUnitDecoder nalDecoder);
    void WriteRawFrame(mfxFrameSurface1* pSurface, uint8_t* fSink);
    mfxStatus InitBitStreamData(mfxBitstream* pBS, pj_uint8_t* decbuf, unsigned size);
    pj_status_t decodeHeader(mfxBitstream mfxBS, mfxVideoParam &par);

private:
    mfxVideoParam               m_mfxVideoParams;
    MFXVideoSession             m_session;
    MFXVideoDECODE*             m_decoder;
    mfxBitstream                m_mfxBS;
    mfxFrameSurface1**          m_pmfxSurfaces;
    mfxU16                      m_numSurfaces;
    mfxFrameAllocResponse       m_mfxResponse;
#if defined(ENABLE_VIDEO_MEMORY)
    VideoMemAllocator           m_videoMemAllocator;
#else
    mfxU8 *                     m_surfaceBuffers;
#endif
    //m_GetFreeSurfaceIndexFail tracks failure rate of GetFreeSurfaceIndex()
    FailureRate                 m_GetFreeSurfaceIndexFail;
    pjmedia_vid_codec*          m_vidCodec;
    pjmedia_rect_size           m_expectedSize;
    pjmedia_format              m_curFormat;
    unsigned int                m_attemptToRecover;
    pj_uint8_t*                 m_OutBuf;
    unsigned int                m_OutSize;

    pjmedia_h264_packetizer*    m_pktz;

    int                         m_decWidth;
    int                         m_decHeight;
    bool                        m_IsKeyFrame;
    bool                        m_bCurFrameIncomplete;
    bool                        m_IsFrameDropped;
    bool                        m_needReset;
    bool                        m_isFirstDecodedFrame;
    bool                        m_initDone;
#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*                       m_decfp;
#endif
};

#endif
#endif