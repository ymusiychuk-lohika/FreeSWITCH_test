/******************************************************************************
 * Reference-counted pjmedia_frame.
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 * Author: Brian T. Toombs
 * Date:   08/06/2014
 *****************************************************************************/

#ifndef __PJ_REF_FRAME_H__
#define __PJ_REF_FRAME_H__

#include <pjmedia/frame.h>

PJ_BEGIN_DECL
/**
 * The pjmedia_ref_frame wraps a reference-counted pjmedia_frame.  Reference counting
 * is assumed to be implemented externally and exposed through the ref_frame_add_ref
 * and ref_frame_dec_ref functions.
 */
typedef struct pjmedia_ref_frame {
    pjmedia_frame   base;	    /**< Base frame info */
    void *obj;                  /* The underlying C++ frame object */
} pjmedia_ref_frame;

/**
 * Functions for adding and decrementing references to the C++ frame inside a pjmedia_ref_frame.
 * ref_frame_add_ref - Adds a reference to the specified ref_frame_obj and returns the new ref count.
 *                     ref_frame_obj must point to a valid pjmedia_ref_frame.
 * ref_frame_dec_ref - Decrements a reference to the specified ref_frame_obj and returns the new ref count.
 *                     ref_frame_obj must point to a valid pjmedia_ref_frame.
 *                     If this function returns 0, the ref_frame_obj pointer has become invalid and must
 *                     not be accessed anymore.
 */
extern pj_int32_t ref_frame_add_ref(void *ref_frame_obj);
extern pj_int32_t ref_frame_dec_ref(void *ref_frame_obj);

PJ_END_DECL

#endif	/* __PJ_REF_FRAME_H__ */
