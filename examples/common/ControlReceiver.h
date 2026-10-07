#pragma once
#include <stdint.h>
#include <stddef.h>
#include "../../src/ControlPacket.h"
// Portable C++11 binary receiver. Call feed once per byte from UART or BLE.
namespace ControlReceiver {
struct Frame {uint8_t seq=0;int axes[8]={};uint32_t buttons=0;bool neutral=true;};
inline bool decode(const uint8_t* b,Frame& f){
    if(b[0]!=0xa5||b[1]!=0x5a||b[2]!=1||b[3]!=1||b[13]&0xfc||b[16]&0xfe||b[17])return false;
    if(ControlPacket::crc(b,18)!=(uint16_t(b[18])|uint16_t(b[19])<<8))return false;
    Frame v;v.seq=b[4];v.buttons=uint32_t(b[11])|uint32_t(b[12])<<8|uint32_t(b[13])<<16;v.neutral=b[16]!=0;
    const int offsets[]={5,6,7,8,9,10,14,15};
    for(int i=0;i<8;i++){int n=b[offsets[i]];v.axes[i]=n<128?n:n-256;if(v.axes[i]<-100||v.axes[i]>100)return false;}
    if(v.neutral){v.buttons=0;for(auto& a:v.axes)a=0;}f=v;return true;
}
class Stream {
    uint8_t buffer[20];size_t used=0;
public:
    void reset(){used=0;}
    bool feed(uint8_t byte,Frame& frame){
        buffer[used++]=byte;if(used<20)return false;
        bool good=decode(buffer,frame);
        if(good)used=0;else {for(size_t i=1;i<20;i++)buffer[i-1]=buffer[i];used=19;}
        return good;
    }
};
class Watchdog {
    uint32_t last=0;bool received=false;
public:
    void accepted(uint32_t now){last=now;received=true;}
    void reset(){received=false;}
    bool expired(uint32_t now,uint32_t timeout=300)const{return !received||uint32_t(now-last)>=timeout;}
};
// Example intent only: two-wheel differential drive; A is a held enable button.
// axes order: LX, LY, RX, RY, KL, KR, AX, AY. Confirm LY/RX signs on your hardware.
inline void differential(const Frame& f,int& left,int& right){
    left=right=0;if(f.neutral||!(f.buttons&1))return;
    auto limit=[](int n){return n<-100?-100:n>100?100:n;};
    left=limit(-f.axes[1]+f.axes[2])*30/100;right=limit(-f.axes[1]-f.axes[2])*30/100;
}
}
