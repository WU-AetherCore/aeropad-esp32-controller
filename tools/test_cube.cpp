#include "../src/CubeGeometry.h"
#include <cassert>
#include <cstdio>
#include <initializer_list>
int main() {
    using namespace CubeGeometry;
    assert(wrap(359)==-1 && wrap(-359)==1);
    for(int r=-180;r<=180;r+=30)for(int p=-180;p<=180;p+=30)for(int y=-180;y<=180;y+=30)
        for(int x:{-1,1})for(int yy:{-1,1})for(int z:{-1,1}) {
            Point v=rotate({float(x),float(yy),float(z)},r,p,y);
            assert(std::fabs(v.x*v.x+v.y*v.y+v.z*v.z-3)<0.0001f);
            auto q=project(rotate(v,22,-28,0));
            assert(std::isfinite(q.x)&&std::isfinite(q.y));
            assert(q.x>20 && q.x<220 && q.y>110 && q.y<310);
        }
    puts("PASS: 17576 projected vertices stay in cube viewport; rotation length and angle wrapping valid");
}
