#pragma once
#include "ControlPacket.h"
#include <string.h>
namespace NrfDebugPacket {
constexpr size_t size=32;
inline void put32(uint8_t* p,uint32_t v){for(int i=0;i<4;i++)p[i]=v>>(8*i);}
inline uint32_t get32(const uint8_t* p){uint32_t v=0;for(int i=0;i<4;i++)v|=uint32_t(p[i])<<(8*i);return v;}
template<class S> void encode(const S& s,uint32_t seq,uint32_t uptime,uint8_t* p){
 memset(p,0,size);memcpy(p,"NDG1",4);put32(p+4,seq);put32(p+8,uptime);
 const int axes[]={s.LX,s.LY,s.RX,s.RY,s.L_knob,s.R_knob};for(int i=0;i<6;i++)p[12+i]=uint8_t(axes[i]);
 uint32_t b=ControlPacket::buttons(s);p[18]=b;p[19]=b>>8;p[20]=b>>16;
 uint16_t crc=ControlPacket::crc(p,30);p[30]=crc;p[31]=crc>>8;
}
inline bool valid(const uint8_t* p){
 if(memcmp(p,"NDG1",4)||p[20]>3)return false;
 for(int i=12;i<18;i++)if(int8_t(p[i]) < -100 || int8_t(p[i]) > 100)return false;
 for(int i=21;i<30;i++)if(p[i])return false;
 return ControlPacket::crc(p,30)==(uint16_t(p[30])|(uint16_t(p[31])<<8));
}
}
