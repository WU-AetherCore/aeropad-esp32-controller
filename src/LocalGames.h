#pragma once
#include <stdint.h>
#include <math.h>
#include <algorithm>
namespace LocalGames {
struct Snake {
    static constexpr int W=18,H=28,N=W*H;
    struct Cell{int x,y;};Cell body[N];int length=4,dir=1,next=1,score=0;Cell food{12,14};bool over=false,won=false;
    void reset(){length=4;dir=next=1;score=0;over=won=false;for(int i=0;i<4;i++)body[i]={8-i,14};food={12,14};}
    bool occupied(int x,int y)const{for(int i=0;i<length;i++)if(body[i].x==x&&body[i].y==y)return true;return false;}
    void spawn(uint32_t seed){int start=seed%N;for(int i=0;i<N;i++){int c=(start+i)%N;if(!occupied(c%W,c/W)){food={c%W,c/W};return;}}won=over=true;}
    void turn(int d){if(d>=0&&d<4&&(d+2)%4!=dir)next=d;}
    void step(uint32_t seed){if(over)return;dir=next;const int dx[]={0,1,0,-1},dy[]={-1,0,1,0};Cell h={body[0].x+dx[dir],body[0].y+dy[dir]};bool eat=h.x==food.x&&h.y==food.y;
        if(h.x<0||h.x>=W||h.y<0||h.y>=H){over=true;return;}
        for(int i=0;i<length-(eat?0:1);i++)if(h.x==body[i].x&&h.y==body[i].y){over=true;return;}
        if(eat&&length<N)length++;for(int i=length-1;i>0;i--)body[i]=body[i-1];body[0]=h;if(eat){score+=10;spawn(seed);}}
};
struct Breakout {
    float x=120,y=405,vx=90,vy=-160,paddle=120;int lives=3,score=0,level=1;uint8_t bricks[40];bool launched=false,over=false,won=false;
    void board(){for(int i=0;i<40;i++)bricks[i]=level>1&&i<16?2:1;serve();}
    void serve(){launched=false;x=paddle;y=405;vx=90;vy=-160-20*(level-1);}
    void reset(){lives=3;score=0;level=1;over=won=false;paddle=120;board();}
    void move(float center){paddle=std::max(38.0f,std::min(202.0f,center));if(!launched)x=paddle;}
    void step(float dt){if(over||!launched)return;dt=std::min(dt,.02f);float ox=x,oy=y;x+=vx*dt;y+=vy*dt;
        if(x<17){x=17;vx=fabsf(vx);}if(x>223){x=223;vx=-fabsf(vx);}if(y<105){y=105;vy=fabsf(vy);}
        if(vy>0&&oy<=405&&y>=405&&fabsf(x-paddle)<=30){y=405;float offset=(x-paddle)/30;vx=offset*(160+level*20);vy=-sqrtf(float((190+level*20)*(190+level*20))-vx*vx);}
        if(y>435){if(--lives==0)over=true;else serve();return;}
        for(int i=0;i<40;i++)if(bricks[i]){int bx=16+(i%8)*26,by=118+(i/8)*20;if(x+4>=bx&&x-4<=bx+23&&y+4>=by&&y-4<=by+15){bricks[i]--;score+=10;if(ox+4<bx||ox-4>bx+23)vx=-vx;else vy=-vy;x=ox;y=oy;break;}}
        bool any=false;for(auto b:bricks)any|=b!=0;if(!any){if(level==3)won=over=true;else{level++;board();}}
    }
};
struct Plane {
    struct Shot{float x,y;bool active;Shot(float px=0,float py=0,bool enabled=false):x(px),y(py),active(enabled){}};
    struct Enemy{float x,y,speed,fire;int hp;Enemy(float px=0,float py=0,float velocity=0,float timer=0,int health=0):x(px),y(py),speed(velocity),fire(timer),hp(health){}};
    Shot shots[24],hostile[20];Enemy enemies[12];float x=120,y=390,spawn=0,shoot=0,invulnerable=0;
    int score=0,lives=3,bombs=3,level=1;bool over=false;
    void reset(){*this=Plane();}
    void hit(){if(invulnerable>0||over)return;if(--lives<=0)over=true;invulnerable=1.4f;}
    void bomb(){if(over||bombs<=0)return;bombs--;for(auto& e:enemies)if(e.hp){score+=20;e.hp=0;}for(auto& b:hostile)b.active=false;}
    void step(float dt,float ax,float ay,uint32_t seed){if(over)return;dt=std::min(dt,.02f);level=1+score/200;invulnerable=std::max(0.0f,invulnerable-dt);
        x=std::max(26.0f,std::min(214.0f,x+ax*180*dt));y=std::max(120.0f,std::min(418.0f,y+ay*180*dt));
        shoot-=dt;if(shoot<=0){for(auto& b:shots)if(!b.active){b={x,y-14,true};break;}shoot=.15f;}
        spawn-=dt;if(spawn<=0){for(auto& e:enemies)if(!e.hp){e={float(26+seed%189),110,float(50+std::min(level,8)*8),1.0f,int(seed%5==0?2:1)};break;}spawn=std::max(.25f,.85f-level*.05f);}
        for(auto& b:shots)if(b.active){b.y-=300*dt;if(b.y<103)b.active=false;}
        for(auto& e:enemies)if(e.hp){e.y+=e.speed*dt;e.fire-=dt;if(e.fire<=0){for(auto& b:hostile)if(!b.active){b={e.x,e.y+12,true};break;}e.fire=1.4f;}
            for(auto& b:shots)if(b.active&&fabsf(b.x-e.x)<14&&fabsf(b.y-e.y)<15){b.active=false;if(--e.hp==0)score+=20;break;}
            if(e.hp&&fabsf(x-e.x)<20&&fabsf(y-e.y)<20){hit();e.hp=0;}
            if(e.y>435)e.hp=0;}
        for(auto& b:hostile)if(b.active){b.y+=(100+std::min(level,8)*8)*dt;if(b.y>434)b.active=false;else if(fabsf(b.x-x)<12&&fabsf(b.y-y)<13){b.active=false;hit();}}
    }
};
}
