#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "ReceiverConfig.h"
#include "../../../src/NrfControlProtocol.h"
#ifndef RECEIVER_MODE
#define RECEIVER_MODE 2
#endif
static_assert(RECEIVER_MODE==1||RECEIVER_MODE==2,"Select drone or car");
SPIClass bus(HSPI);RF24 radio(NRF_CE,NRF_CSN);NrfControl::Receiver receiver(RECEIVER_MODE);
bool ready=false;uint32_t lastSbus=0,lastLog=0;int previousLeft=0,previousRight=0;bool previousEnabled=false;uint8_t report[32];
void motor(int in1,int in2,int pwm,int value,bool reverse){
 if(reverse)value=-value;
 // Break before changing direction; never set both bridge direction inputs high.
 ledcWrite(pwm,0);digitalWrite(in1,value>0);digitalWrite(in2,value<0);ledcWrite(pwm,abs(value)*255/100);
}
void outputs(){
 if(RECEIVER_MODE==1){
  if(millis()-lastSbus>=14){lastSbus=millis();uint8_t frame[25];NrfControl::sbus(receiver,frame);Serial1.write(frame,25);}
 }else{
  int left=0,right=0;if(receiver.armed&&!receiver.failsafe)NrfControl::mix(-receiver.axes[1],receiver.axes[2],receiver.limit,left,right);
  if(MOTOR_OUTPUT_ENABLED){
   bool enabled=receiver.armed&&!receiver.failsafe;
   if(left==previousLeft&&right==previousRight&&enabled==previousEnabled)return;
   bool reversing=(left*previousLeft<0)||(right*previousRight<0);
   digitalWrite(MOTOR_STBY,LOW);if(reversing)delayMicroseconds(100);
   motor(LEFT_IN1,LEFT_IN2,0,left,LEFT_REVERSE);motor(RIGHT_IN1,RIGHT_IN2,1,right,RIGHT_REVERSE);
   digitalWrite(MOTOR_STBY,receiver.armed&&!receiver.failsafe);previousLeft=left;previousRight=right;previousEnabled=enabled;
  }
 }
}
void setup(){
 Serial.begin(115200);
 if(RECEIVER_MODE==1)Serial1.begin(100000,SERIAL_8E2,-1,SBUS_TX,true);
 else if(MOTOR_OUTPUT_ENABLED){
  pinMode(MOTOR_STBY,OUTPUT);digitalWrite(MOTOR_STBY,LOW);
  for(int pin:{LEFT_IN1,LEFT_IN2,RIGHT_IN1,RIGHT_IN2}){pinMode(pin,OUTPUT);digitalWrite(pin,LOW);}
  ledcSetup(0,20000,8);ledcSetup(1,20000,8);ledcAttachPin(LEFT_PWM,0);ledcAttachPin(RIGHT_PWM,1);ledcWrite(0,0);ledcWrite(1,0);
 }
 receiver.stop();bus.begin(NRF_SCK,NRF_MISO,NRF_MOSI);ready=radio.begin(&bus);
 if(!ready){Serial.println("[NRF RX] module missing; outputs locked");return;}
 radio.setChannel(NRF_CHANNEL);radio.setDataRate(RF24_1MBPS);radio.setPALevel(NRF_POWER);
 radio.setAddressWidth(5);radio.setPayloadSize(32);radio.setCRCLength(RF24_CRC_16);radio.setAutoAck(true);radio.setRetries(0,3);radio.enableAckPayload();
 radio.openReadingPipe(1,reinterpret_cast<const uint8_t*>(RECEIVER_MODE==1?"NDRN1":"NCAR1"));
 receiver.feedback(report);radio.writeAckPayload(1,report,32);radio.startListening();
 Serial.printf("[NRF RX] mode=%d motor_output=%d timeout=250ms\n",RECEIVER_MODE,MOTOR_OUTPUT_ENABLED);
}
void loop(){
 uint32_t now=millis();receiver.tick(now);
 if(ready){
  for(int i=0;i<3&&radio.available();i++){
   uint8_t n=radio.getDynamicPayloadSize(),packet[32];
   if(n==32){radio.read(packet,32);receiver.input(packet,millis());}
   else if(n&&n<=32){radio.read(packet,n);receiver.rejected++;}else radio.flush_rx();
   receiver.feedback(report);radio.flush_tx();radio.writeAckPayload(1,report,32);
  }
 }
 receiver.tick(millis());outputs();
 if(now-lastLog>=500){lastLog=now;Serial.printf("[NRF RX] armed=%d failsafe=%d accepted=%lu rejected=%lu throttle=%d LX=%d LY=%d RX=%d RY=%d\n",receiver.armed,receiver.failsafe,(unsigned long)receiver.accepted,(unsigned long)receiver.rejected,receiver.armed?NrfControl::throttle(receiver.axes[4]):0,receiver.axes[0],receiver.axes[1],receiver.axes[2],receiver.axes[3]);}
 delay(1);
}
