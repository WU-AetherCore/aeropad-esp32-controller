#include "../src/BleControlProtocol.h"
#include <assert.h>
#include <stdio.h>
struct Sample{
 int LX=0,LY=0,RX=0,RY=0,L_knob=0,R_knob=0,angleX=0,angleY=0;
 bool a=1,b=1,x=1,o=1,L_up=1,L_down=1,R_up=1,R_down=1,board_L=1,board_R=1,up=1,down=1,left=1,right=1,switch_L1=1,switch_L2=1,switch_R1=1,switch_R2=1;
};
int main(){
 auto testPreset=[](unsigned k){auto p=BleControl::preset(k);BleControl::setSpeed(p,250);return p;};
 Sample s;s.L_knob=s.R_knob=100;uint8_t out[64];auto p=testPreset(0);
 assert(BleControl::valid(p)&&BleControl::centered(p,s));s.LY=-100;
 auto n=BleControl::encode(p,s,0,out,64,false,true);
 const uint8_t expected[]={'{','0','=','0','}','I','A','A','A','A','A',0,'{','0','=','2','5','0','}','A'};
 assert(n==sizeof(expected)&&memcmp(out,expected,n)==0);
 n=BleControl::encode(p,s,1,out,64,false,false,false);assert(n==1&&out[0]=='A');
 s.RX=100;n=BleControl::encode(p,s,2,out,64);assert(out[n-1]=='B');
 s.LY=100;n=BleControl::encode(p,s,2,out,64);assert(out[n-1]=='D');
 s.LY=0;s.RX=-100;n=BleControl::encode(p,s,2,out,64);assert(out[n-1]=='G');
 n=BleControl::encode(p,s,2,out,64,true,true);assert(n==6&&memcmp(out,"{0=0}",5)==0&&out[5]==0);
 p=testPreset(1);s.LY=-100;s.RX=100;n=BleControl::encode(p,s,2,out,64);assert(out[n-1]=='C');
 p=testPreset(2);s.LY=-100;s.RX=100;n=BleControl::encode(p,s,2,out,64);
 assert(n==11&&out[0]==0x7b&&out[10]==0x7d&&out[3]==0&&out[4]==250&&out[7]==0xfa&&out[8]==0x24);
 uint8_t x=0;for(int i=0;i<9;i++)x^=out[i];assert(x==out[9]);
 n=BleControl::encode(p,s,2,out,64,true);assert(n==11);for(int i=1;i<9;i++)assert(out[i]==0);assert(out[9]==0x7b);
 // End points, center deadband, smooth monotonic travel, and independent knob limits.
 p=testPreset(0);s.RX=0;int previous=-1;
 for(int stick=0;stick<=100;stick++){s.LY=-stick;int v=BleControl::linear(p,s);assert(v>=previous&&v<=250);previous=v;}
 assert(BleControl::linear(p,s)==250);s.LY=-50;assert(BleControl::linear(p,s)==102);
 s.LY=-100;s.L_knob=0;assert(BleControl::limit(p,s)==125&&BleControl::linear(p,s)==125);
 s.L_knob=-100;assert(BleControl::linear(p,s)==0);n=BleControl::encode(p,s,0,out,64);assert(n==6&&out[5]==0);
 s.L_knob=100;s.LY=100;assert(BleControl::linear(p,s)==-250);
 p.speedKnob=0;s.L_knob=-100;s.LY=-100;assert(BleControl::linear(p,s)==250);
 p.speedKnob=2;s.R_knob=0;assert(BleControl::linear(p,s)==125);
 s.L_knob=s.R_knob=100;s.RX=100;p.turnKnob=2;s.R_knob=-100;assert(BleControl::turnPercent(p,s)==0);
 // Vendor App direction lookup: all eight directions, including backward steering.
 p=testPreset(0);s.L_knob=100;s.R_knob=-100;
 const int axes[8][2]={{-100,0},{-100,100},{0,100},{100,100},{100,0},{100,-100},{0,-100},{-100,-100}};
 for(int i=0;i<8;i++){s.LY=axes[i][0];s.RX=axes[i][1];n=BleControl::encode(p,s,0,out,64);assert(out[n-1]=='A'+i);}
 // Ackermann default steering must work with the unused right knob at minimum.
 p=testPreset(0);s.R_knob=-100;s.L_knob=100;s.LY=-100;s.RX=-100;
 n=BleControl::encode(p,s,0,out,64);assert(out[n-1]=='H');
 s.RX=100;n=BleControl::encode(p,s,0,out,64);assert(out[n-1]=='B');
 s.LY=100;n=BleControl::encode(p,s,0,out,64);assert(out[n-1]=='D');
 s.RX=-100;n=BleControl::encode(p,s,0,out,64);assert(out[n-1]=='F');
 // Preserve pre-existing v1 slot contents while adding the new knob settings.
 p=testPreset(0);uint8_t legacy[110];memcpy(legacy,&p,110);legacy[0]=1;BleControl::Profile migrated;assert(BleControl::restore(legacy,110,migrated));assert(migrated.version==2&&migrated.speedKnob==1&&migrated.turnKnob==0&&BleControl::speed(migrated)==250);
 s.L_knob=s.R_knob=100;s.LY=-100;s.RX=100;
 p=testPreset(3);p.length=6;p.map[0]=0;p.bytes[0]=0xa5;p.map[1]=26;p.map[2]=27;p.map[3]=19;p.map[4]=24;p.checksum=1;s.a=0;
 n=BleControl::encode(p,s,0x1234,out,64);assert(n==6&&out[0]==0xa5&&out[1]==100&&out[2]==100&&out[3]==1&&out[4]==0x34);
 x=0;for(int i=0;i<5;i++)x^=out[i];assert(out[5]==x);
 p.stopBytes[0]=0x5a;p.stopBytes[1]=0x10;n=BleControl::encode(p,s,2,out,64,true);assert(out[0]==0x5a&&out[1]==0x10&&out[2]==0&&out[5]==0x4a);
 p.forwardReverse=1;p.turnReverse=1;p.checksum=3;BleControl::encode(p,s,2,out,64);assert(out[1]==uint8_t(-100)&&out[2]==uint8_t(-100));unsigned sum=0;for(int i=0;i<6;i++)sum+=out[i];assert((sum&255)==0);
 p.map[0]=32;assert(!BleControl::valid(p)&&BleControl::encode(p,s,2,out,64)==0);p.map[0]=0;p.length=33;assert(!BleControl::valid(p));
 p=testPreset(0);BleControl::setSpeed(p,3501);assert(!BleControl::valid(p));
 p=testPreset(0);s.L_knob=s.R_knob=100;assert(!BleControl::startReady(p,s));
 s.L_knob=-100;assert(BleControl::startReady(p,s)); // right turn knob must not block start
 for(int k=-100;k<=-88;k++){s.L_knob=k;assert(BleControl::startReady(p,s));}
 s.L_knob=-50;assert(BleControl::startReady(p,s));s.L_knob=-48;assert(!BleControl::startReady(p,s));p.speedKnob=2;s.R_knob=-100;assert(BleControl::startReady(p,s));s.R_knob=100;assert(!BleControl::startReady(p,s));p.speedKnob=0;assert(BleControl::startReady(p,s));
 p=testPreset(0);BleControl::setSpeed(p,3500);s.L_knob=100;s.LY=-100;s.RX=0;assert(BleControl::valid(p)&&BleControl::linear(p,s)==3500);n=BleControl::encode(p,s,0,out,64);assert(n==9&&memcmp(out,"{0=3500}A",9)==0);

 puts("PASS: C30D APP/ROS vectors, proportional travel, knob limits, startup interlock, safe zero-speed setup, v1 migration, stop frames and checksums");
}
