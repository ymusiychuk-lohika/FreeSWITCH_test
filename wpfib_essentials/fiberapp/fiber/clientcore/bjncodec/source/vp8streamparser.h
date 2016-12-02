/******************************************************************************
 * VP8 Encoder Plugin codec
 *
 * Copyright (C) Bluejeans Network, All Rights Reserved
 *
 *****************************************************************************/

#ifndef __VP8STREAMPARSER_H__
#define __VP8STREAMPARSER_H__

#include <stdint.h>
#include <memory.h>
#include <string.h>

#define FULL_VP8_HEADER_SIZE    10

///////////////////////////////////////////////////////////////////////////////
// struct RTPPayloadVP8

struct RTPPayloadVP8
{
    bool                 nonReferenceFrame;
    bool                 beginningOfPartition;
    int                  partitionID;
    bool                 hasPictureID;
    bool                 hasTl0PicIdx;
    bool                 hasTID;
    bool                 hasKeyIdx;
    int                  pictureID;
    int                  tl0PicIdx;
    int                  tID;
    bool                 layerSync;
    int                  keyIdx;

    bool                 IFrame;
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
//

enum VP8PacketizerMode
{
    kStrict = 0, // split partitions if too large; never aggregate, balance size
    kAggregate,  // split partitions if too large; aggregate whole partitions
    kSloppy,     // split entire payload without considering partition limits
    kNumModes,
};

enum {kNoPictureId = -1};
enum {kNoTl0PicIdx = -1};
enum {kNoTemporalIdx = -1};
enum {kNoKeyIdx = -1};
struct RTPVideoHeaderVP8
{
    void InitRTPVideoHeaderVP8()
    {
        nonReference = false;
        pictureId = kNoPictureId;
        tl0PicIdx = kNoTl0PicIdx;
        temporalIdx = kNoTemporalIdx;
        layerSync = 0;
        keyIdx = kNoKeyIdx;
        partitionId = 0;
        beginningOfPartition = false;
    }

    bool           nonReference;    // Frame is discardable.
    int16_t        pictureId;       // Picture ID index, 15 bits;
                                    // kNoPictureId if PictureID does not exist.
    int16_t        tl0PicIdx;       // TL0PIC_IDX, 8 bits;
                                    // kNoTl0PicIdx means no value provided.
    int8_t         temporalIdx;     // Temporal layer index, or kNoTemporalIdx.
    bool           layerSync;
    int            keyIdx;
    
    int            partitionId;     // VP8 partition ID
    bool           beginningOfPartition;  // True if this packet is the first
                                          // in a VP8 partition. Otherwise false
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// class RTPFragmentationHeader

class RTPFragmentationHeader
{
public:
    RTPFragmentationHeader() :
        fragmentationVectorSize(0),
        fragmentationOffset(NULL),
        fragmentationLength(NULL),
        fragmentationTimeDiff(NULL),
        fragmentationPlType(NULL)
    {};

    ~RTPFragmentationHeader()
    {
        delete [] fragmentationOffset;
        delete [] fragmentationLength;
        delete [] fragmentationTimeDiff;
        delete [] fragmentationPlType;
    }

    RTPFragmentationHeader& operator=(const RTPFragmentationHeader& header)
    {
        if(this == &header)
        {
            return *this;
        }

        if(header.fragmentationVectorSize != fragmentationVectorSize)
        {
            // new size of vectors

            // delete old
            delete [] fragmentationOffset;
            fragmentationOffset = NULL;
            delete [] fragmentationLength;
            fragmentationLength = NULL;
            delete [] fragmentationTimeDiff;
            fragmentationTimeDiff = NULL;
            delete [] fragmentationPlType;
            fragmentationPlType = NULL;

            if(header.fragmentationVectorSize > 0)
            {
                // allocate new
                if(header.fragmentationOffset)
                {
                    fragmentationOffset = new uint32_t[header.fragmentationVectorSize];
                }
                if(header.fragmentationLength)
                {
                    fragmentationLength = new uint32_t[header.fragmentationVectorSize];
                }
                if(header.fragmentationTimeDiff)
                {
                    fragmentationTimeDiff = new uint16_t[header.fragmentationVectorSize];
                }
                if(header.fragmentationPlType)
                {
                    fragmentationPlType = new uint8_t[header.fragmentationVectorSize];
                }
            }
            // set new size
            fragmentationVectorSize =   header.fragmentationVectorSize;
        }

        if(header.fragmentationVectorSize > 0)
        {
            // copy values
            if(header.fragmentationOffset)
            {
                memcpy(fragmentationOffset, header.fragmentationOffset,
                        header.fragmentationVectorSize * sizeof(uint32_t));
            }
            if(header.fragmentationLength)
            {
                memcpy(fragmentationLength, header.fragmentationLength,
                        header.fragmentationVectorSize * sizeof(uint32_t));
            }
            if(header.fragmentationTimeDiff)
            {
                memcpy(fragmentationTimeDiff, header.fragmentationTimeDiff,
                        header.fragmentationVectorSize * sizeof(uint16_t));
            }
            if(header.fragmentationPlType)
            {
                memcpy(fragmentationPlType, header.fragmentationPlType,
                        header.fragmentationVectorSize * sizeof(uint8_t));
            }
        }
        return *this;
    }
    void VerifyAndAllocateFragmentationHeader( const uint16_t size)
    {
        if( fragmentationVectorSize < size)
        {
            uint16_t oldVectorSize = fragmentationVectorSize;
            {
                // offset
                uint32_t* oldOffsets = fragmentationOffset;
                fragmentationOffset = new uint32_t[size];
                memset(fragmentationOffset+oldVectorSize, 0,
                       sizeof(uint32_t)*(size-oldVectorSize));
                // copy old values
                memcpy(fragmentationOffset,oldOffsets, sizeof(uint32_t) * oldVectorSize);
                delete[] oldOffsets;
            }
            // length
            {
                uint32_t* oldLengths = fragmentationLength;
                fragmentationLength = new uint32_t[size];
                memset(fragmentationLength+oldVectorSize, 0,
                       sizeof(uint32_t) * (size- oldVectorSize));
                memcpy(fragmentationLength, oldLengths,
                       sizeof(uint32_t) * oldVectorSize);
                delete[] oldLengths;
            }
            // time diff
            {
                uint16_t* oldTimeDiffs = fragmentationTimeDiff;
                fragmentationTimeDiff = new uint16_t[size];
                memset(fragmentationTimeDiff+oldVectorSize, 0,
                       sizeof(uint16_t) * (size- oldVectorSize));
                memcpy(fragmentationTimeDiff, oldTimeDiffs,
                       sizeof(uint16_t) * oldVectorSize);
                delete[] oldTimeDiffs;
            }
            // payload type
            {
                uint8_t* oldTimePlTypes = fragmentationPlType;
                fragmentationPlType = new uint8_t[size];
                memset(fragmentationPlType+oldVectorSize, 0,
                       sizeof(uint8_t) * (size- oldVectorSize));
                memcpy(fragmentationPlType, oldTimePlTypes,
                       sizeof(uint8_t) * oldVectorSize);
                delete[] oldTimePlTypes;
            }
            fragmentationVectorSize = size;
        }
    }

    uint16_t    fragmentationVectorSize;    // Number of fragmentations
    uint32_t*   fragmentationOffset;        // Offset of pointer to data for each fragm.
    uint32_t*   fragmentationLength;        // Data size for each fragmentation
    uint16_t*   fragmentationTimeDiff;      // Timestamp difference relative "now" for
                                                  // each fragmentation
    uint8_t*    fragmentationPlType;        // Payload type of each fragmentation
};

//
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
// Packetizer for VP8.

class RtpFormatVp8
{
public:
    // Initialize with payload from encoder and fragmentation info.
    // The payload_data must be exactly one encoded VP8 frame.
    RtpFormatVp8(const uint8_t* payload_data,
                 uint32_t payload_size,
                 const RTPVideoHeaderVP8& hdr_info,
                 const RTPFragmentationHeader& fragmentation,
                 VP8PacketizerMode mode);

    // Initialize without fragmentation info. Mode kSloppy will be used.
    // The payload_data must be exactly one encoded VP8 frame.
    RtpFormatVp8(const uint8_t* payload_data,
                 uint32_t payload_size,
                 const RTPVideoHeaderVP8& hdr_info);

    // Get the next payload with VP8 payload header.
    // max_payload_len limits the sum length of payload and VP8 payload header.
    // buffer is a pointer to where the output will be written.
    // bytes_to_send is an output variable that will contain number of bytes
    // written to buffer. Parameter last_packet is true for the last packet of
    // the frame, false otherwise (i.e., call the function again to get the
    // next packet). Returns the partition index from which the first payload
    // byte in the packet is taken, with the first partition having index 0;
    // returns negative on error.
    int NextPacket(int max_payload_len, uint8_t* buffer,
                   int* bytes_to_send, bool* last_packet);

private:
    enum AggregationMode
    {
        kAggrNone = 0,   // no aggregation
        kAggrPartitions, // aggregate intact partitions
        kAggrFragments   // aggregate intact and fragmented partitions
    };

    static const AggregationMode aggr_modes_[kNumModes];
    static const bool balance_modes_[kNumModes];
    static const bool separate_first_modes_[kNumModes];
    static const int kXBit        = 0x80;
    static const int kNBit        = 0x20;
    static const int kSBit        = 0x10;
    static const int kPartIdField = 0x0F;
    static const int kKeyIdxField = 0x1F;
    static const int kIBit        = 0x80;
    static const int kLBit        = 0x40;
    static const int kTBit        = 0x20;
    static const int kKBit        = 0x10;
    static const int kYBit        = 0x20;

    // Calculate size of next chunk to send. Returns 0 if none can be sent.
    int CalcNextSize(int max_payload_len, int remaining_bytes,
                     bool split_payload) const;

    // Write the payload header and copy the payload to the buffer.
    // Will copy send_bytes bytes from the current position on the payload data.
    // last_fragment indicates that this packet ends with the last byte of a
    // partition.
    int WriteHeaderAndPayload(int send_bytes, uint8_t* buffer,
                              int buffer_length);


    // Write the X field and the appropriate extension fields to buffer.
    // The function returns the extension length (including X field), or -1
    // on error.
    int WriteExtensionFields(uint8_t* buffer, int buffer_length) const;

    // Set the I bit in the x_field, and write PictureID to the appropriate
    // position in buffer. The function returns 0 on success, -1 otherwise.
    int WritePictureIDFields(uint8_t* x_field, uint8_t* buffer,
                             int buffer_length, int* extension_length) const;

    // Set the L bit in the x_field, and write Tl0PicIdx to the appropriate
    // position in buffer. The function returns 0 on success, -1 otherwise.
    int WriteTl0PicIdxFields(uint8_t* x_field, uint8_t* buffer,
                             int buffer_length, int* extension_length) const;

    // Set the T and K bits in the x_field, and write TID, Y and KeyIdx to the
    // appropriate position in buffer. The function returns 0 on success,
    // -1 otherwise.
    int WriteTIDAndKeyIdxFields(uint8_t* x_field, uint8_t* buffer,
                                int buffer_length, int* extension_length) const;

    // Write the PictureID from codec_specific_info_ to buffer. One or two
    // bytes are written, depending on magnitude of PictureID. The function
    // returns the number of bytes written.
    int WritePictureID(uint8_t* buffer, int buffer_length) const;

    // Calculate and return length (octets) of the variable header fields in
    // the next header (i.e., header length in addition to vp8_header_bytes_).
    int PayloadDescriptorExtraLength() const;

    // Calculate and return length (octets) of PictureID field in the next
    // header. Can be 0, 1, or 2.
    int PictureIdLength() const;

    // Check whether each of the optional fields will be included in the header.
    bool XFieldPresent() const;
    bool TIDFieldPresent() const;
    bool KeyIdxFieldPresent() const;
    bool TL0PicIdxFieldPresent() const;
    bool PictureIdPresent() const { return (PictureIdLength() > 0); }

    const uint8_t* payload_data_;
    const int payload_size_;
    RTPFragmentationHeader part_info_;
    int payload_bytes_sent_;
    int part_ix_;
    bool beginning_; // first partition in this frame
    bool first_fragment_; // first fragment of a partition
    const int vp8_fixed_payload_descriptor_bytes_; // length of VP8 payload
                                                   // descriptors's fixed part
    AggregationMode aggr_mode_;
    bool balance_;
    bool separate_first_;
    const RTPVideoHeaderVP8 hdr_info_;
    int first_partition_in_packet_;
};

//
///////////////////////////////////////////////////////////////////////////////

int ParseVP8(RTPPayloadVP8* vp8,uint8_t** data, int* len);

#endif//  __VP8STREAMPARSER_H__



