#pragma once
#include "NrfCustomProfile.h"
#include "ControlPacket.h"
// Original encoder, based on the user's WHEELTEC C30D USART2/USART3 receivers.
// Bluetooth transport is BLE UART (FFE0 or NUS); this is not Classic SPP.
namespace BleControl {
enum Kind { AppDirection, AppSteering, RosVelocity, Custom };
struct Profile {
 uint8_t version=2,kind=AppDirection,interval=1,length=12,checksum=0;
 uint8_t forwardAxis=1,turnAxis=2,forwardReverse=0,turnReverse=0,deadzone=15;
 uint8_t speedLo=172,speedHi=13,turnLo=220,turnHi=5;
 uint8_t map[32]={1,2,3,4,5,6,19,20,21,24,25},bytes[32]={},stopBytes[32]={};
 uint8_t speedKnob=1,turnKnob=0; // 0=configured maximum, 1=left knob, 2=right knob
};
static_assert(sizeof(Profile)==112,"Stable NVS profile");
inline unsigned speed(const Profile& p){return p.speedLo|(unsigned(p.speedHi)<<8);}
inline unsigned angular(const Profile& p){return p.turnLo|(unsigned(p.turnHi)<<8);}
inline void setSpeed(Profile& p,unsigned n){p.speedLo=n;p.speedHi=n>>8;}
inline void setAngular(Profile& p,unsigned n){p.turnLo=n;p.turnHi=n>>8;}
inline bool valid(const Profile& p){
 if(p.version!=2||p.kind>Custom||p.interval>8||p.length<1||p.length>32||(p.checksum&&p.length<2)||p.checksum>3||p.deadzone>50||speed(p)>3500||angular(p)>3000||p.speedKnob>2||p.turnKnob>2)return false;
 if((p.forwardAxis!=1&&p.forwardAxis!=3)||(p.turnAxis!=0&&p.turnAxis!=2)||p.forwardReverse>1||p.turnReverse>1)return false;
 for(auto s:p.map)if(s>31)return false;return true;
}
inline bool restore(const void* bytes,size_t size,Profile& result){
 if(!bytes||!((size==110&&static_cast<const uint8_t*>(bytes)[0]==1)||(size==sizeof(Profile)&&static_cast<const uint8_t*>(bytes)[0]==2)))return false;
 Profile p;memcpy(&p,bytes,size);p.version=2;if(!valid(p))return false;result=p;return true;
}
inline Profile preset(unsigned kind){Profile p;p.kind=kind<=Custom?kind:Custom;if(kind==RosVelocity)p.length=11;return p;}
template<class S> int forward(const Profile& p,const S& s){int v=-(p.forwardAxis==1?s.LY:s.RY);return p.forwardReverse?-v:v;}
template<class S> int turn(const Profile& p,const S& s){int v=p.turnAxis==0?s.LX:s.RX;return p.turnReverse?-v:v;}
inline int dead(int v,unsigned d){return v>=-int(d)&&v<=int(d)?0:v;}
inline int proportional(int v,unsigned d){v=v<-100?-100:v>100?100:v;int a=v<0?-v:v;if(a<=int(d))return 0;int n=(a-int(d))*100/(100-int(d));return v<0?-n:n;}
template<class S> int knobPercent(unsigned choice,const S& s){int v=choice==1?s.L_knob:s.R_knob;v=v<-100?-100:v>100?100:v;int percent=(v+100)/2;return !choice?100:percent<=5?0:percent>=95?100:percent;}
template<class S> bool startReady(const Profile& p,const S& s){return p.kind==Custom||!p.speedKnob||knobPercent(p.speedKnob,s)<=25;}
template<class S> unsigned limit(const Profile& p,const S& s){return speed(p)*knobPercent(p.speedKnob,s)/100;}
template<class S> int linear(const Profile& p,const S& s){return proportional(forward(p,s),p.deadzone)*int(limit(p,s))/100;}
template<class S> int turnPercent(const Profile& p,const S& s){return proportional(turn(p,s),p.deadzone)*knobPercent(p.turnKnob,s)/100;}
template<class S> int appSpeed(const Profile& p,const S& s){int f=proportional(forward(p,s),p.deadzone),t=turnPercent(p,s);int a=(p.kind==AppSteering&&t)?(t<0?-t:t):f?(f<0?-f:f):(t<0?-t:t);return a*int(limit(p,s))/100;}
template<class S> bool centered(const Profile& p,const S& s){return dead(forward(p,s),p.deadzone)==0&&dead(turn(p,s),p.deadzone)==0;}
inline void be16(uint8_t* b,int n){b[0]=uint16_t(n)>>8;b[1]=uint16_t(n);}
template<class S> size_t encode(const Profile& p,const S& input,uint16_t seq,uint8_t* out,size_t capacity,bool stop=false,bool setup=false,bool updateSpeed=true){
 if(!valid(p)||capacity<40)return 0;
 int f=stop?0:proportional(forward(p,input),p.deadzone),t=stop?0:turnPercent(p,input);
 if(p.kind==AppDirection||p.kind==AppSteering){
  size_t n=0;
  // Enter the APP latch at explicitly configured zero speed. Five consecutive
  // A bytes support the old steering latch and the newer consecutive-AA latch.
  // Never prime the receiver using a nonzero speed.
  if(setup&&!stop){const uint8_t prime[]={'{','0','=','0','}',p.kind==AppDirection?uint8_t('I'):uint8_t('K'),'A','A','A','A','A',0};memcpy(out,prime,sizeof(prime));n=sizeof(prime);}
  unsigned target=stop?0:unsigned(appSpeed(p,input));
  if(updateSpeed||setup||stop){int bytes=snprintf(reinterpret_cast<char*>(out+n),capacity-n,"{0=%u}",target);if(bytes<=0||n+size_t(bytes)+1>capacity)return 0;n+=bytes;}
  uint8_t direction=0;
  if(p.kind==AppSteering){if(t>0)direction='C';else if(t<0)direction='G';else if(f>0)direction='A';else if(f<0)direction='E';}
  else if(f>0)direction=t>0?'B':t<0?'H':'A';else if(f<0)direction=t>0?'D':t<0?'F':'E';else if(t)direction=t>0?'C':'G';
  if(target==0)direction=0;out[n++]=direction;return n;
 }
 if(p.kind==RosVelocity){
  out[0]=0x7b;out[1]=out[2]=0;be16(out+3,stop?0:linear(p,input));be16(out+5,0);
  // WHEELTEC positive angular velocity is counterclockwise (left).
  be16(out+7,-t*int(angular(p))/100);out[9]=0;for(int i=0;i<9;i++)out[9]^=out[i];out[10]=0x7d;return 11;
 }
 S sample=input;if(stop){sample.LX=sample.LY=sample.RX=sample.RY=sample.L_knob=sample.R_knob=sample.angleX=sample.angleY=0;}
 NrfCustom::Profile mapped;memcpy(mapped.map,p.map,32);memcpy(mapped.radio.payload,p.bytes,32);mapped.forwardAxis=p.forwardAxis;mapped.turnAxis=p.turnAxis;mapped.forwardReverse=p.forwardReverse;mapped.turnReverse=p.turnReverse;
 if(stop)memcpy(out,p.stopBytes,32);else NrfCustom::encode(mapped,sample,ControlPacket::buttons(sample),seq,out);size_t n=p.length;
 if(p.checksum&&n>1){uint16_t sum=0;for(size_t i=0;i<n-1;i++){if(p.checksum==1)sum^=out[i];else sum+=out[i];}out[n-1]=p.checksum==3?uint8_t(0-sum):uint8_t(sum);}
 return n;
}
}
