#pragma once
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
namespace ControlPacket {
enum Format { Binary=0,Json=1,Hex=2,Text=3 };
constexpr int periods[]={20,50,100,200,250,500,1000,1500,2000};
inline bool validPeriod(int value){for(int v:periods)if(v==value)return true;return false;}
inline uint16_t crc(const uint8_t* data,size_t size) {
    uint16_t value=0xffff;
    for(size_t n=0;n<size;n++) {
        value^=uint16_t(data[n])<<8;
        for(int bit=0;bit<8;bit++)value=(value&0x8000)?(value<<1)^0x1021:value<<1;
    }
    return value;
}
template<class S> uint32_t buttons(const S& s) {
    const bool pressed[]={!s.a,!s.b,!s.x,!s.o,!s.L_up,!s.L_down,!s.R_up,!s.R_down,
        !s.board_L,!s.board_R,!s.up,!s.down,!s.left,!s.right,
        !s.switch_L1,!s.switch_L2,!s.switch_R1,!s.switch_R2};
    uint32_t result=0;for(int i=0;i<18;i++)if(pressed[i])result|=1UL<<i;
    return result;
}
template<class S> void binary(const S& s,uint8_t sequence,uint8_t* out,bool neutral=false) {
    uint32_t mask=neutral?0:buttons(s);
    const int axes[]={s.LX,s.LY,s.RX,s.RY,s.L_knob,s.R_knob};
    out[0]=0xa5;out[1]=0x5a;out[2]=1;out[3]=1;out[4]=sequence;
    for(int i=0;i<6;i++)out[5+i]=neutral?0:uint8_t(axes[i]);
    out[11]=mask;out[12]=mask>>8;out[13]=mask>>16;
    out[14]=neutral?0:uint8_t(s.angleX);out[15]=neutral?0:uint8_t(s.angleY);
    out[16]=neutral?1:0;out[17]=0;
    uint16_t check=crc(out,18);out[18]=check;out[19]=check>>8;
}
template<class S> size_t json(const S& s,uint8_t seq,char* out,size_t capacity,bool neutral=false) {
    int length=snprintf(out,capacity,
        "{\"v\":1,\"seq\":%u,\"lx\":%d,\"ly\":%d,\"rx\":%d,\"ry\":%d,\"kl\":%d,\"kr\":%d,\"buttons\":%lu,\"ax\":%d,\"ay\":%d,\"neutral\":%d}\n",
        seq,neutral?0:s.LX,neutral?0:s.LY,neutral?0:s.RX,neutral?0:s.RY,
        neutral?0:s.L_knob,neutral?0:s.R_knob,(unsigned long)(neutral?0:buttons(s)),
        neutral?0:s.angleX,neutral?0:s.angleY,neutral?1:0);
    return length>0 && size_t(length)<capacity?size_t(length):0;
}
template<class S> size_t encode(const S& s,uint8_t seq,int format,uint8_t* out,size_t capacity,bool neutral=false) {
    if(format==Binary){if(capacity<20)return 0;binary(s,seq,out,neutral);return 20;}
    if(format==Json)return json(s,seq,reinterpret_cast<char*>(out),capacity,neutral);
    if(format==Hex){
        if(capacity<61)return 0;uint8_t bytes[20];binary(s,seq,bytes,neutral);
        for(int i=0;i<20;i++)snprintf(reinterpret_cast<char*>(out)+i*3,4,"%02X%c",bytes[i],i==19?'\n':' ');
        return 60;
    }
    if(format!=Text)return 0;
    int n=snprintf(reinterpret_cast<char*>(out),capacity,"AP1 seq=%u LX=%d LY=%d RX=%d RY=%d KL=%d KR=%d BTN=%05lX AX=%d AY=%d N=%d\n",
        seq,neutral?0:s.LX,neutral?0:s.LY,neutral?0:s.RX,neutral?0:s.RY,neutral?0:s.L_knob,neutral?0:s.R_knob,
        (unsigned long)(neutral?0:buttons(s)),neutral?0:s.angleX,neutral?0:s.angleY,neutral?1:0);
    return n>0&&size_t(n)<capacity?size_t(n):0;
}
}
