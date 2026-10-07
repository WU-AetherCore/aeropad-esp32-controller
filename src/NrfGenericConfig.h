#pragma once
#include <stdint.h>
namespace NrfGeneric {
// Fixed byte layout for versioned NVS storage; no pointers or compiler padding.
struct Config {
 uint8_t version=1, channel=2, rate=1, power=0, width=5, crc=1, ack=1, dynamic=0;
 uint8_t rxLength=4, txLength=4, retryDelay=0, retryCount=3, interval=2, format=0, source=0;
 uint8_t txAddress[5]={0x11,0x22,0x33,0x44,0x55}, rxAddress[5]={0x11,0x22,0x33,0x44,0x55};
 uint8_t payload[32]={0,1,2,3};
};
inline bool valid(const Config& c){return c.version==1&&c.channel<=83&&c.rate<3&&c.power<4&&c.width>=3&&c.width<=5&&c.crc<=2&&c.ack<=1&&(!c.ack||c.crc!=0)&&c.dynamic<=1&&c.rxLength>=1&&c.rxLength<=32&&c.txLength>=1&&c.txLength<=32&&c.retryDelay<16&&c.retryCount<16&&c.interval<9&&c.format<3&&c.source<2;}
inline void preset(Config& c,int n){
 c=Config();
 if(n==1){c.channel=76;c.rate=0;c.crc=2;c.rxLength=c.txLength=32;}
 if(n==2){c.channel=76;c.rate=0;c.crc=2;c.rxLength=c.txLength=32;for(int i=0;i<5;i++){c.txAddress[i]='0';c.rxAddress[i]='0';}c.rxAddress[4]='1';}
}
inline uint8_t axis(int n){return uint8_t(n<-100?-100:n>100?100:n);}
}
