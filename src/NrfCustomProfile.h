#pragma once
#include "NrfGenericConfig.h"
#include <string.h>
namespace NrfCustom {
// Each vehicle has five named-by-number slots; all fields are fixed bytes.
struct Profile {
 uint8_t version=2;
 NrfGeneric::Config radio;
 uint8_t map[32]={1,2,3,4,5,6,19,20,21};
 // Forward/back always uses a vertical axis; steering always a horizontal axis.
 uint8_t forwardAxis=1,turnAxis=2,forwardReverse=0,turnReverse=0;
};
// One atomic NVS blob holds both the default route and its complete applied snapshot.
// slot=255 means an explicitly applied current configuration, not a named preset.
struct Selection {uint8_t version=1,kind=0,slot=255;Profile profile;};
inline bool valid(const Profile& p){
 if(p.version!=2||!NrfGeneric::valid(p.radio))return false;
 if((p.forwardAxis!=1&&p.forwardAxis!=3)||(p.turnAxis!=0&&p.turnAxis!=2)||p.forwardReverse>1||p.turnReverse>1)return false;
 for(auto source:p.map)if(source>31)return false;
 return true;
}
inline bool valid(const Selection& s){return s.version==1&&s.kind<=1&&(s.slot<5||s.slot==255)&&valid(s.profile);}
constexpr unsigned LegacySize=sizeof(Profile)-4;
inline bool restore(const void* bytes,unsigned length,Profile& output){
 if(!bytes)return false;const auto* data=static_cast<const uint8_t*>(bytes);Profile candidate=output;
 if((length!=LegacySize||data[0]!=1)&&(length!=sizeof(Profile)||data[0]!=2))return false;
 memcpy(&candidate,bytes,length);if(data[0]==1){for(auto source:candidate.map)if(source>25)return false;}candidate.version=2;if(!valid(candidate))return false;output=candidate;return true;
}
template<class S> int forward(const Profile& p,const S& s){int v=-(p.forwardAxis==1?s.LY:s.RY);return p.forwardReverse?-v:v;}
template<class S> int turn(const Profile& p,const S& s){int v=p.turnAxis==0?s.LX:s.RX;return p.turnReverse?-v:v;}
// Canonical NRC1 channels keep receiver protocol unchanged.
template<class S> S controlSample(const Profile& p,const S& s,uint8_t mode){S result=s;result.RX=turn(p,s);if(mode==1){result.RY=-forward(p,s);result.LX=p.turnAxis==0?s.RX:s.LX;}else result.LY=-forward(p,s);return result;}
template<class S> void encode(const Profile& p,const S& s,uint32_t buttons,uint16_t sequence,uint8_t* out){
 const int axes[]={s.LX,s.LY,s.RX,s.RY,s.L_knob,s.R_knob};
 for(int i=0;i<32;i++){
  unsigned code=p.map[i];int v=0;
  if(code>=1&&code<=18){v=axes[(code-1)%6];v=v<-100?-100:v>100?100:v;
   if(code>=13)v=(v+100)*255/200;else if(code>=7)v+=100;
  }else if(code>=26&&code<=31){v=((code-26)%2)?turn(p,s):forward(p,s);v=v<-100?-100:v>100?100:v;if(code>=30)v=(v+100)*255/200;else if(code>=28)v+=100;}
  else if(code>=19&&code<=21)v=buttons>>(8*(code-19));
  else if(code==22)v=s.angleX;else if(code==23)v=s.angleY;
  else if(code==24)v=sequence;else if(code==25)v=sequence>>8;
  else v=p.radio.payload[i];
  out[i]=uint8_t(v);
 }
}
}
