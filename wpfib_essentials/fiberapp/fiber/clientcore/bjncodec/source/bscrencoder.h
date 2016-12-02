//
//  bscrencoder.h
//

#ifndef Fiber_bscrencoder_h
#define Fiber_bscrencoder_h

#include <pjmedia/vid_codec.h>
#include "basevideocodec.h"
extern "C" {
#include <ArithmeticTypes.h>
#include <ScreenEncoder_Api.h>
}

#define ENABLE_OUTPUT_VIDEO_RECORDING   (0)

typedef struct
{
    /* Encoder In Parameters */
    FILE        *mInFile;
    char        mInFileName[256];

    FILE        *mOutFile;
    char       mOutFileName[256];

    uint32_t      mPrintAvgFrameEncTimeFlag;
    uint32_t      mPrintMinFrameEncTimeFlag;
    uint32_t      mPrintMaxFrameEncTimeFlag;
    uint32_t      mOutFilePerFrameEncTimeFlag;
    FILE        *mOutFilePerFrameEncTime;
    uint8_t       mOutFileNamePerFrameEncTime[256];

    uint32_t      mPrintAvgFrameSizeFlag;
    uint32_t      mPrintMinFrameSizeFlag;
    uint32_t      mPrintMaxFrameSizeFlag;
    uint32_t      mOutFilePerFrameSizeFlag;
    FILE        *mOutFilePerFrameSize;
    uint8_t       mOutFileNamePerFrameSize[256];

    uint32_t      mPrintRatioTiles;

    uint32_t      mPrintAvgTileSizeFlag;
    uint32_t      mPrintMinTileSizeFlag;
    uint32_t      mPrintMaxTileSizeFlag;

    uint32_t      mOutFilePerTileStatsPerFrameFlag;
    FILE        *mOutFilePerTileStatsPerFrame;
    uint8_t       mOutFileNamePerTileStatsPerFrame[256];

    uint32_t      mPrintPayloadBitsFlag;

    /* Encoder out stats */
    uint32_t      mNumFrames;

    double      mFrameEncTime;      //frame encoding time
    double      mTotalEncTime;      //for average encoding time calculation
    double      mMinFrameEncTime;   //minimum frame encoding time
    double      mMaxFrameEncTime;   //maximum frame encoding time

    uint32_t      mFrameSize;
    uint32_t      mTotalFrameSize;
    uint32_t      mMinFrameSize;
    uint32_t      mMaxFrameSize;

    uint32_t      mTotalNumTiles_PerTileType[4];
    uint32_t      mTileSizeTotal_PerTileType[4];
    uint32_t      mMinTileSize_PerTileType[4];
    uint32_t      mMaxTileSize_PerTileType[4];

}TBjnScrEncStats;

typedef struct 
{
    uint8_t* planes[4];
    int32_t stride[4];
}TBjnYUVFrame;

//
///////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// class bsrencoder

class bscrencoder : public basevideoencoder
{
public:
    bscrencoder(pjmedia_vid_codec* codec,
                const pj_str_t *logPath,
                pj_pool_t*         pool);
    virtual ~bscrencoder();

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

    pj_status_t setDynamicEncParameters(dynamicEncParameters& params);

private:
    bool init(void);
    bool close(void);
    bool reset(void);
    bool InitAndSetControlSettings(void);

    pj_status_t setSize(unsigned int width, unsigned int height);
    pj_status_t setBitrate(unsigned bitrate);
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

    void encoderFillInDefaultValues(TBjnScrEncoder  *apScrEnc, TBjnScrEncStats     *apScrEncStats);

    void setupDynamicTable(void);

private:
    TBjnScrEncoder*           m_encoder;
    TBjnScrEncStats*          m_encStatistics;
    TBjnEncInOutStruct*       m_encinoutstruct;

    uint16_t                 m_pictureID;
    uint16_t                 m_pictureIDNoRefLast;//The picture ID of the frame which not ref the last frame, but the golden or altref,
                                                  //The frame encoded after receving and handling SLI.
    bool                     m_encBufIsKeyframe;
    uint32_t                 m_uRoundTripTime;
    uint32_t                 m_bitrate;

    //Per frame information
    uint32_t                 m_encFrameLen;
    uint64_t                 m_timeStamp;

    //Indicate if the current frame is a long term reference frame or not
    //it also includes the frame that refer to long term reference frame because of SLI message
    bool                     m_isFirstFrame;
    TBjnYUVFrame             m_inputFrame;
    uint8_t                  *m_sessionHeaderFrame;
    //Session heade size
    uint32_t                 m_sessionHeaderSize;
    uint32_t                 m_encodeOffset;
    //Encoder dynamic configurations
    dynamicEncParameters     m_encDynParameters;
    //Quad-tree mode
    bool                     m_quadTreeMode;
#if ENABLE_OUTPUT_VIDEO_RECORDING
    FILE*                    m_encfp;
    uint32_t                 m_frameCounter;
#endif
};

//
////////////////////////////////////////////////////////////////////////////////

#endif
