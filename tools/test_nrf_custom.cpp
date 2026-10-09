#include "../src/NrfCustomProfile.h"
#include <cassert>
#include <cstdio>
#include "../src/NrfExitChord.h"
struct Sample {int LX,LY,RX,RY,L_knob,R_knob,angleX,angleY;};
int main(){
 NrfCustom::Profile p;Sample s{-100,0,100,-42,50,-50,-12,34};uint8_t out[32];
 assert(NrfCustom::valid(p));p.map[31]=32;assert(!NrfCustom::valid(p));p.map[31]=0;
 for(int code=0;code<=31;code++){
  p.map[0]=code;p.radio.payload[0]=0xa5;NrfCustom::encode(p,s,0x2a55a,0xabcd,out);
  if(code==0)assert(out[0]==0xa5);
  if(code==1)assert(int8_t(out[0])==-100);
  if(code==7||code==13)assert(out[0]==0);
  if(code==9)assert(out[0]==200);
  if(code==15)assert(out[0]==255);
  if(code>=19&&code<=21)assert(out[0]==uint8_t(0x2a55a>>(8*(code-19))));
  if(code==22)assert(int8_t(out[0])==-12);if(code==23)assert(out[0]==34);
  if(code==24)assert(out[0]==0xcd);if(code==25)assert(out[0]==0xab);
 }
 for(int a=-150;a<=150;a++){s.LX=a;p.map[0]=13;NrfCustom::encode(p,s,0,0,out);assert(out[0]==(a<-100?0:a>100?255:(a+100)*255/200));}
 // A v1 saved slot must migrate without changing a single radio or mapped byte.
 p.map[0]=1;uint8_t legacy[NrfCustom::LegacySize];memcpy(legacy,&p,sizeof(legacy));legacy[0]=1;
 NrfCustom::Profile upgraded;upgraded.forwardAxis=3;assert(NrfCustom::restore(legacy,sizeof(legacy),upgraded));assert(upgraded.forwardAxis==3);assert(!memcmp(&upgraded.radio,&p.radio,sizeof(p.radio)));assert(!memcmp(upgraded.map,p.map,32));
 legacy[0]=9;assert(!NrfCustom::restore(legacy,sizeof(legacy),upgraded));
 p.forwardAxis=1;p.turnAxis=2;p.forwardReverse=0;p.turnReverse=0;s.LY=-70;s.RX=40;
 assert(NrfCustom::forward(p,s)==70&&NrfCustom::turn(p,s)==40);
 p.map[0]=26;p.map[1]=27;p.map[2]=28;p.map[3]=31;NrfCustom::encode(p,s,0,0,out);assert(out[0]==70&&out[1]==40&&out[2]==170&&out[3]==178);
 p.forwardReverse=1;p.turnReverse=1;assert(NrfCustom::forward(p,s)==-70&&NrfCustom::turn(p,s)==-40);
 p.forwardAxis=3;p.turnAxis=0;s.RY=-60;s.LX=30;auto car=NrfCustom::controlSample(p,s,2);assert(car.LY==60&&car.RX==-30);
 auto drone=NrfCustom::controlSample(p,s,1);assert(drone.RY==60&&drone.RX==-30&&drone.LX==s.RX);
 p.forwardAxis=0;assert(!NrfCustom::valid(p));p.forwardAxis=3;p.turnAxis=1;assert(!NrfCustom::valid(p));p.turnAxis=0;
 NrfExitChord chord;assert(!chord.update(true,true,0));assert(!chord.update(true,true,1000));assert(!chord.update(false,false,1100));
 assert(!chord.update(false,true,1200));assert(!chord.update(false,true,2000));assert(!chord.update(true,false,2200));assert(!chord.update(true,false,3000));
 assert(!chord.update(true,true,3100));assert(!chord.update(true,true,3449));assert(chord.update(true,true,3450));assert(!chord.update(true,true,4000));
 NrfExitChord wrap;wrap.update(false,false,0xfffffe00u);wrap.update(true,true,0xffffff00u);assert(!wrap.update(true,true,0x5du));assert(wrap.update(true,true,0x5eu));
 NrfCustom::Profile saved=p,loaded;memcpy(&loaded,&saved,sizeof(saved));assert(NrfCustom::valid(loaded));assert(!memcmp(&loaded,&saved,sizeof(saved)));
 puts("PASS: 32 byte sources, direction mapping/reversal, v1 slot migration, profile validation, guarded exit/chord rollover");
}
