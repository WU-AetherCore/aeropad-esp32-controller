#include "../src/ControlPacket.h"
struct Inputs {
 int LX=0,LY=0,RX=0,RY=0,L_knob=0,R_knob=0,angleX=0,angleY=0;
 bool a=true,b=true,x=true,o=true,L_up=true,L_down=true,R_up=true,R_down=true,board_L=true,board_R=true,up=true,down=true,left=true,right=true,switch_L1=true,switch_L2=true,switch_R1=true,switch_R2=true;
};
int main(){bool first=true;puts("[");for(int sample=0;sample<3;sample++){Inputs s;if(sample){s.LX=-100;s.LY=100;s.RX=-1;s.RY=1;s.L_knob=42;s.R_knob=-42;s.angleX=12;s.angleY=-12;s.a=s.x=s.switch_R2=false;}bool neutral=sample==2;int seq=sample==0?0:sample==1?255:0;for(int format=0;format<4;format++){uint8_t wire[192];auto n=ControlPacket::encode(s,seq,format,wire,sizeof wire,neutral);if(!n)return 1;if(!first)puts(",");first=false;
 printf("{\"case\":\"%s\",\"format\":\"%s\",\"wire_hex\":\"",sample==0?"idle":sample==1?"signed-buttons":"release",format==0?"binary":format==1?"json":format==2?"hex":"text");for(size_t i=0;i<n;i++)printf("%02X",wire[i]);
 printf("\",\"expected\":{\"seq\":%d,\"lx\":%d,\"ly\":%d,\"rx\":%d,\"ry\":%d,\"kl\":%d,\"kr\":%d,\"buttons\":%lu,\"ax\":%d,\"ay\":%d,\"neutral\":%d}}",seq,neutral?0:s.LX,neutral?0:s.LY,neutral?0:s.RX,neutral?0:s.RY,neutral?0:s.L_knob,neutral?0:s.R_knob,(unsigned long)(neutral?0:ControlPacket::buttons(s)),neutral?0:s.angleX,neutral?0:s.angleY,neutral);}}
 puts("\n]");}
