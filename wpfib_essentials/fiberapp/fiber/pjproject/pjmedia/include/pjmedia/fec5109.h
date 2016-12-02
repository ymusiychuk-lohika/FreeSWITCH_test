/******************************************************************************
 * FEC encode/decode following RFC 5109
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * The FEC encoder follow RFC 5109 but have the following limitations:
 * - One level of protection only
 * - No packet interleaving
 *
 * Author: Brian Ashford
 * Date:   8/30/2012
 *****************************************************************************/

#include <pj/config.h>
#include <pjmedia/endpoint.h>
#include <pjmedia/jbuf.h>
#include <pjmedia/port.h>
#include <pjmedia/rtcp.h>
#include <pjmedia/transport.h>
#include <pjmedia/types.h>
#include <pj/list.h>

#ifndef __PJMEDIA_FEC5109_H__
#define __PJMEDIA_FEC5109_H__

#define PJMEDIA_MAX_FEC_PACKETS	48

PJ_BEGIN_DECL

/**
 * @defgroup PJMED_VID_STRM Video streams
 * @ingroup PJMEDIA_PORT
 * @brief FEC Decode/Encode
 * @{
 *
 * FEC RFC 5109 is a forward error correction mechanism described in
 * RFC 5109.
 *
 * The FEC decode/encode API supports encoding of fec packets for use by
 * the receiving endpoint and decoding fec packets to enable restoration of
 * dropped packets. All fec information is contained in a fec context and
 * is passed into all fec API calls after its creation by 
 * specifying #pjmedia_fec5109_info structure in the parameter. Application
 * can construct the #pjmedia_fec5109_info structure manually, or use 
 * #pjmedia_fec5109_create() function to construct the strcuture
 */

 typedef enum pjmedia_fecstatus_t {
     PJMEDIA_FEC_RTP_PACKET,
     PJMEDIA_FEC_FEC_PACKET,
     PJMEDIA_FEC_PACKET_RESTORED,
     PJMEDIA_FEC_DUPLICATE_RTP_PACKET,
     PJMEDIA_FEC_PARSE_FAILED,
     PJMEDIA_FEC_PARSE_SUCCEEDED
} pjmedia_fecstatus_t;

/**
 *  FEC RFC5109 info structure
 */
typedef struct pjmedia_fec5109_info pjmedia_fec5109_info;

typedef struct pjmedia_fec5109_enc_info pjmedia_fec5109_enc_info;

typedef struct pjmedia_fec5109_dec_info pjmedia_fec5109_dec_info;

/**
 *  FEC RFC5109 calls used during decode process
 */

/**
 * This function creates a structure that will hold information
 * needed to create the fec and restored rtp packets.
 * 
 * @param pool		Pool to allocate memory.
 * @param encpt		outgoing payload type
 * @param fec_info	A pointer to the fec info structure
 *
 * @return		PJ_SUCCESS if fec info is successfully initialized.
 */

PJ_DECL(pj_status_t)
pjmedia_fec5109_create(pj_pool_t *pool,
				 pj_uint8_t decpt,
				 pj_uint8_t encpt,
				 pj_uint32_t fec_ssrc,
				 pjmedia_fec5109_info** fec_info,
				 pj_bool_t fec_on_unique_ssrc);

PJ_DECL(void)
pjmedia_fec5109_destroy(pjmedia_fec5109_info* fec_info);

/**
 * Feeds a received RTP packet for future use
 * 
 * @param fec_info	A pointer to the fec info structure
 * @param data		Incoming RTP packet
 * @param len		Length of packet
 *
 * @return		returns packet type: fec, rtp, duplicate, or restored packet
 */

PJ_DECL(pj_status_t)
pjmedia_fec5109_recv_pkt(pjmedia_fec5109_info* fec_info,
				 void* data,
				 int	len);

/**
 * This function creates a structure that will hold information
 * needed to create the fec and restored rtp packets.
 * 
 * @param pool		Pool to allocate memory.
 * @param seqnum	Sequence number of packet to restore
 * @param pkt		An outgoing pointer to the restored packet
 * @param len		An outgoing length of the restored packet
 *
 * @return		PJ_SUCCESS if packet is restored
 */
PJ_DECL(pj_status_t)
pjmedia_fec5109_restore_pkt(pjmedia_fec5109_info* fec_info,
				 pj_uint16_t seqnum,
				 void* pkt,
				 void* pktlen);

/**
 * Reset the fec decoder for the next block of protected packets, this
 * can be based on frame boundary, packet count, etc... This also
 * puts the packets used by the decoder back into the main fec pool
 * 
 * @param fec_info	An pointer to the fec info structure
 *
 * @return		PJ_SUCCESS if fec info is successfully initialized.
 */
PJ_DECL(void)
pjmedia_fec5109_dec_reset(pjmedia_fec5109_info* fec_info);

/**
 * Get the restored packets, if any
 * 
 * @param fec_info	A pointer to the fec info structure
 * @param data		Array of restored packets
 * @param len		Array of packet lengths
 * @param num_fec	Number of restored packets
 *
 * @return		PJ_SUCCESS if fec info is successfully initialized.
 */

PJ_DECL(pj_status_t)
pjmedia_fec5109_get_restored_pkts(pjmedia_fec5109_info* fec_info,
				 void* data[],
				 int	len[],
				 int*	num_fec);


/**
 * Tell the fec engine what the valid decode media type is
 * 
 * @param fec_info	A pointer to the fec info structure
 * @param pt		Media payload type
 *
 */
PJ_DECL(void)
pjmedia_fec5109_set_dec_media_pt(pjmedia_fec5109_info* fec_info,
			     pj_uint8_t pt);

/*****************************************************************************
 *  FEC RFC5109 calls used during encode process
 */


/**
 * Feeds an RTP packet to fec engine so it can be protected
 * 
 * @param fec_info	A pointer to the fec info structure
 * @param data		Outgoing RTP packet to be protected
 * @param len		Length of protected packet
 * @param send_fec	Indicates it is time to send the fec packet
 * @return		PJ_SUCCESS if fec info is successfully initialized.
 */

PJ_DECL(pj_status_t)
pjmedia_fec5109_protect_pkt(pjmedia_fec5109_info* fec_info,
				 void* data,
				 int	len);

/**
 * Get the FEC packet that protects the max packet count
 *
 * @param fec_info	A pointer to the fec info structure
 * @param data		Array of FEC packets
 * @param len		Array of packet lengths
 * @param num_fec	Number of fec packets
 *
 * @return		PJ_SUCCESS if fec info is successfully initialized.
 */

PJ_DECL(pj_status_t)
pjmedia_fec5109_get_full_fec_pkts(pjmedia_fec5109_info* fec_info,
				 void* data[],
				 int	len[],
				 int*	num_fec);

/**
 * Get the FEC packet used to protect previously sent rtp packets
 * 
 * @param fec_info	A pointer to the fec info structure
 * @param data		Array of FEC packets
 * @param len		Array of packet lengths
 * @param num_fec	Number of fec packets
 *
 * @return		PJ_SUCCESS if fec info is successfully initialized.
 */

PJ_DECL(pj_status_t)
pjmedia_fec5109_get_fec_pkts(pjmedia_fec5109_info* fec_info,
				 void* data[],
				 int	len[],
				 int*	num_fec);

/**
 * Reset the fec encoder for the next block of protected packets, this
 * can be based on frame boundary, packet count, etc...
 * 
 * @param fec_info	An pointer to the fec info structure
 *
 */
PJ_DECL(void)
pjmedia_fec5109_enc_reset(pjmedia_fec5109_info* fec_info);

/**
 * Sets the fec ssrc and seq no to the RTP header for fec
 * @param fec_info  An pointer to the fec info structure
 * @param hdr RTP header of fec packet
 *
 */
PJ_DECL(void)
pjmedia_fec5109_fill_header(pjmedia_rtp_hdr *hdr, pjmedia_fec5109_info* fec_info);


/**
 * Set the maximum number of packets each fec packet will protect
 *
 * @param fec_info  An pointer to the fec info structure
 * @param num_pkts  The max number of packets
 *
 */
PJ_DECL(void)
pjmedia_fec5109_enc_max_pkts(pjmedia_fec5109_info* fec_info,
                int num_pkts);

/**
 * Inform fec engine of current send loss. This allows fec to determine
 * the how many rtp packets should be protected by fec
 * 
 * @param fec_info	A pointer to the fec info structure
 * @param loss		The percent of loss (rounded up to next percent)
 *
 * @return		PJ_SUCCESS if fec info is successfully initialized.
 */

PJ_DECL(pj_status_t)
pjmedia_fec5109_current_send_loss(pjmedia_fec5109_info* fec_info,
				 unsigned loss);
/**
 * Inform fec engine of current send bandwidth. The returned value
 * will be the bandwidth the video encoder should use.
 * 
 * @param fec_info	An pointer to the fec info structure
 * @param max_bw	Current max send bandwidth
 *
 * @return		The encoder bandwidth after taking fec into account
 */

PJ_DECL(unsigned)
pjmedia_fec5109_get_encode_bw(pjmedia_fec5109_info* fec_info,
				unsigned max_bw);

/**
 * get the maximum size rtp packet that can be protected. Not to
 * be confused with 'protection length'
 *
 * @param fec_info	An pointer to the fec info structure
 *
 * @return		Max size of an rtp packet that can be protected
 */

PJ_DECL(unsigned)
pjmedia_fec5109_get_max_protect_size(pjmedia_fec5109_info* fec_info);


/**
 * @}
 */

PJ_END_DECL

#endif /* __PJMEDIA_FEC5109_H__ */
