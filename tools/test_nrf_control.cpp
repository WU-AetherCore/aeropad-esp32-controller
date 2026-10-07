#include "../src/NrfControlProtocol.h"
#include <assert.h>
#include <stdio.h>
struct Samples {int LX=0,LY=0,RX=0,RY=0,L_knob=100,R_knob=0,angleX=0,angleY=0;bool a=1,b=1,x=1,o=1,L_up=1,L_down=1,R_up=1,R_down=1,board_L=1,board_R=1,up=1,down=1,left=1,right=1,switch_L1=1,switch_L2=1,switch_R1=1,switch_R2=1;};
unsigned channel(const uint8_t* p,int index){unsigned v=0;for(int b=0;b<11;b++)if(p[1+(index*11+b)/8]&(1u<<((index*11+b)%8)))v|=1u<<b;return v;}
int main(){using namespace NrfControl;assert(sbusAxis(0)==992&&sbusAxis(1)>sbusAxis(0)&&sbusAxis(-1)<sbusAxis(0));Samples s;uint8_t p[32],f[32],bus[25];
 encode(s,Car,false,0,123,0,25,p);assert(valid(p));for(int i=0;i<32;i++){p[i]^=1;assert(!valid(p));p[i]^=1;}
 Receiver car(Car);assert(car.input(p,100)&&!car.armed);encode(s,Car,true,1,123,120,25,p);assert(car.input(p,120)&&car.armed);
 s.LY=-100;s.RX=100;encode(s,Car,true,2,123,140,25,p);assert(car.input(p,140));int l,r;mix(-car.axes[1],car.axes[2],car.limit,l,r);assert(l==25&&r==0);
 assert(!car.input(p,300));car.tick(390);assert(!car.armed&&car.failsafe&&car.axes[1]==0);
 s.LY=s.RX=0;encode(s,Car,true,3,123,410,25,p);car.input(p,410);assert(!car.armed);encode(s,Car,false,4,123,430,25,p);car.input(p,430);encode(s,Car,true,5,123,450,25,p);car.input(p,450);assert(car.armed);
 encode(s,Car,false,5,123,470,25,p);assert(!car.input(p,470)&&!car.armed);encode(s,Car,true,6,999,490,25,p);car.input(p,490);assert(!car.armed);
 Receiver drone(Drone);encode(s,Drone,false,65535,77,0,100,p);drone.input(p,0);s.L_knob=0;encode(s,Drone,true,0,77,20,100,p);drone.input(p,20);assert(!drone.armed);s.L_knob=100;encode(s,Drone,true,1,77,40,100,p);drone.input(p,40);assert(drone.armed);
 s.L_knob=-100;s.RX=100;s.RY=-100;s.LX=-100;encode(s,Drone,true,2,77,60,100,p);drone.input(p,60);sbus(drone,bus);assert(bus[0]==15&&bus[24]==0&&bus[23]==0);assert(channel(bus,0)==1811&&channel(bus,1)==1811&&channel(bus,2)==1811&&channel(bus,3)==172&&channel(bus,4)==1811);
 drone.feedback(f);assert(feedbackValid(f,Drone)&&!feedbackValid(f,Car)&&r32(f+8)==4);assert((f[27]|unsigned(f[28])<<8|unsigned(f[29])<<16)==77);
 drone.tick(310);sbus(drone,bus);assert(!drone.armed&&channel(bus,2)==172&&channel(bus,4)==172&&bus[23]==12);
 Receiver wrap(Car);encode(Samples{},Car,false,1,1,0,25,p);wrap.input(p,0xfffffff0);wrap.tick(0x20);assert(!wrap.failsafe);wrap.tick(0x100);assert(wrap.failsafe);
 encode(s,Car,false,7,999,0,25,p);assert(!drone.input(p,0));p[28]=1;seal(p);assert(!valid(p));p[28]=0;p[8]=127;seal(p);assert(!valid(p));p[8]=0;p[23]=101;seal(p);assert(!valid(p));
 for(int t=-100;t<=100;t++)for(int st=-100;st<=100;st++){mix(t,st,25,l,r);assert(l>=-25&&l<=25&&r>=-25&&r<=25);}
 puts("PASS: NRC1 corruption, arming/neutral, duplicate/stop/session, timeout/wrap, mix bounds, feedback, 16-channel SBUS and failsafe");}
