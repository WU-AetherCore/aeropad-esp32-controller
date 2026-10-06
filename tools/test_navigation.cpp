#include "../src/JoystickNavigation.h"
#include <cassert>
#include <cstdio>
int main() {
    using N=JoystickNavigation; N n;
    assert(n.update(100,0,0)==N::None);
    assert(n.update(0,0,10)==N::None);
    assert(n.update(54,0,20)==N::None);
    assert(n.update(80,0,30)==N::Right);
    assert(n.update(80,0,479)==N::None);
    assert(n.update(80,0,480)==N::Right);
    assert(n.update(80,0,659)==N::None);
    assert(n.update(80,0,660)==N::Right);
    assert(n.update(40,0,670)==N::None);
    assert(n.update(80,0,680)==N::None);
    n.update(0,0,700);
    assert(n.update(60,90,710)==N::Down);
    n.reset(); assert(n.update(0,-100,720)==N::None);
    n.update(0,0,730); assert(n.update(0,-100,740)==N::Up);
    n.update(0,0,750); assert(n.update(-100,0,760)==N::Left);
    puts("PASS: entry neutral, threshold, hysteresis, dominant axis, repeat, reset, four directions");
}
