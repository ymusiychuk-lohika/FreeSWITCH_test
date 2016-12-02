/*
    NACK seq-list processing APIs
    This file keeps track of all missing seq-num, and also timestamp logic
    that keeps missing seq-num waiting until we're sure FEC can't recover them.
 */
#ifndef __PJMEDIA_NACK_LIST_H__
#define __PJMEDIA_NACK_LIST_H__


/**
 * @file rtp.h
 * @brief RTP packet and RTP session declarations.
 */
#include <pjmedia/types.h>

PJ_BEGIN_DECL

#ifdef _MSC_VER
#   pragma warning(disable:4214)    // bit field types other than int
#endif


#pragma pack()

/**
* Tracks a set of sequence numbers.
*/
#define NACK_LIST_DEBUG                    0

#define NACK_LIST_SIZE                    32 // with 8 we see overwrites at 5% loss
#define NACK_MAX_MISORDER  ((pj_uint16_t)  1000) // restart loss tracking if seq-num jumps
#define NACK_MAX_SEQ       ((pj_uint16_t)0xFFFF)
#define NACK_STR_SIZE                       128

typedef struct rtx_pid_blp
{
    pj_uint16_t pid; // first seq-num of this range - as defined by RTCP
    pj_uint16_t blp; // bitmask for next 16 missing seq-num - as defined by RTCP
} rtx_pid_blp;

typedef struct nack_list
{
    pj_uint32_t write;  // current write pointer
    pj_uint32_t read;   // current read pointer (only if != write)
    pj_uint32_t ts;
    rtx_pid_blp ranges[NACK_LIST_SIZE];

} nack_list;

    static __inline pj_uint32_t nack_list_index_incr(pj_uint32_t index)
            { return (++index % NACK_LIST_SIZE); }
    static __inline pj_uint32_t nack_list_index_decr(pj_uint32_t index)
            { return (--index % NACK_LIST_SIZE); }
#if (NACK_LIST_DEBUG == 0) // i.e. no debug logs
    static __inline void nack_list_print_state(nack_list *seqlist, const char *str) { return; }
#else
    void nack_list_print_state(nack_list *seqlist, const char *str);
#endif
    void nack_list_init(nack_list *seqlist);

#if (NACK_LIST_DEBUG == 0) // i.e. no debug logs
    static __inline pj_bool_t nack_list_is_empty(nack_list *seqlist)
            { return (seqlist->read == seqlist->write); }
#else
    pj_bool_t nack_list_is_empty(nack_list *seqlist);
#endif
    void        nack_list_incr_read     (nack_list *seqlist);
    void        nack_list_incr_write    (nack_list *seqlist);
    void        nack_list_ts_update     (nack_list *seqlist, pj_uint32_t timestamp); // of next received pkt
    void        nack_list_add_seq       (nack_list *seqlist, pj_uint16_t seq);
    void        nack_list_add_seq_range (nack_list *seqlist, pj_uint16_t start, pj_uint16_t len);
    pj_bool_t   nack_list_remove_seq    (nack_list *seqlist, pj_uint16_t seq);
    pj_bool_t   nack_list_get_pid_blp   (nack_list *seqlist, pj_uint16_t *pid, pj_uint16_t *blp);
    //pj_bool_t   nack_list_get_seq(nack_list *seqlist, pj_uint16_t *seq);
    //void        nack_list_append(nack_list *dst, nack_list *src);
    //void        nack_list_copy(nack_list *dst, nack_list *src);
    //void        nack_list_move(nack_list *dst, nack_list *src);


/**
 * @}
 */

PJ_END_DECL


#endif	/* __PJMEDIA_NACK_LIST_H__ */
