#include "../src/NrfDebugPacket.h"
#include <assert.h>
#include <stdio.h>
struct Samples{int LX=-100,LY=100,RX=-1,RY=1,L_knob=42,R_knob=-42;int a=0,b=1,x=1,o=1,L_up=1,L_down=1,R_up=1,R_down=1,board_L=1,board_R=1,up=1,down=1,left=1,right=1,switch_L1=1,switch_L2=1,switch_R1=1,switch_R2=0;};
int main(){Samples s;uint8_t p[32];NrfDebugPacket::encode(s,0xffffffff,12345,p);assert(NrfDebugPacket::valid(p));assert(NrfDebugPacket::get32(p+4)==0xffffffff);assert(NrfDebugPacket::get32(p+8)==12345);assert(int8_t(p[12])==-100);assert(p[18]==1&&p[20]==2);for(int i=0;i<32;i++){p[i]^=1;assert(!NrfDebugPacket::valid(p));p[i]^=1;}p[21]=1;auto c=ControlPacket::crc(p,30);p[30]=c;p[31]=c>>8;assert(!NrfDebugPacket::valid(p));puts("PASS: NDG1 layout, signed axes, 18 buttons, sequence, CRC and reserved fields");}
