#pragma once
#include "ControlPacket.h"
#include <string.h>
namespace NrfControl {
enum Mode:uint8_t{Drone=1,Car=2};
constexpr unsigned Size=32,Timeout=250;
inline int clamp(int v,int low,int high){return v<low?low:v>high?high:v;}
inline void u16(uint8_t* p,uint16_t v){p[0]=v;p[1]=v>>8;}
inline uint16_t r16(const uint8_t* p){return p[0]|uint16_t(p[1])<<8;}
inline void u32(uint8_t* p,uint32_t v){for(int i=0;i<4;i++)p[i]=v>>(8*i);}
inline uint32_t r32(const uint8_t* p){uint32_t v=0;for(int i=0;i<4;i++)v|=uint32_t(p[i])<<(8*i);return v;}
inline void seal(uint8_t* p){u16(p+30,ControlPacket::crc(p,30));}
inline bool checksum(const uint8_t* p){return r16(p+30)==ControlPacket::crc(p,30);}
inline int throttle(int knob){return clamp((100-knob)/2,0,100);}
inline bool neutral(uint8_t mode,const int8_t* axes){
 if(mode==Drone)return throttle(axes[4])<=5&&axes[0]>=-10&&axes[0]<=10&&axes[2]>=-10&&axes[2]<=10&&axes[3]>=-10&&axes[3]<=10;
 return axes[1]>=-10&&axes[1]<=10&&axes[2]>=-10&&axes[2]<=10;
}
template<class S> void encode(const S& s,uint8_t mode,bool armed,uint16_t sequence,uint32_t session,uint32_t now,int limit,uint8_t* p){
 memset(p,0,Size);memcpy(p,"NRC1",4);u16(p+4,sequence);p[6]=mode;p[7]=armed?1:2;
 const int axes[]={s.LX,s.LY,s.RX,s.RY,s.L_knob,s.R_knob};for(int i=0;i<6;i++)p[8+i]=uint8_t(axes[i]);
 p[14]=s.angleX;p[15]=s.angleY;uint32_t b=ControlPacket::buttons(s);p[16]=b;p[17]=b>>8;p[18]=b>>16;
 u32(p+19,now);p[23]=clamp(limit,25,100);u32(p+24,session);seal(p);
}
inline bool valid(const uint8_t* p){
 if(memcmp(p,"NRC1",4)||!checksum(p)||(p[6]!=Drone&&p[6]!=Car)||(p[7]!=1&&p[7]!=2)||p[18]>3||p[23]<25||p[23]>100||p[28]||p[29])return false;
 for(int i=8;i<16;i++)if(int8_t(p[i])<-100||int8_t(p[i])>100)return false;return true;
}
inline void mix(int throttle,int steering,int limit,int& left,int& right){
 int a=clamp(throttle,-100,100)+clamp(steering,-100,100),b=clamp(throttle,-100,100)-clamp(steering,-100,100);
 int peak=(a<0?-a:a)>(b<0?-b:b)?(a<0?-a:a):(b<0?-b:b);if(peak>100){a=a*100/peak;b=b*100/peak;}
 left=a*clamp(limit,25,100)/100;right=b*clamp(limit,25,100)/100;
}
// Receiver owns the watchdog and arming interlock, independent of the transmitter.
struct Receiver {
 uint8_t mode;bool armed=false,failsafe=true,prepared=false,haveSession=false;uint16_t sequence=0;
 uint32_t session=0,last=0,accepted=0,rejected=0;int8_t axes[8]={};uint32_t buttons=0;int limit=25;
 explicit Receiver(uint8_t m):mode(m){}
 void stop(){armed=false;memset(axes,0,sizeof(axes));axes[4]=100;buttons=0;}
 bool input(const uint8_t* p,uint32_t now){
  if(!valid(p)||p[6]!=mode){rejected++;return false;}
  uint32_t incoming=r32(p+24);uint16_t seq=r16(p+4);
  if(!haveSession||incoming!=session){stop();prepared=false;haveSession=true;session=incoming;sequence=uint16_t(seq-1);}
  // Stop commands always take effect, including duplicates; duplicate data cannot feed the watchdog.
  if(p[7]==2){stop();prepared=true;}
  int16_t delta=int16_t(uint16_t(seq-sequence));if(delta<=0){rejected++;return false;}
  sequence=seq;last=now;failsafe=false;accepted++;
  if(p[7]==1&&!armed&&prepared&&neutral(mode,reinterpret_cast<const int8_t*>(p+8))){armed=true;prepared=false;}
  if(armed&&p[7]==1){memcpy(axes,p+8,8);buttons=p[16]|uint32_t(p[17])<<8|uint32_t(p[18])<<16;limit=p[23];}
  return true;
 }
 void tick(uint32_t now){if(uint32_t(now-last)>=Timeout){stop();failsafe=true;prepared=false;}}
 void feedback(uint8_t* p,uint16_t millivolts=0xffff){
  memset(p,0,Size);memcpy(p,"NRF1",4);u16(p+4,sequence);p[6]=mode;p[7]=(armed?1:0)|(failsafe?2:0)|4;
  u32(p+8,accepted);u32(p+12,rejected);u16(p+16,millivolts);memcpy(p+18,axes,8);p[26]=limit;p[27]=session;p[28]=session>>8;p[29]=session>>16;seal(p);
 }
};
inline bool feedbackValid(const uint8_t* p,uint8_t mode){return !memcmp(p,"NRF1",4)&&p[6]==mode&&!(p[7]&0xf8)&&checksum(p);}
// Standard SBUS 16 x 11-bit channels, no bitfields or host endian assumptions.
inline unsigned sbusValue(int percent){return 172+(unsigned(clamp(percent,0,100))*1639+50)/100;}
inline unsigned sbusAxis(int axis){return 172+(unsigned(clamp(axis,-100,100)+100)*1639+100)/200;}
inline void sbus(const Receiver& r,uint8_t* frame){
 unsigned channels[16];for(int i=0;i<16;i++)channels[i]=sbusValue(50);
 channels[0]=sbusAxis(r.axes[2]*r.limit/100);channels[1]=sbusAxis(-r.axes[3]*r.limit/100);
 channels[2]=r.armed?sbusAxis(-r.axes[4]):172;channels[3]=sbusAxis(r.axes[0]*r.limit/100);
 channels[4]=sbusValue(r.armed?100:0);channels[5]=sbusAxis(r.axes[5]);
 for(int i=6;i<16;i++)channels[i]=sbusValue((r.buttons>>(i-6))&1?100:0);
 memset(frame,0,25);frame[0]=0x0f;for(int i=0;i<16;i++)for(int bit=0;bit<11;bit++)if(channels[i]&(1u<<bit))frame[1+(i*11+bit)/8]|=1u<<((i*11+bit)%8);
 frame[23]=r.failsafe?0x0c:0;frame[24]=0;
}
}
