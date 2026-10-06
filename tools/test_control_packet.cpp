#include "../src/ControlPacket.h"
#include <cassert>
#include <cstring>
#include <cstdio>
struct S {
    int LX=-100,LY=100,RX=-1,RY=1,L_knob=42,R_knob=-42,angleX=12,angleY=-12;
    bool a=1,b=1,x=1,o=1,L_up=1,L_down=1,R_up=1,R_down=1,board_L=1,board_R=1,
         up=1,down=1,left=1,right=1,switch_L1=1,switch_L2=1,switch_R1=1,switch_R2=1;
};
int main() {
    assert(ControlPacket::crc((const uint8_t*)"123456789",9)==0x29b1);
    bool S::* fields[]={&S::a,&S::b,&S::x,&S::o,&S::L_up,&S::L_down,&S::R_up,&S::R_down,
        &S::board_L,&S::board_R,&S::up,&S::down,&S::left,&S::right,&S::switch_L1,&S::switch_L2,&S::switch_R1,&S::switch_R2};
    S s;assert(ControlPacket::buttons(s)==0);
    for(int i=0;i<18;i++) {s.*fields[i]=false;assert(ControlPacket::buttons(s)==(1UL<<i));s.*fields[i]=true;}
    for(auto p:fields)s.*p=false;
    assert(ControlPacket::buttons(s)==0x3ffff);
    uint8_t b[20];ControlPacket::binary(s,255,b);
    assert(b[0]==0xa5&&b[1]==0x5a&&b[2]==1&&b[4]==255);
    const int axes[]={-100,100,-1,1,42,-42};for(int i=0;i<6;i++)assert(int8_t(b[5+i])==axes[i]);
    assert(b[11]==255&&b[12]==255&&b[13]==3&&int8_t(b[15])==-12);
    assert(ControlPacket::crc(b,18)==uint16_t(b[18]|uint16_t(b[19])<<8));
    ControlPacket::binary(s,0,b,true);for(int i=5;i<16;i++)assert(b[i]==0);assert(b[16]==1);
    char json[192];auto n=ControlPacket::json(s,7,json,sizeof(json));assert(n&&json[n-1]=='\n');
    assert(strstr(json,"\"buttons\":262143")&&strstr(json,"\"lx\":-100")&&strstr(json,"\"seq\":7"));
    assert(ControlPacket::json(s,0,json,5)==0);
    uint8_t encoded[194];memset(encoded,0xcc,sizeof(encoded));
    assert(ControlPacket::encode(s,255,ControlPacket::Hex,encoded,192)==60);
    assert(encoded[59]=='\n'&&encoded[60]==0&&encoded[192]==0xcc);
    uint8_t original[20];ControlPacket::binary(s,255,original);
    for(int i=0;i<20;i++){unsigned v=0;assert(sscanf((char*)encoded+i*3,"%2x",&v)==1);assert(v==original[i]);}
    assert(ControlPacket::encode(s,1,ControlPacket::Hex,encoded,60)==0);
    auto t=ControlPacket::encode(s,255,ControlPacket::Text,encoded,192);
    assert(t>0&&encoded[t-1]=='\n'&&strstr((char*)encoded,"AP1 seq=255")&&strstr((char*)encoded,"BTN=3FFFF"));
    assert(ControlPacket::encode(s,0,ControlPacket::Text,encoded,192,true)>0&&strstr((char*)encoded,"LX=0")&&strstr((char*)encoded,"BTN=00000")&&strstr((char*)encoded,"N=1"));
    assert(ControlPacket::encode(s,0,ControlPacket::Text,encoded,5)==0);
    assert(ControlPacket::encode(s,0,99,encoded,192)==0);
    for(int p:ControlPacket::periods)assert(ControlPacket::validPeriod(p));assert(!ControlPacket::validPeriod(0)&&!ControlPacket::validPeriod(2001));
    s.LX=s.LY=s.RX=s.RY=s.L_knob=s.R_knob=s.angleX=s.angleY=-100;
    for(int f=0;f<4;f++){auto n=ControlPacket::encode(s,255,f,encoded,192);assert(n&&n<192);printf("format=%d max-field example length=%zu\n",f,n);}
    puts("PASS: 18 independent button bits, six signed axes, two gyro fields, neutral frame, CRC and JSON bounds");
}
