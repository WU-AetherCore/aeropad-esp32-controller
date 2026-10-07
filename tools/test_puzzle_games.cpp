#include "../src/PuzzleGames.h"
#include <cassert>
#include <cstdio>
int main(){
 LocalGames::Merge2048 m;m.cells[0]=m.cells[1]=m.cells[2]=m.cells[3]=2;
 assert(m.move(3,7)&&m.cells[0]==4&&m.cells[1]==4&&m.score==8);m.undo();assert(m.score==0&&m.cells[3]==2&&!m.undoReady);
 m=LocalGames::Merge2048();m.cells[0]=2;assert(!m.move(3,7)&&m.cells[1]==0);assert(m.move(1,7)&&m.cells[3]==2);m.undo();assert(m.cells[0]==2);
 for(int i=0;i<16;i++)m.cells[i]=((i/4+i%4)%2)?2:4;assert(!m.available());m.move(0,1);assert(m.over);
 LocalGames::Tetris t;unsigned bits=0;for(int i=0;i<7;i++)bits|=1u<<t.draw(123);assert(bits==127);
 for(int p=0;p<7;p++)for(int r=0;r<4;r++){int n=0;for(int y=0;y<4;y++)for(int x=0;x<4;x++)n+=t.cell(p,r,x,y);assert(n==4);}
 t.reset(17);assert(t.fits(t.x,t.y,t.rotation));int g=t.ghost();assert(t.fits(t.x,g,t.rotation)&&!t.fits(t.x,g+1,t.rotation));t.drop(19);assert(t.score>0);
 t=LocalGames::Tetris();for(int c=0;c<10;c++)t.board[19][c]=1;t.piece=1;t.x=3;t.y=16;t.lock(1);assert(t.lines==1&&t.score==100);
 t.reset(1);for(int i=0;i<30;i++)t.move(-1);assert(t.fits(t.x,t.y,t.rotation));t.rotate();assert(t.fits(t.x,t.y,t.rotation));
 puts("PASS: 2048 merge-once/directions/undo/no-op/game-over; Tetris seven-bag/shapes/ghost/drop/line-clear/walls/rotation.");
}
