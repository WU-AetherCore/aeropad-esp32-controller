#include "../src/NrfButtons.h"
#include <cassert>
#include "../src/NrfActionLatch.h"
#include <cstdio>
int main(){
 NrfButtons b;b.update(0,0);
 // Contact bounce must not trigger until a stable 12 ms press.
 b.update(1<<NrfButtons::B,1);b.update(0,4);b.update(1<<NrfButtons::B,7);
 b.update(1<<NrfButtons::B,18);assert(!b.pressed(NrfButtons::B));
 b.update(1<<NrfButtons::B,19);assert(b.pressed(NrfButtons::B));assert(b.pressed(NrfButtons::B));
 b.update(1<<NrfButtons::B,90);assert(!b.pressed(NrfButtons::B));
 // Release is sampled even on a page that never uses B's action.
 b.update(0,100);b.update(0,112);b.update(1<<NrfButtons::B,130);b.update(1<<NrfButtons::B,142);assert(b.pressed(NrfButtons::B));
 NrfButtons held;held.update(1<<NrfButtons::O,0);held.update(1<<NrfButtons::O,99);assert(!held.pressed(NrfButtons::O));
 held.update(0,100);held.update(0,112);held.update(1<<NrfButtons::O,113);held.update(1<<NrfButtons::O,125);assert(held.pressed(NrfButtons::O));
 NrfButtons wrap;wrap.update(0,0xfffffff0u);wrap.update(1<<NrfButtons::X,0xfffffff8u);wrap.update(1<<NrfButtons::X,4);assert(wrap.pressed(NrfButtons::X));
 // Eight simultaneous keys are independent; quick press/release sequences stay repeatable.
 NrfButtons all;all.update(0,0);for(unsigned i=0;i<100;i++){unsigned t=100+i*40;all.update(255,t);all.update(255,t+12);for(int k=0;k<8;k++)assert(all.pressed(NrfButtons::Key(k)));all.update(0,t+20);all.update(0,t+32);}
 NrfActionLatch latch;assert(!latch.update(false,0,false));
 assert(latch.update(true,1,true)); // Stop on first observed press, no debounce wait.
 assert(!latch.update(false,2,false));assert(!latch.update(true,15,false));
 assert(!latch.update(true,100,false)); // Release bounce cannot restart.
 assert(!latch.update(false,101,false));assert(!latch.update(false,141,false));
 assert(!latch.update(true,142,false));assert(latch.update(true,154,false));
 assert(!latch.update(true,200,true));
 puts("PASS: bounce, entry-held, cross-page release, short presses, simultaneous keys, clock wrap");
}
