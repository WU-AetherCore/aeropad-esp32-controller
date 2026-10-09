#include "../src/C30dFeedback.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
int main(){
 C30dFeedback p;C30dFeedback::Frame f;int count=0;
 auto text=[&](const char* s){int n=0;for(;*s;s++)if(p.feed(*s,f))n++;return n;};
 assert(text("junk{A12:34:57:12:34}$")==1&&f.kind==C30dFeedback::None);
 assert(text("{B350:0:0}")==1&&f.kind==C30dFeedback::AppWheels&&f.zero);
 assert(text("{A0:2:57:0}")==1&&!f.zero&&f.right==2);
 assert(text("{A0:0:57:0:0}")==1&&f.zero);
 assert(text("{B350:7:8}")==1&&f.left==7&&f.right==8);
 assert(text("{C1500:500:300}")==1&&f.kind==C30dFeedback::AppParameters&&f.speed==1500);
 assert(text("{A300:0:57:0:0}")==0);
 assert(text("{A0:0:57:0:0junk}")==0);
 uint8_t packet[24]={0x7b,0,0xff,0x06,0,0,0,100};packet[20]=0x2e;packet[21]=0xe0;packet[23]=0x7d;
 for(int i=0;i<22;i++)packet[22]^=packet[i];
 for(auto c:packet)if(p.feed(c,f))count++;
 assert(count==1&&f.kind==C30dFeedback::RosState&&f.vx==-250&&f.vz==100&&f.battery==12000&&!f.zero);
 packet[22]^=1;for(auto c:packet)assert(!p.feed(c,f));
 packet[22]^=1;count=0;for(auto c:packet)if(p.feed(c,f))count++;assert(count==1);
 p.reset();memset(packet,0,24);packet[0]=0x7b;packet[22]=0x7b;packet[23]=0x7d;count=0;for(auto c:packet)if(p.feed(c,f))count++;assert(count==1&&f.zero);
 puts("PASS: official APP A/B/C fragmented and concatenated feedback, malformed frames, ROS24 signed values, checksum rejection/resync and stop detection");
}
