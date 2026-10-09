#include "../src/NrfUiPolicy.h"
#include "../src/NrfControlProtocol.h"
#include "../src/NrfCustomProfile.h"
#include <cassert>
#include <cstdio>
struct Samples {int LX=0,LY=0,RX=0,RY=0,L_knob=100,R_knob=0,angleX=0,angleY=0;bool a=1,b=1,x=1,o=1,L_up=1,L_down=1,R_up=1,R_down=1,board_L=1,board_R=1,up=1,down=1,left=1,right=1,switch_L1=1,switch_L2=1,switch_R1=1,switch_R2=1;};
int main(){
 using namespace NrfUiPolicy;HoldB b;assert(!b.update(true,false,0));assert(!b.update(true,false,900));b.update(false,false,901);
 assert(!b.update(true,false,1000));assert(!b.update(false,false,1100)); // accidental short B
 assert(!b.update(true,false,2000));assert(!b.update(true,false,2699));assert(b.update(true,false,2700));assert(!b.update(true,false,5000));
 HoldB combo;combo.update(false,false,0);combo.update(true,false,10);assert(!combo.update(true,true,800));
 HoldB wrap;wrap.update(false,false,0);wrap.update(true,false,0xffffff00u);assert(!wrap.update(true,false,0x1bb));assert(wrap.update(true,false,0x1bc));
 assert(!canConfigure(true,false,false)&&!canConfigure(false,true,false)&&!canConfigure(false,false,true));assert(canConfigure(false,false,false));
 Samples s;uint8_t packet[32];NrfControl::Receiver receiver(NrfControl::Drone);
 NrfControl::encode(s,1,false,0,17,0,100,packet);assert(receiver.input(packet,0));
 NrfControl::encode(s,1,true,1,17,20,100,packet);assert(receiver.input(packet,20)&&receiver.armed);
 int page=0;s.L_knob=0;s.RX=30;
 for(unsigned i=2;i<500;i++){
  if(i%7==0)page=standardNext(page,true);assert(page<2); // display changes never enter settings
  NrfControl::encode(s,1,true,i,17,i*20,100,packet);assert(receiver.input(packet,i*20));receiver.tick(i*20);assert(receiver.armed&&!receiver.failsafe);
 }
 NrfControl::encode(s,1,false,500,17,10000,100,packet);assert(receiver.input(packet,10000)&&!receiver.armed);assert(standardNext(1,false)==3);
 NrfCustom::Selection applied;applied.kind=1;applied.slot=2;applied.profile.radio.channel=42;assert(NrfCustom::valid(applied));
 auto draft=applied.profile;draft.radio.channel=7;assert(applied.profile.radio.channel==42);
 NrfCustom::Selection reboot;memcpy(&reboot,&applied,sizeof(reboot));assert(NrfCustom::valid(reboot)&&reboot.kind==1&&reboot.slot==2&&reboot.profile.radio.channel==42);
 reboot.kind=2;assert(!NrfCustom::valid(reboot));reboot.kind=1;reboot.slot=5;assert(!NrfCustom::valid(reboot));
 puts("PASS: short/held/chord B, wrap, configuration lock, 498 uninterrupted armed frames across display changes, stopped receiver and immutable default snapshot");
}
