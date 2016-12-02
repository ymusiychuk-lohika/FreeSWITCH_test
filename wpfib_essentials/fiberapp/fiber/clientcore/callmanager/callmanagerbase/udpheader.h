#ifndef __UDP_HEADER_H__
#define __UDP_HEADER_H__

#ifdef POSIX
#include <arpa/inet.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif
#include "packed.h"
class UDPHeader{
    unsigned short sourcePort;
    unsigned short destinationPort;
    unsigned short length;
    unsigned short checksum;
    public:
        UDPHeader(){
            sourcePort = 0x0000;
            destinationPort = 0x0000;
            length = 0x0000;
            checksum = 0x0000;
        }
        inline void setLength(unsigned short len){
            length = htons(len);
        }
        inline void setSourcePort(unsigned short port){
            sourcePort = htons(port);
        }
        inline void setDestinationPort(unsigned short port){
            destinationPort = htons(port);
        }
}PACKED;
#include "endpacked.h"
#endif // __UDP_HEADER_H__
