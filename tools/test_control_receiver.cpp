#include "../examples/common/ControlReceiver.h"
#include <cassert>
int main(){
 const uint8_t sample[]={0xa5,0x5a,1,1,255,0x9c,0x64,0xff,1,42,0xd6,5,0,2,12,0xf4,0,0,0,0};uint8_t b[20];for(int i=0;i<20;i++)b[i]=sample[i];auto crc=ControlPacket::crc(b,18);b[18]=crc;b[19]=crc>>8;
 ControlReceiver::Stream s;ControlReceiver::Frame f;assert(!s.feed(42,f));for(int i=0;i<19;i++)assert(!s.feed(b[i],f));assert(s.feed(b[19],f));assert(f.seq==255&&f.axes[0]==-100&&f.axes[5]==-42&&f.buttons==0x20005);
 b[19]^=1;for(auto v:b)assert(!s.feed(v,f));b[19]^=1;bool got=false;for(auto v:b)got=s.feed(v,f)||got;assert(got);
 f.neutral=false;f.buttons=1;f.axes[1]=-100;f.axes[2]=100;int l,r;ControlReceiver::differential(f,l,r);assert(l==30&&r==0);f.neutral=true;ControlReceiver::differential(f,l,r);assert(l==0&&r==0);
 ControlReceiver::Watchdog w;assert(w.expired(0));w.accepted(100);assert(!w.expired(399)&&w.expired(400));w.accepted(0xfffffff0u);assert(!w.expired(30)&&w.expired(400));
 puts("PASS: binary stream/noise/fragmentation/CRC/resync/signed axes/buttons/motor gating/valid-frame timeout.");
}
