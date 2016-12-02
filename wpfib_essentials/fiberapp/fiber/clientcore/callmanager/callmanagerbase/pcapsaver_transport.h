#ifndef __PJMEDIA_TRANSPORT_PCAPSAVER_H__
#define __PJMEDIA_TRANSPORT_PCAPSAVER_H__

#include <pjmedia/transport.h>
#include <string>
#include "pcapwriter.h"

PJ_BEGIN_DECL

typedef void (*rtcp_cb) (void *user_data, void *pkt, pj_ssize_t size);
typedef void (*rtp_cb) (void *user_data, void *pkt, pj_ssize_t size);

typedef struct pjmedia_pcapsaver_setting
{
    BJ::PCapWriter  *pcap_writer;
} pjmedia_pcapsaver_setting;

PJ_DECL(void) pjmedia_pcapsaver_setting_default(pjmedia_pcapsaver_setting *opt);
PJ_DECL(void) pjmedia_pcapsaver_setting_set(pjmedia_pcapsaver_setting *opt, pjmedia_transport *tp);

PJ_DECL(pj_status_t) pjmedia_transport_pcapsaver_create(
    pjmedia_endpt *endpt,
    pjmedia_transport *tp,
    const pjmedia_pcapsaver_setting *opt,
    pjmedia_transport **p_tp);

PJ_END_DECL
#endif /* __PJMEDIA_TRANSPORT_PCAPSAVER_H__ */
