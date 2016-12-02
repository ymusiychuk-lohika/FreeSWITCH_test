#ifndef BJ_BJSTATUS_H
#define BJ_BJSTATUS_H

namespace BJ
{
    enum BJStatus
    {
        StatusOK                    =  0,
        StatusOkFrameRecovered      =  1000,//special status

        StatusError = -1,
        StatusErrorOutOfMemory = -2,
        StatusErrorInvalidArg = -3,
        StatusErrorInvalidPointer = -4,
        StatusErrorNotImplemented = -5,
        StatusErrorBufferTooSmall = -6,
        StatusErrorUnknownMessage = -7,
        StatusErrorUnknownFormat = -8,
        StatusErrorNeedKeyframe = -9,
        StatusErrorInvalidOperation = -10,
        StatusErrorEndOfFile = -11,
        StatusErrorDuplicatePacket = -12,
        StatusErrorCorruptedPacket = -13,
        StatusErrorEmptyPacket = -14,
        StatusErrorNoMorePackets = -15,
        StatusErrorPacketTooOld = -16,
    };
}

#endif // BJ_BJSTATUS_H
