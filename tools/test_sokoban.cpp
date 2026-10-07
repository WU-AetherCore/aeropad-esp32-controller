#include "../src/Sokoban.h"
#include <cassert>
#include <cstdio>
#include <cstring>
#include "sokoban_solutions.h"
int direction(char c){return c=='U'?0:c=='R'?1:c=='D'?2:3;}
int main(){
 LocalGames::Sokoban g,expected;
 for(int level=0;level<g.COUNT;level++){
  g.reset(level);assert(!g.won()&&!g.stuck()&&strlen(g.map(level))==g.N);int n=0,people=0;for(int i=0;i<g.N;i++){n+=g.boxes[i];people+=g.map(level)[i]=='@'||g.map(level)[i]=='+';}assert(n==g.total()&&people==1);
  if(level)assert(LocalGames::sokobanMinimumPushes[level]>LocalGames::sokobanMinimumPushes[level-1]);
  const char* solution=sokobanSolutions[level];for(const char* p=solution;*p;p++)assert(g.move(direction(*p)));
  assert(g.won()&&g.pushes==LocalGames::sokobanMinimumPushes[level]);int moves=strlen(solution),undo=moves<g.HISTORY?moves:g.HISTORY;
  for(int i=0;i<undo;i++)g.undo();expected.reset(level);for(int i=0;i<moves-undo;i++)assert(expected.move(direction(solution[i])));
  assert(g.steps==expected.steps&&g.pushes==expected.pushes&&g.player==expected.player&&memcmp(g.boxes,expected.boxes,sizeof g.boxes)==0&&!g.undoReady);
  printf("PASS level %d: %d pushes, %d moves, undo %d\n",level+1,LocalGames::sokobanMinimumPushes[level],moves,undo);
 }
 g.reset(0);for(int i=0;i<g.N;i++){g.boxes[i]=false;g.terrain[i]=(i/g.W==0||i/g.W==7||i%g.W==0||i%g.W==7)?'#':' ';}g.terrain[54]='.';g.boxes[10]=true;g.player=11;
 assert(g.move(3)&&g.stuck());assert(!g.move(3));g.undo();assert(!g.stuck()&&g.steps==0);assert(g.neighbour(7,1)==-1&&g.neighbour(8,3)==-1);assert(!g.move(4));
 LocalGames::SokobanCelebration timer;timer.begin(100);assert(!timer.due(3299)&&timer.due(3300));timer.cancel();assert(!timer.due(4000));timer.begin(0xfffffff0u);assert(!timer.due(50)&&timer.due(4000));
 puts("PASS: 10 solvable increasing-push puzzles, bounded multi-undo, corner deadlock, invalid movement, timer and rollover.");
}
