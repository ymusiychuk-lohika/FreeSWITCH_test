/******************************************************************************
 * This file defines classes for doing temporal layers with VP8.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Jie Zhang
 * Date:   04/03/2014
 *****************************************************************************/
#ifndef VP8_TEMPORAL_LAYERS_H_
#define VP8_TEMPORAL_LAYERS_H_

#include <pjmedia/vid_codec.h>
#include "basevideocodec.h"
#include "vp8streamparser.h"

// libvpx forward declaration.
typedef struct vpx_codec_enc_cfg vpx_codec_enc_cfg_t;

enum { kMaxTemporalStreams = 4};

// Ratio allocation between temporal streams:
// Values as required for the VP8 codec (accumulating).
static const float
    kVp8LayerRateAlloction[kMaxTemporalStreams][kMaxTemporalStreams] =
    {
        {1.0f, 0, 0, 0},  // 1 layer
        {0.6f, 1.0f , 0 , 0},  // 2 layers {60%, 40%}
        {0.6f, 0.8f , 1.0f, 0},  // 3 layers {60%, 20%, 20%}
        {0.25f, 0.4f, 0.6f, 1.0f}  // 4 layers {25%, 15%, 20%, 40%}
    };

static const dynamicQP dynamicSVCQPTable[kMaxTemporalStreams][2] =
{
    {
        { 0,    4, 38 },
        { 1200, 4, 38 }, //layer-0
    },
    {
        { 0,    4, 44 },
        { 1200, 4, 42 }, //layer-1
    },
    {
        { 0,    4, 48 },
        { 1200, 4, 48 }, //layer-2
    },
    {
        { 0,    4, 48 },
        { 1200, 4, 48 }, //layer-3
    },
};

struct TemporalLayerInfo
{
    int                  tl0PicIdx;
    int                  tID;
    bool                 layerSync;
};

class TemporalLayers
{
public:
    TemporalLayers(unsigned int numberOfTemporalLayers, uint8_t initialTl0PicIdx);
    virtual ~TemporalLayers() {}

    // Returns the recommended VP8 encode flags needed. May refresh the decoder
    // and/or update the reference buffers.
    virtual int encodeFlags();

    virtual bool configureBitrates(int bitrate_kbit, vpx_codec_enc_cfg_t* cfg);

    //Not sure if m_bReferenceFrames is being set correctly or not.
    //but this is only for long term reference + temporal layers
    //And we are not enable long term reference currently.
    //Need revisit this later
    virtual bool isReferenceFrame()
    {
        return m_bReferenceFrames[m_patternIdx % m_temporalIdsLength];
    }

    virtual TemporalLayerInfo getLayerInfo(uint32_t timestamp);

    virtual void setTemporalLayers(unsigned int numLayers)
    {
        m_numTemporalLayers = numLayers;
        m_patternIdx = 255;
    }

    virtual unsigned int getNumTemporalLayers() const
    {
        return m_numTemporalLayers;
    }

    virtual void resetPatternInx()
    {
        m_patternIdx = 0;//for key frame
    }

    virtual void setImprovedTemporalScalability(bool useIt)
    {
        m_useImprovedTemporalScalability = useIt;
    }

    virtual bool useImprovedTemporalScalability()
    {
        return m_useImprovedTemporalScalability;
    }

    virtual void adjustForDroppedFrame(bool wasDropped);

private:
    enum TemporalReferences {
    // Highest enhancement layer without dependency on golden with alt ref
    // dependency.
    kTemporalUpdateNoneRefAltRef = 6,
    // Highest enhancement layer without dependency on alt ref with golden
    // dependency.
    kTemporalUpdateNoneRefGolden  = 5,
    // Highest enhancement layer.
    kTemporalUpdateNoneRefLast = 4,
    // Second enhancement layer
    kTemporalUpdateAltRefRefGolden = 3,
    // Second enhancement layer without dependency on previous frames in
    // the second enhancement layer.
    kTemporalUpdateAltRefRefLast = 2,
    // First enhancement layer without dependency on previous frames in
    // the first enhancement layer.
    kTemporalUpdateGoldenRefLast = 1,
    // Base layer.
    kTemporalUpdateLast = 0,
    };
    enum { kMaxTemporalPattern = 16 };

    unsigned int          m_numTemporalLayers;
    int                   m_temporalIdsLength;
    int                   m_temporalIds[kMaxTemporalPattern];
    bool                  m_layerSyncs[kMaxTemporalPattern];
    bool                  m_bReferenceFrames[kMaxTemporalPattern];
    int                   m_temporalPatternLength;
    TemporalReferences    m_temporalPattern[kMaxTemporalPattern];
    uint8_t               m_patternIdx;
    uint8_t               m_tl0PicIdx;
    uint32_t              m_timestamp;
    bool                  m_useImprovedTemporalScalability;
};

#endif  // VP8_TEMPORAL_LAYERS_H_
