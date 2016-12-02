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

#ifndef PCAPREADER_H
#define PCAPREADER_H

#include <fstream>
#include <stdint.h>
#include <string>
#include <pj/types.h>
#include "bjstatus.h"

namespace BJ
{

///////////////////////////////////////////////////////////////////////////////
// class PCapReader

class PCapReader
{
public:
    PCapReader(const std::string& pcapfilename);
    ~PCapReader();

    // does what the name suggests.. if successful, use the following accessors to get at the frame
    BJStatus readNextPacket();

    uint8_t *getFrame() const { return m_Buffer; }
    uint32_t frameSize() const { return m_FrameSize; }
    uint64_t frameTime() const { return m_FrameTime; }

    BJStatus rewind();

    bool isOpen() { return m_InputFile.is_open(); }

private:
    bool init();
    void close();
    BJStatus readNextData(void* ptr, uint32_t count);

    std::string   m_PcapFileName;
    std::ifstream m_InputFile;

    uint8_t      *m_Buffer;
    uint32_t      m_FrameSize;
    uint64_t      m_FrameTime;
    pj_time_val   m_BaseTime;

    uint32_t      m_SnapLen;
    uint32_t      m_PcapHdrNetwork;
    bool          m_SwapBytes;
};


//
///////////////////////////////////////////////////////////////////////////////
} //namespace BJ

#endif // PCAPWRITER_H
