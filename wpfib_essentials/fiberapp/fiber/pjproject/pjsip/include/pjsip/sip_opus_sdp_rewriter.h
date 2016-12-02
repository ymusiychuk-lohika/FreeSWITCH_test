/*
 * sip_opus_sdp_rewriter.h
 *
 *  Created on: 1 déc. 2012
 *      Author: r3gis3r
 */

#ifndef PJSIP_PJSIP_OPUS_SDP_REWRITER_H_
#define PJSIP_PJSIP_OPUS_SDP_REWRITER_H_

#include <pj/config_site.h>
#include <pjsua.h>
#if defined(PJMEDIA_HAS_OPUS_CODEC) && (PJMEDIA_HAS_OPUS_CODEC!=0)
#ifdef __cplusplus
extern "C" {
#endif

pj_status_t pjsip_opus_sdp_rewriter_init(unsigned target_clock_rate);

#ifdef __cplusplus
}
#endif
#endif

#endif /* PJSIP_SIPCLF_H_ */
