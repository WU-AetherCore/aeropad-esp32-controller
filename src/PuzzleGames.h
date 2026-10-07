#pragma once
#include <stdint.h>
#include <algorithm>
namespace LocalGames {
struct Merge2048 {
    uint32_t cells[16]={},previous[16]={};int score=0,oldScore=0;bool undoReady=false,over=false;
    void spawn(uint32_t seed){int empty[16],n=0;for(int i=0;i<16;i++)if(!cells[i])empty[n++]=i;if(n)cells[empty[seed%n]]=seed%10==0?4:2;}
    void reset(uint32_t seed){for(auto& c:cells)c=0;score=0;over=undoReady=false;spawn(seed);spawn(seed/17+3);}
    int index(int line,int pos,int d)const{return d==0?pos*4+line:d==1?line*4+3-pos:d==2?(3-pos)*4+line:line*4+pos;}
    bool available()const{for(int i=0;i<16;i++)if(!cells[i]||(i%4<3&&cells[i]==cells[i+1])||(i<12&&cells[i]==cells[i+4]))return true;return false;}
    bool move(int d,uint32_t seed){if(d<0||d>3||over)return false;uint32_t saved[16];std::copy(cells,cells+16,saved);int before=score;
        for(int line=0;line<4;line++){uint32_t row[4]={},merged[4]={};int n=0,m=0;for(int pos=0;pos<4;pos++){auto v=cells[index(line,pos,d)];if(v)row[n++]=v;}
            for(int i=0;i<n;i++){uint32_t v=row[i];if(i+1<n&&v==row[i+1]){v*=2;score+=v;i++;}merged[m++]=v;}for(int pos=0;pos<4;pos++)cells[index(line,pos,d)]=merged[pos];}
        bool changed=!std::equal(cells,cells+16,saved);if(changed){std::copy(saved,saved+16,previous);oldScore=before;undoReady=true;spawn(seed);}over=!available();return changed;}
    void undo(){if(undoReady){std::copy(previous,previous+16,cells);score=oldScore;undoReady=false;over=false;}}
};
struct Tetris {
    uint8_t board[20][10]={};int piece=0,next=1,rotation=0,x=3,y=-1,score=0,lines=0;bool over=false;
    uint8_t bag[7]={};int bagAt=7;
    static uint16_t mask(int p,int r){static const uint16_t shapes[7][4]={{0x0F00,0x2222,0x00F0,0x4444},{0x6600,0x6600,0x6600,0x6600},{0x0E40,0x4C40,0x4E00,0x4640},{0x06C0,0x8C40,0x06C0,0x8C40},{0x0C60,0x4C80,0x0C60,0x4C80},{0x08E0,0x6440,0x0E20,0x44C0},{0x02E0,0x4460,0x0E80,0xC440}};return shapes[p][r%4];}
    static bool cell(int p,int r,int cx,int cy){return mask(p,r)&(0x8000>>(cy*4+cx));}
    int draw(uint32_t seed){if(bagAt>=7){for(int i=0;i<7;i++)bag[i]=i;for(int i=6;i>0;i--){seed=1664525*seed+1013904223;std::swap(bag[i],bag[seed%(i+1)]);}bagAt=0;}return bag[bagAt++];}
    void spawn(uint32_t seed){piece=next;next=draw(seed);rotation=0;x=3;y=-1;if(!fits(x,y,rotation))over=true;}
    void reset(uint32_t seed){*this=Tetris();next=draw(seed);spawn(seed+1);}
    bool fits(int px,int py,int r)const{for(int cy=0;cy<4;cy++)for(int cx=0;cx<4;cx++)if(cell(piece,r,cx,cy)){int bx=px+cx,by=py+cy;if(bx<0||bx>=10||by>=20||(by>=0&&board[by][bx]))return false;}return true;}
    void move(int dx){if(!over&&fits(x+dx,y,rotation))x+=dx;}
    void rotate(){if(over)return;for(int offset:{0,-1,1,-2,2})if(fits(x+offset,y,(rotation+1)%4)){x+=offset;rotation=(rotation+1)%4;return;}if(fits(x,y-1,(rotation+1)%4)){y--;rotation=(rotation+1)%4;}}
    void lock(uint32_t seed){for(int cy=0;cy<4;cy++)for(int cx=0;cx<4;cx++)if(cell(piece,rotation,cx,cy)){int by=y+cy;if(by<0){over=true;return;}board[by][x+cx]=piece+1;}
        int cleared=0;for(int row=19;row>=0;row--){bool full=true;for(auto v:board[row])full&=v!=0;if(full){for(int r=row;r>0;r--)for(int c=0;c<10;c++)board[r][c]=board[r-1][c];for(auto& v:board[0])v=0;cleared++;row++;}}
        const int points[]={0,100,300,500,800};score+=points[cleared]*(1+lines/10);lines+=cleared;spawn(seed);}
    void down(uint32_t seed){if(over)return;if(fits(x,y+1,rotation))y++;else lock(seed);}
    void drop(uint32_t seed){if(over)return;while(fits(x,y+1,rotation)){y++;score+=2;}lock(seed);}
    int ghost()const{int row=y;while(fits(x,row+1,rotation))row++;return row;}
};
}
