/******************************************************************************
 * A pjmedia_transport to tack on the absolute send time header extension
 * (http://www.webrtc.org/experiments/rtp-hdrext/abs-send-time) onto outgoing
 * packets.
 *
 * Copyright (C) 2015 Bluejeans Network, All Rights Reserved
 *****************************************************************************/

#ifndef ASTRTPHEADEREXTENSION_SENDER_H
#define ASTRTPHEADEREXTENSION_SENDER_H

#include <pjmedia/transport.h>
#include <string>


// This is a pjmedia_transport to tack on the absolute send time rtp header extension
// onto outgoing rtp packets.
//
// This class has a few peculiarities, however.  We're created with placement new
// on a pjmedia pool, so we don't go through usual C++ new/delete.  Virtual functions
// and complex types (eg. an std::string) shouldn't be used here.
class AbsSendTimeRtpHdrExtSender : public pjmedia_transport
{
public:
    // Factory -- object's lifetime is managed by a pjproject pool
    static AbsSendTimeRtpHdrExtSender *Create(pjmedia_endpt *endpoint,
        pjmedia_transport *upstreamTransport, pj_uint16_t hdrId, bool useMonotonicClock);

    // On by default, but there's possibly use cases in which we want to pause it.
    void Enable(bool shouldSend) { mShouldSend = shouldSend; }

    void SetHeaderId(pj_uint16_t hdrId) { mId = hdrId; }


protected:
    AbsSendTimeRtpHdrExtSender(pj_uint16_t id);

    pj_uint16_t mId;
    bool mShouldSend;
};


#endif

