/*
    NACK list processing APIs
    This is the new NACK processing logic that keeps track of missing sequence numbers
    and compiles list based on the number of times a missing seq no has been NACK'ed
    and TTL of missing packet.
 */
#ifndef __PJMEDIA_NACK_BUILDER_H__
#define __PJMEDIA_NACK_BUILDER_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(PJ_HAS_SYS_TIME_H) && PJ_HAS_SYS_TIME_H!=0
#include <sys/time.h>
#endif
#include <time.h>

#include <pjmedia/types.h>


PJ_BEGIN_DECL

#define MAX_NUM_NACK_RESENT      3
#define TIME_TO_DELETE_MS        2000
#define TIME_BETWEEN_NACKS_MS    15
#define MAX_SUPPORTED_BURST_LOSS 50
#define MAX_NACK_INFO_Q_SIZE     500
#define MAX_NACK_PID_BLP_Q_SIZE  100

/**
 * This struct is used to store info on every missing packet.
 */
struct _nack_info_{
    pj_uint16_t seq_no;
    pj_uint32_t num_nacks_sent;
    pj_time_val first_nack_sent_time;
    struct _nack_info_ *next;
};

typedef struct _nack_info_ nack_info_t;

/**
 * This struct is used to store nack q.
 */
typedef struct  _nack_info_q_ {
    nack_info_t   *head;
    pj_uint32_t   q_size;
    pj_time_val   last_nack_sent;
    pj_uint32_t   max_num_nack_retransmits;
    pj_mutex_t    *nack_tx_mutex;
} nack_info_q_t;

/**
 * Maintains pid and blp in the RFC 4588 format.
 */
typedef struct {
    pj_uint16_t pid;
    pj_uint16_t blp;
} nack_pid_blp_t;

/**
 * This struct holds the list of pid blps that are to be sent in a 
 * RTCP FB NACK packet
 */
typedef struct {
    nack_pid_blp_t pidBlpQ[MAX_NACK_PID_BLP_Q_SIZE];
    pj_uint32_t q_size;
} nack_pid_blp_q_t;

PJ_DEF(pj_bool_t) nack_info_q_has_missing_packets(nack_info_q_t *nack_q);
PJ_DEF(pj_status_t) nack_info_remove_seq_no(nack_info_q_t *nack_q, pj_uint16_t seq_no);
PJ_DEF(pj_status_t) nack_info_init_nack_info_q(pj_pool_t *pool, nack_info_q_t* nack_q);
PJ_DEF(pj_status_t) nack_info_deinit_nack_info_q(nack_info_q_t* nack_q);
PJ_DEF(pj_status_t) nack_info_get_pid_blp_q(nack_info_q_t* nack_q, nack_pid_blp_q_t *nack_pid_blp_q);
PJ_DEF(pj_status_t) nack_info_add_seq_range(nack_info_q_t *nack_q,
                                pj_uint16_t start, pj_uint16_t len);
PJ_DEF(pj_status_t) nack_info_set_num_nack_retransmits(nack_info_q_t* nack_q,
                                pj_uint32_t num_nack_retransmits);
PJ_DEF(pj_status_t) nack_info_reset_nack_info_q(nack_info_q_t* nack_q);


PJ_END_DECL



#endif	/* __PJMEDIA_NACK_BUILDER_H__ */
