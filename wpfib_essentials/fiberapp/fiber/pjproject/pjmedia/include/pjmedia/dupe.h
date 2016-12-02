/******************************************************************************
 * This defines a duplicate packet checker, which is essentially a sliding
 * window of packets in which there are two states -- received or not received.
 * This is a C port of the code we use within Denim -- albeit with a smaller
 * window.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Jeff Wallace
 * Date:   Sept 29, 2014
 *****************************************************************************/

#ifndef PJMEDIA_DUPE_H
#define PJMEDIA_DUPE_H

#include <pjmedia/types.h>

PJ_BEGIN_DECL

/* This is a C port of the duplicate packet checker used within denim, though with a couple modifications.
   For example, here the window is 64 pkts rather than 256, as we don't have a generic arbitrary sized bitvector
   structure -- likewise the test for "ancient" packets is a smaller window -- pjproject seems to use 100
   pkts old as a threshold to change streams. */
typedef struct pjmedia_dupe_bitvector
{
    pj_uint64_t bitvector;
    pj_bool_t   first_packet;
    pj_uint16_t max_seq;
} pjmedia_dupe_bitvector;

/** Inits/resets to a default state */
PJ_INLINE(void) pjmedia_dupe_bitvector_init(pjmedia_dupe_bitvector *dupe)
{
    dupe->bitvector = 0;
    dupe->first_packet = PJ_TRUE;
    dupe->max_seq = 0;
}

/** Inserts seqno into the bitvector.  Returns PJ_TRUE if not a dupe, PJ_FALSE if a dupe or very old */
PJ_INLINE(pj_bool_t) pjmedia_dupe_bitvector_insert_and_test(pjmedia_dupe_bitvector *dupe, pj_uint16_t seqno, pj_bool_t is_rtx_packet)
{
    const pj_int16_t BITVECTOR_SIZE = 8 * sizeof(dupe->bitvector);
    const pj_int16_t MAX_NEG_DELTA = -100; /* 100 is a value that pjproject uses elsewhere for max delta checks */
    pj_bool_t  new_stream;
    pj_int16_t delta;

    if (dupe->first_packet)
    {
        dupe->bitvector |= 1;
        dupe->max_seq = seqno;
        dupe->first_packet = PJ_FALSE;
        return PJ_TRUE;
    }

    delta = seqno - dupe->max_seq;
    /* the bit vector is a pj_uint64_t, hence the 64 */
    new_stream = delta >= BITVECTOR_SIZE || delta <= MAX_NEG_DELTA;
    if (new_stream)
    {
        pjmedia_dupe_bitvector_init(dupe);
        return pjmedia_dupe_bitvector_insert_and_test(dupe, seqno, is_rtx_packet);
    }

    if (delta > 0)
    {
        dupe->max_seq = seqno;
        dupe->bitvector <<= delta;
        dupe->bitvector |= 1;
        return PJ_TRUE;
    }

    delta = -delta;
    if (delta < BITVECTOR_SIZE)
    {
        pj_uint64_t bit = ((pj_uint64_t)1) << delta;
        if ((dupe->bitvector & bit) != 0)
        {
            return PJ_FALSE;
        }
        dupe->bitvector |= bit;
        return PJ_TRUE;
    }

    /**
     * If this is an RTX packet and not in the jitter buffer insert range,
     * push it anyway to avoid MISSING frame while decode.
     */
    if(is_rtx_packet){
        return PJ_TRUE;
    }

    return PJ_FALSE;
}

PJ_END_DECL

#endif
