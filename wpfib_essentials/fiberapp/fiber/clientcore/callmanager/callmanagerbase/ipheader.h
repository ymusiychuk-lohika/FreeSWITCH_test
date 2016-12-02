#ifndef __IP_HEADER_H__
#define __IP_HEADER_H__

#ifdef POSIX
#include <arpa/inet.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif
#define IP_ADDR_LEN 4
#include "packed.h"
class IPHeader{
    unsigned char verhl;
    unsigned char dServices;
    unsigned short totalLen;
    unsigned short identification;
    unsigned short fragmentOffset;
    unsigned char timeToLive;
    unsigned char protocol;
    unsigned short checksum;
    unsigned destinationIP;
    unsigned sourceIP;
    public:
       IPHeader(){
           verhl = 0x25;
           dServices = 0x00;
           identification = 0x0000;
           fragmentOffset = 0x0000;
           timeToLive = 0x00;
           protocol = 0x11;
           checksum = 0x0000;
       }
       inline unsigned short getTotalLen(void){
           return ntohs(totalLen);
       }
       inline void setTotalLen(unsigned short tLen){
           totalLen = htons(tLen);
       }
       inline void calculateCheckSum(){
           unsigned short * ptr = (unsigned short *)this;
           unsigned int sum = 0;
           unsigned int nwords = 10;
           while(nwords > 0){
               sum = sum + *(ptr);
               ptr++;
               nwords--;
           }
           sum = (sum >> 16) + (sum & 0xFFFF);
           sum = (sum >> 16) + (sum & 0xFFFF);
           checksum = ~sum;
       }
        inline unsigned getSourceAddr(void){
            return ntohl(sourceIP);
        }
        inline unsigned getDestinationAddr(void){
            return ntohl(destinationIP);
        }
        inline void setSourceAddr(unsigned addr){
            sourceIP = htonl(addr);
        }
        inline void setDestinationAddr(unsigned addr){
            destinationIP = htonl(addr);
        }
}PACKED;
#include "endpacked.h"
#endif //__IP_HEADER_H__
