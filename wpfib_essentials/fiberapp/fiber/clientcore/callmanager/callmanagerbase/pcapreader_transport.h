#ifndef PCAPREADER_TRANSPORT_H
#define PCAPREADER_TRANSPORT_H

#include <pjmedia/transport.h>
#include <string>

namespace BJ
{
    class PCapReader;
}

struct pjmedia_pcapreader_setting
{
    BJ::PCapReader  *pcap_reader;
};

PJ_DECL(void) pjmedia_pcapreader_setting_default(pjmedia_pcapreader_setting *opt);

PJ_DECL(pj_status_t) pjmedia_transport_pcapreader_create(
    pjmedia_endpt *endpt,
    pjmedia_transport *tp,
    const pjmedia_pcapreader_setting *opt,
    pjmedia_transport **p_tp);

#endif /* PCAPREADER_TRANSPORT_H */
