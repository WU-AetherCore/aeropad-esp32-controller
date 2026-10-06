#pragma once
#include <TFT_eSPI.h>
namespace UiLayout {
constexpr int joystickSize=101;
constexpr int joystickLeft=10;
constexpr int joystickRight=129;
constexpr int joystickCenter=50;
constexpr int joystickTravel=39;
inline void joystick(TFT_eSprite &sprite,int x,int y,int vx,int vy,bool crosshair=true) {
    sprite.fillRect(x+1,y+1,joystickSize-2,joystickSize-2,TFT_BLACK);
    sprite.drawRect(x,y,joystickSize,joystickSize,TFT_CYAN);
    if(crosshair){sprite.drawFastHLine(x+6,y+joystickCenter,joystickSize-12,TFT_DARKGREY);sprite.drawFastVLine(x+joystickCenter,y+6,joystickSize-12,TFT_DARKGREY);}
    sprite.fillCircle(x+joystickCenter+constrain(vx,-100,100)*joystickTravel/100,
                      y+joystickCenter+constrain(vy,-100,100)*joystickTravel/100,8,TFT_CYAN);
}
}
