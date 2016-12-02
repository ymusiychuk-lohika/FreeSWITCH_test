/******************************************************************************
 * Queue of pjmedia_ref_frame objects.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Brian T. Toombs
 * Date:   08/06/2014
 *****************************************************************************/

#ifndef __PJ_FRAME_QUEUE_H__
#define __PJ_FRAME_QUEUE_H__

#include <pjmedia/types.h>
#include <pj/leaky_queue.h>
#include <pjmedia/ref_frame.h>

PJ_BEGIN_DECL

typedef struct ref_frame_queue
{
    pj_leaky_queue  base;
    pj_mutex_t     *queue_mutex;

} ref_frame_queue;

pj_status_t frame_queue_init(ref_frame_queue *queue, pj_uint32_t max_size);
pj_status_t frame_queue_push_back(ref_frame_queue *queue, pjmedia_ref_frame *frame);
pj_status_t frame_queue_erase(ref_frame_queue *queue);
pjmedia_ref_frame *frame_queue_first_non_matching_timestamp(ref_frame_queue *queue, const pj_timestamp *ts);
pjmedia_ref_frame *frame_queue_first_greater_timestamp(ref_frame_queue *queue, const pj_timestamp *ts);
pjmedia_ref_frame *frame_queue_last(ref_frame_queue *queue);

PJ_END_DECL

#endif	/* __PJ_FRAME_QUEUE_H__ */
