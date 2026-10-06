#pragma once
#include <cmath>
namespace CubeGeometry {
struct Point { float x,y,z; };
inline float wrap(float angle) {
    return std::remainder(angle,360.0f);
}
inline Point rotate(Point p,float roll,float pitch,float yaw) {
    constexpr float rad=0.01745329252f;
    float cx=std::cos(roll*rad),sx=std::sin(roll*rad);
    float cy=std::cos(pitch*rad),sy=std::sin(pitch*rad);
    float cz=std::cos(yaw*rad),sz=std::sin(yaw*rad);
    Point a={p.x,p.y*cx-p.z*sx,p.y*sx+p.z*cx};
    Point b={a.x*cy+a.z*sy,a.y,-a.x*sy+a.z*cy};
    return {b.x*cz-b.y*sz,b.x*sz+b.y*cz,b.z};
}
inline Point project(Point p) {
    float scale=170.0f/(3.8f+p.z);
    return {120+p.x*scale,210-p.y*scale,p.z};
}
}
