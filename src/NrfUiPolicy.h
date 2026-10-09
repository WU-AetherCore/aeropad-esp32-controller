#pragma once
#include <stdint.h>
namespace NrfUiPolicy {
// A single short B is a control input, never a control-screen navigation event.
class HoldB {
public:
 bool update(bool down,bool x,uint32_t now){
  if(!ready){if(!down)ready=true;return false;}
  if(!down||x){timing=false;return false;}
  if(!timing){timing=true;start=now;}
  if(uint32_t(now-start)<700)return false;
  ready=false;timing=false;return true;
 }
private:
 bool ready=false,timing=false;uint32_t start=0;
};
inline bool standardControl(int page){return page<2;}
inline int standardNext(int page,bool active){
 if(active)return page==0?1:0; // display-only switch; arm state and radio unchanged
 if(page==0)return 1;
 if(page==1)return 3; // stopped control -> configuration menu
 return page==2?3:page==3?4:0;
}
inline bool canConfigure(bool armed,bool arming,bool sending){return !armed&&!arming&&!sending;}
}
