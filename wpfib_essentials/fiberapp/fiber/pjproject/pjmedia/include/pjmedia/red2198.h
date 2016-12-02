/******************************************************************************
 * Redundant Audio Data decoder based on RFC 2198
 *
 * Copyright (C) BlueJeans Network, All Rights Reserved
 *
 * Author: Lee Ho
 * Date:   4/18/2012
 *****************************************************************************/

#include <pj/config.h>

#if defined(PJMEDIA_HAS_RED) && (PJMEDIA_HAS_RED != 0)

#include <pjmedia/endpoint.h>
#include <pjmedia/jbuf.h>
#include <pjmedia/port.h>
#include <pjmedia/rtcp.h>
#include <pjmedia/transport.h>
#include <pjmedia/types.h>
#include <pj/list.h>

#ifndef __PJMEDIA_RED2198_H__
#define __PJMEDIA_RED2198_H__

PJ_BEGIN_DECL

typedef enum pjmedia_audio_format_t
{
    RAW_8KHZ_MONO               = 1,
    RAW_16KHZ_MONO              = 2,
    RAW_32KHZ_MONO              = 3,
    G711u                       = 4,
    G711a                       = 5,
    G722                        = 6,
    G7221                       = 7,
    G7221c                      = 8,
    G729                        = 9,
    iLBC                         = 10,
    AAC                         = 11,
    SILK                        = 12,
    iSAC                         = 13
} pjmedia_audio_format_t;

/**
 *  RED RFC 2198 info structure
 */
typedef struct pjmedia_red2198_info pjmedia_red2198_info;

typedef struct pjmedia_red2198_enc_info pjmedia_red2198_enc_info;

typedef struct pjmedia_red2198_dec_info pjmedia_red2198_dec_info;

/**
 * This function creates a structure that will hold information
 * needed to create the red and restored rtp packets.
 *
 * @param pool		Pool to allocate memory.
 * @param encpt		outgoing payload type
 * @param red_info	A pointer to the red info structure
 *
 * @return		PJ_SUCCESS if red info is successfully initialized.
 */

PJ_DECL(pj_status_t)
pjmedia_red2198_create(pj_pool_t *pool,
				 pjmedia_red2198_info** red_info);

/**
 *  RED Encoder
 */

/**
 * Feeds an RTP packet to red engine so it can be protected
 *
 * @param red_info	A pointer to the red info structure
 * @param data		Outgoing RTP packet to be protected
 * @param len		Length of protected packet

 * @return		PJ_SUCCESS means a RED packet was generated, so don't
 * 				send this data, get the RED packet and send that
 */

PJ_DECL(pj_status_t)
pjmedia_red2198_protect_pkt(pjmedia_red2198_info* red_info,
				 void* data,
				 int	len);

/**
 * Get the RED packet that contains the redundant data
 *
 * @param red_info	A pointer to the red info structure
 * @param data		RED packets
 * @param len		RED packet lengths
 * @param num_red	Number of RED packets
 *
 * @return		PJ_SUCCESS if red info is successfully initialized.
 */
PJ_DECL(pj_status_t)
pjmedia_red2198_get_red_pkt(pjmedia_red2198_info* red_info,
				 void** data,
				 int*	len);

/**
 * Set the packet distance to be used when generating the RED packet
 *
 * @param red_info	A pointer to the red info structure
 * @param pkt_distance		Packet Distance
 *
 * @return		PJ_SUCCESS packet distance was set
 */

PJ_DECL(pj_status_t)
pjmedia_red2198_set_enc_pkt_dist(pjmedia_red2198_info* red_info, pj_uint8_t pkt_distance);

/**
 * Set payload type to be used in outgoing RED packets
 *
 * @param red_info	A pointer to the red info structure
 * @param pt		RED payload type
 *
 * @return		PJ_SUCCESS RED payload type was set
 */

PJ_DECL(pj_status_t)
pjmedia_red2198_set_enc_red_pt(pjmedia_red2198_info* red_info, pj_uint8_t pt);

/*****************************************************************************
 *  RED Decoder
 */

/**
 * Pass the received RTP packet into the RED engine. If it RED,
 * get call pjmedia_red2198_get_rtp_pkts() and send them
 *
 * @param red_info	A pointer to the red info structure
 * @param data		Rtp packet data
 * @param len		RTP packet length
  *
 * @return		PJ_SUCCESS if RED packet was processed
 */
PJ_DECL(pj_status_t)
pjmedia_red2198_recvd_red_pkt(pjmedia_red2198_info* red_info,
			     void* data,
			     int len);

/**
 * Get the RTP packets pulled from the RED packets
 *
 * @param red_info	A pointer to the red info structure
 * @param data		Array of RTP packets
 * @param len		Array of packet lengths
 * @param num_rtp	Number of RED packets
 *
 * @return		PJ_SUCCESS if there are rtp packets to deliver
 */

PJ_DECL(pj_status_t)
pjmedia_red2198_get_rtp_pkts(pjmedia_red2198_info* red_info,
				 void* data[],
				 int	len[],
				 int*	num_rtp);


/**
 * Audio media format
 *
 * @param red_info	A pointer to the red info structure
 * @param format	Audio format
 *
 * @return		PJ_SUCCESS value was set correctly
 */
PJ_DECL(pj_status_t)
pjmedia_red2198_set_dec_media_format(pjmedia_red2198_info* red_info, pjmedia_audio_format_t format);

/**
 * Audio format sampling rate
 *
 * @param red_info	A pointer to the red info structure
 * @param rate		Sampling
 *
 * @return		PJ_SUCCESS value was set correctly
 */
PJ_DECL(pj_status_t)
pjmedia_red2198_set_dec_media_sampling_rate(pjmedia_red2198_info* red_info, pj_uint32_t rate);

/**
 * Audio format duration
 *
 * @param red_info	A pointer to the red info structure
 * @param duration	duration
 *
 * @return		PJ_SUCCESS value was set correctly
 */

PJ_DECL(pj_status_t)
pjmedia_red2198_set_dec_media_pkt_duration(pjmedia_red2198_info* red_info, pj_uint32_t duration);

/**
 * Set the payload type of incoming RED packets
 *
 * @param red_info	A pointer to the red info structure
 * @param pt		RED payload type
 *
 * @return		PJ_SUCCESS value was set correctly
 */

PJ_DECL(pj_status_t) pjmedia_red2198_set_dec_red_pt(pjmedia_red2198_info* red_info, pj_uint8_t pt);

PJ_END_DECL

#endif /* __PJMEDIA_RED2198_H__ */

#endif /* PJMEDIA_HAS_RED */


