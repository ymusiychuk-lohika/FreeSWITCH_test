/******************************************************************************
 * PCapWriter implementation
 *
 * Copyright (C) Bluejeans Network, Inc  All Rights Reserved
 *
 * The purpose of a VideoDescriptor is to abstract video parameters.
 *
 * Author: Emmanuel Weber
 * Date:   06/25/2010
 *****************************************************************************/

#ifndef PCAPWRITER_H
#define PCAPWRITER_H

#include <fstream>
#include <stdint.h>
#include <string>
#include <pj/os.h>
#include <pj/pool.h>
#include "bjstatus.h"

namespace BJ {

///////////////////////////////////////////////////////////////////////////////
// class PCapWriter

class PCapWriter
{
public:
    static BJStatus createInstance(
        const std::string&      filename,
       PCapWriter** writer );

    virtual BJStatus handleRtpPacket(const uint8_t* rtp_pkt_start, int32_t rtp_pkt_len,
            unsigned sourceIP, unsigned destinationIP,
            unsigned short sourcePort, unsigned short destinationPort);
    PCapWriter(const std::string& filename);
    virtual ~PCapWriter();

protected:
    void writeHeader();
    void StorePacket(const void *data, int32_t data_length,
            unsigned sourceIP, unsigned destinationIP,
            unsigned short sourcePort, unsigned short destinationPort);

private:
    std::ostream &m_Output;
    uint32_t m_SnapLen;
    std::ofstream m_OutputFile;
    pj_pool_t      *mPool;
    pj_mutex_t     *mPpcapMutex;
};

//
///////////////////////////////////////////////////////////////////////////////
} //namespace BJ

#endif // PCAPWRITER_H
