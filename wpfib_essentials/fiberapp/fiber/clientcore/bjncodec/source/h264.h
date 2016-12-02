//
//  h264.h
//  Acid
//
//  Created by Emmanuel Weber on 8/20/12.
//
////////////////////////////////////////////////////////////////////////////////

#ifndef Acid_h264_h
#define Acid_h264_h

#define H264_PROFILE_LEVEL_ID   "profile-level-id"

#define H264_PACKETIZATION_MODE "packetization-mode"
#define H264_PACKETIZATION_MODE_SINGLE_NAL      "0"
#define H264_PACKETIZATION_MODE_NON_INTERLEAVED "1"
#define H264_PACKETIZATION_MODE_INTERLEAVED     "2"

#define H264_MAX_RCMD_NALU_SIZE "max-rcmd-nalu-size"
#define H264_MAX_MBPS           "max-mbps"
#define H264_MAX_FS             "max-fs"
#define H264_MAX_CPB            "max-cpb"
#define H264_MAX_DPB            "max-dpb"
#define H264_MAX_BR             "max-br"
#define H264_PARAMETER_SETS     "sprop-parameter-sets"

const char* getNALName(int nalu_type);

typedef enum
{
	NALU_SLICE = 1,
	NALU_DPA   = 2,
	NALU_DPB   = 3,
	NALU_DPC   = 4,
	NALU_IDR   = 5,
	NALU_SEI   = 6,
	NALU_SPS   = 7,
	NALU_PPS   = 8,
	NALU_PD    = 9,
	NALU_ESEQ  = 10,
	NALU_ESTRM = 11,
	NALU_FILL  = 12,
	/* 13...23 -> Reserved */
	/* 24...29 -> RFC-3984 */
	NALU_STAP_A = 24,		// Single-time aggregation packet
	NALU_STAP_B = 25,		// Single-time aggregation packet
	NALU_MTAP16 = 26,		// Multi-time aggregation packet
	NALU_MTAP24 = 27,		// Multi-time aggregation packet
	NALU_FU_A   = 28,		// Fragmentation unit
	NALU_FU_B   = 29		// Fragmentation unit
	/* 30...31 -> Unspecified */
} nal_type;


#endif
