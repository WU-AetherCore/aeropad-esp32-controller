#include "../src/LocalGames.h"
#include <cassert>
#include <cstdio>
int main(){
    LocalGames::Snake s;s.reset();s.turn(3);assert(s.next==1);for(int i=0;i<4;i++)s.step(0);assert(s.score==10&&s.length==5&&!s.occupied(s.food.x,s.food.y));
    s.reset();for(int i=0;i<20;i++)s.step(5);assert(s.over);s.reset();assert(!s.over&&s.score==0);
    s.length=4;s.dir=s.next=0;s.body[0]={2,2};s.body[1]={3,2};s.body[2]={3,1};s.body[3]={2,1};s.food={9,9};s.step(0);assert(!s.over&&s.body[0].y==1); // departing tail is legal
    s.length=5;s.dir=s.next=1;s.body[0]={2,2};s.body[1]={2,3};s.body[2]={3,3};s.body[3]={3,2};s.body[4]={4,2};s.step(0);assert(s.over);
    LocalGames::Breakout b;b.reset();b.move(-500);assert(b.paddle==38);b.move(999);assert(b.paddle==202);
    b.launched=true;b.x=16;b.vx=-90;b.step(.01);assert(b.vx>0);
    b.x=b.paddle;b.y=404;b.vy=100;b.step(.02);assert(b.vy<0);
    b.x=27;b.y=137;b.vy=-100;b.vx=0;b.step(.02);assert(b.bricks[0]==0&&b.score==10);
    for(int i=0;i<3;i++){b.launched=true;b.y=436;b.vy=100;b.step(.01);}assert(b.over&&b.lives==0);
    b.reset();for(auto& brick:b.bricks)brick=0;b.launched=true;b.step(.01);assert(b.level==2&&!b.launched);
    b.level=3;for(auto& brick:b.bricks)brick=0;b.launched=true;b.step(.01);assert(b.won&&b.over);
    LocalGames::Plane p;p.step(.02f,1,-1,17);assert(p.x>120&&p.y<390&&p.shots[0].active&&p.enemies[0].hp);
    p.reset();p.spawn=p.shoot=100;p.enemies[0]={120,200,0,100,1};p.shots[0]={120,203,true};p.step(.01f,0,0,0);assert(p.score==20&&!p.enemies[0].hp);
    p.reset();p.spawn=p.shoot=100;p.hostile[0]={120,389,true};p.step(.01f,0,0,0);assert(p.lives==2&&p.invulnerable>0);p.hit();assert(p.lives==2);
    p.invulnerable=0;p.hit();p.invulnerable=0;p.hit();assert(p.over&&p.lives==0);p.reset();assert(!p.over&&p.bombs==3&&p.score==0);
    p.enemies[0]={120,200,0,100,2};p.hostile[0]={120,300,true};p.bomb();assert(p.bombs==2&&!p.enemies[0].hp&&!p.hostile[0].active&&p.score==20);
    p.bomb();p.bomb();p.bomb();assert(p.bombs==0);for(int i=0;i<200;i++)p.step(.01f,1,1,0);assert(p.x<=214&&p.y<=418);
    puts("PASS: snake, breakout and plane movement/shooting/hits/invulnerability/bomb limits/reset.");
}
