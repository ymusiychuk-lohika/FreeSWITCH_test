/* 
    RTX processing APIs
    NACK is processed elsewhere as part of RTCP. This file keeps track of all
    RTP packets of the session - excluding FEC packets. NACK sequence list is
    passed here - any packets found are retransmitted.
 */
#ifndef __PJMEDIA_RTX_H__
#define __PJMEDIA_RTX_H__


/**
 * @file rtx.h
 * @brief RTX - retransmission handling APIs.
 */
#include <pjmedia/types.h>

#include <pjmedia/dupe.h>

PJ_BEGIN_DECL


#ifdef _MSC_VER
#   pragma warning(disable:4214)    // bit field types other than int
#endif

typedef void pjmedia_rtx;

typedef struct pjmedia_rtp_buffer
{
    struct pjmedia_rtp_buffer   *next;	    /**< Next buffer in ring	*/
    struct pjmedia_rtp_buffer   *prev;      /**< previous buffer in ring    */
    unsigned                    size;       /**< Length of this buffer	*/
    unsigned                    datalen;	/**< Amount of data contained	*/
    char                        data[1];	/**< Variable length		*/
} pjmedia_rtp_buffer;

typedef void (*rtx_send_rtp)(void *, const char *, const void *, pj_size_t);

void rtx_init(  pj_pool_t                      *pool, 
                pjmedia_vid_stream_info        *info,
                pj_str_t		               *name,
                rtx_send_rtp                    fn_send_rtp,
                pjmedia_rtx                   **pp_rtx);

void rtx_destroy(pjmedia_rtx *rtx);

void rtx_retransmit(pjmedia_rtx *rtx, void *stream);

pj_bool_t rtx_rx_nack_seqlist_isempty(pjmedia_rtx *rtx);

pjmedia_rtp_buffer *rtx_rtp_buffer_alloc(pjmedia_rtx *rtx);

pj_status_t rtx_rx_nack_mutex_lock  (pjmedia_rtx *r);
pj_status_t rtx_rx_nack_mutex_unlock(pjmedia_rtx *r);

void rtx_rx_nack_list_move(pjmedia_rtx *r, pjmedia_rtp_nack_list *src);
pj_uint32_t rtx_rx_nack_list_size(pjmedia_rtx *r);
void rtx_rx_nack_list_print(pjmedia_rtx *r);
/**
 * @}
 */

PJ_END_DECL


#endif	/* __PJMEDIA_RTX_H__ */
