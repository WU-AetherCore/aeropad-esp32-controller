#pragma once
#include <stdint.h>
// Sample every key even when its action is inactive on the current page.
// Entry-held keys must be released first; repeated consumers see the same edge.
class NrfButtons {
public:
 enum Key { Up, Down, Left, Right, O, X, A, B };
 void update(uint8_t pressedMask,uint32_t now){
  edges=0;
  if(!initialized){initialized=true;raw=stable=pressedMask;for(int i=0;i<8;i++)changed[i]=now;return;}
  for(int i=0;i<8;i++){
   uint8_t bit=1u<<i;
   if((raw^pressedMask)&bit){raw^=bit;changed[i]=now;}
   if(((stable^raw)&bit)&&uint32_t(now-changed[i])>=12){stable^=bit;if(stable&bit)edges|=bit;}
  }
 }
 template<class S> void sample(const S& s,uint32_t now){
  const bool values[]={!s.up,!s.down,!s.left,!s.right,!s.o,!s.x,!s.a,!s.b};uint8_t bits=0;
  for(int i=0;i<8;i++)if(values[i])bits|=1u<<i;update(bits,now);
 }
 bool pressed(Key key)const{return edges&(1u<<key);}
private:
 uint8_t raw=0,stable=0,edges=0;bool initialized=false;uint32_t changed[8]={};
};
