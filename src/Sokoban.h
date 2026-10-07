#pragma once
#include <stdint.h>
#include "SokobanLevels.h"
namespace LocalGames {
struct SokobanCelebration {
    static const uint32_t Duration=3200;
    uint32_t started=0;bool active=false;
    void begin(uint32_t now){started=now;active=true;}
    void cancel(){active=false;}
    bool due(uint32_t now)const{return active&&uint32_t(now-started)>=Duration;}
};
// # wall, . goal, $ box, @ player, * box on goal, + player on goal.
struct Sokoban {
    static const int W=8,H=8,N=W*H,COUNT=10,HISTORY=128;
    char terrain[N];bool boxes[N];int player=0,steps=0,pushes=0,level=0,historyCount=0,historyHead=0;bool undoReady=false;
    struct Snapshot {uint64_t boxes;int steps,pushes;uint8_t player;};
    Snapshot history[HISTORY];
    static const char* map(int n){return sokobanMaps[n>=0&&n<COUNT?n:0];}
    void reset(int n){level=n>=0&&n<COUNT?n:0;steps=pushes=historyCount=historyHead=0;undoReady=false;const char* s=map(level);
        for(int i=0;i<N;i++){terrain[i]=s[i]=='#'?'#':s[i]=='.'||s[i]=='*'||s[i]=='+'?'.':' ';boxes[i]=s[i]=='$'||s[i]=='*';if(s[i]=='@'||s[i]=='+')player=i;}}
    int neighbour(int i,int d)const{if(d<0||d>3)return -1;const int delta[]={-W,1,W,-1};int j=i+delta[d];return j>=0&&j<N&&((d%2==0)||j/W==i/W)?j:-1;}
    bool free(int i)const{return i>=0&&i<N&&terrain[i]!='#'&&!boxes[i];}
    bool won()const{for(int i=0;i<N;i++)if(terrain[i]=='.'&&!boxes[i])return false;return true;}
    int placed()const{int n=0;for(int i=0;i<N;i++)n+=terrain[i]=='.'&&boxes[i];return n;}
    int total()const{int n=0;for(char c:terrain)n+=c=='.';return n;}
    bool corner(int i)const{if(!boxes[i]||terrain[i]=='.')return false;bool wall[4];for(int d=0;d<4;d++){int j=neighbour(i,d);wall[d]=j<0||terrain[j]=='#';}return (wall[0]||wall[2])&&(wall[1]||wall[3]);}
    bool stuck()const{for(int i=0;i<N;i++)if(corner(i))return true;return false;}
    bool move(int d){if(d<0||d>3||won())return false;int to=neighbour(player,d);if(to<0||terrain[to]=='#')return false;int dest=neighbour(to,d);if(boxes[to]&&!free(dest))return false;
        auto& s=history[historyHead];s.boxes=0;for(int i=0;i<N;i++)if(boxes[i])s.boxes|=uint64_t(1)<<i;s.player=player;s.steps=steps;s.pushes=pushes;
        historyHead=(historyHead+1)%HISTORY;if(historyCount<HISTORY)historyCount++;undoReady=true;
        if(boxes[to]){boxes[to]=false;boxes[dest]=true;pushes++;}player=to;steps++;return true;}
    void undo(){if(!historyCount)return;historyHead=(historyHead+HISTORY-1)%HISTORY;auto& s=history[historyHead];for(int i=0;i<N;i++)boxes[i]=(s.boxes>>i)&1;player=s.player;steps=s.steps;pushes=s.pushes;historyCount--;undoReady=historyCount>0;}
};
}
