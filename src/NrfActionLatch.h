#pragma once
#include <stdint.h>
// Start needs a stable press; stopping is urgent. Release bounce cannot restart TX.
class NrfActionLatch {
public:
 bool update(bool down,uint32_t now,bool urgent){
  if(!initialized){initialized=true;previous=down;armed=!down;changed=now;return false;}
  if(down!=previous){previous=down;changed=now;}
  if(!down){if(uint32_t(now-changed)>=40)armed=true;return false;}
  if(armed&&(urgent||uint32_t(now-changed)>=12)){armed=false;return true;}
  return false;
 }
private:
 bool initialized=false,previous=false,armed=false;uint32_t changed=0;
};
