#pragma once
#include <stdint.h>

// Local to each selection page; never changes the remote-control samples.
class JoystickNavigation {
public:
    enum Direction { None, Left, Right, Up, Down };
    void reset() { armed=false; held=None; }
    Direction update(int x,int y,uint32_t now) {
        int ax=x<0?-x:x, ay=y<0?-y:y;
        if(ax<=25 && ay<=25) { armed=true; held=None; return None; }
        if(!armed) return None;
        Direction next=None;
        if(ax>=55 || ay>=55) next=ax>ay?(x>0?Right:Left):(y>0?Down:Up);
        if(next==None) return None;
        if(next!=held) { held=next; since=now; interval=450; return next; }
        if(uint32_t(now-since)>=interval) { since=now; interval=180; return next; }
        return None;
    }
private:
    bool armed=false;
    Direction held=None;
    uint32_t since=0, interval=450;
};
