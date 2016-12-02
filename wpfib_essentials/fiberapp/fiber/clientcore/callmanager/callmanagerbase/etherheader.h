#ifndef __ETHER_HEADER_H__
#define __ETHER_HEADER_H__

#ifdef POSIX
#include <arpa/inet.h>
#endif

#ifdef WIN32
#include <winsock2.h>
#endif
#define ETHER_ADDR_LEN 6
#include "packed.h"
class EtherHeader{
	unsigned char etherDhost[ETHER_ADDR_LEN];
	unsigned char etherShost[ETHER_ADDR_LEN];
    unsigned short etherType;

    public:
        EtherHeader(){
            for(int i = 0; i < ETHER_ADDR_LEN; i++){
                etherDhost[i] = 0x00;
                etherShost[i] = 0x00;
            }
            etherType = htons(0x0800);
        }
        inline void setDestinationMac(unsigned char dhost[]){
            for(int i = 0; i < ETHER_ADDR_LEN; i++){
                etherDhost[i] = dhost[i];
            }
        }
        inline void setSourceMac(unsigned char shost[]){
            for(int i = 0; i < ETHER_ADDR_LEN; i++){
                etherShost[i] = shost[i];
            }
        }
        inline void setEtherType(unsigned short eType){
            etherType = htons(eType);
        }
}PACKED;

class CRCCheckSum{
    unsigned char chksum[4];
    public:
        CRCCheckSum(){
            for(int i = 0; i < 4; i++){
                chksum[i] = 0;
            }
        }
}PACKED;
#include "endpacked.h"
#endif //__ETHER_HEADER_H__
