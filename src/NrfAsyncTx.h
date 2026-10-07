#pragma once
#include <RF24.h>
// Single packet in flight. Hardware handles retries while the UI keeps polling keys.
class NrfAsyncTx {
public:
 bool busy()const{return active;}
 void begin(RF24& radio,const uint8_t* bytes,uint8_t length,uint32_t now){
  if(active)return;
  radio.stopListening();radio.setPayloadSize(length);bool ok,fail,rx;radio.whatHappened(ok,fail,rx);
  radio.startWrite(bytes,length,false);active=true;started=now;
 }
 int poll(RF24& radio,uint32_t now,uint8_t receiveLength){
  if(!active)return 0;
  bool ok,fail,rx;radio.whatHappened(ok,fail,rx);
  if(!ok&&!fail&&uint32_t(now-started)<120)return 0;
  radio.stopListening();if(!ok)radio.flush_tx();radio.setPayloadSize(receiveLength);radio.startListening();active=false;
  return ok?1:-1;
 }
 void cancel(RF24& radio,uint8_t receiveLength){
  if(!active)return;radio.stopListening();radio.flush_tx();bool ok,fail,rx;radio.whatHappened(ok,fail,rx);
  radio.setPayloadSize(receiveLength);radio.startListening();active=false;
 }
private:
 bool active=false;uint32_t started=0;
};
