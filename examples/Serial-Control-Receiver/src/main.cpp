#include <Arduino.h>
#include "../../common/ControlReceiver.h"
constexpr int UART_RX=18,UART_TX=17;
ControlReceiver::Stream parser;ControlReceiver::Watchdog watchdog;
int lastLeft=999,lastRight=999;
bool stopped=true,haveSequence=false;uint8_t lastSequence=0;
// Replace this one function with the receiver's actual motor driver.
// Until adapted, this example only prints intended outputs and drives no motor.
void applyOutputs(int left,int right){
    if(left==lastLeft&&right==lastRight)return;
    lastLeft=left;lastRight=right;Serial.printf("motor intent left=%d%% right=%d%%\n",left,right);
}
void setup(){Serial.begin(115200);Serial1.begin(115200,SERIAL_8N1,UART_RX,UART_TX);applyOutputs(0,0);}
void loop(){
    ControlReceiver::Frame f;
    while(Serial1.available())if(parser.feed(Serial1.read(),f)){
        if(f.neutral)applyOutputs(0,0);
        if(haveSequence&&lastSequence==f.seq)continue;
        haveSequence=true;lastSequence=f.seq;stopped=false;
        watchdog.accepted(millis());int left,right;ControlReceiver::differential(f,left,right);applyOutputs(left,right);
    }
    if(watchdog.expired(millis(),300)){applyOutputs(0,0);if(!stopped){parser.reset();haveSequence=false;stopped=true;}}
    delay(1);
}
