#pragma once
#include <stdint.h>
// B+X held together for 350 ms. Entry-held keys cannot cause an exit.
class NrfExitChord {
public:
 bool update(bool b,bool x,uint32_t now){
  if(!ready){if(!b&&!x)ready=true;return false;}
  if(!b||!x){holding=false;return false;}
  if(!holding){holding=true;since=now;}
  if(uint32_t(now-since)<350)return false;
  ready=false;holding=false;return true;
 }
private:
 bool ready=false,holding=false;uint32_t since=0;
};
