#include <Arduino.h>
#include <NimBLEDevice.h>
#include <Preferences.h>
#include <atomic>
#include "../../../src/ControlPacket.h"
// Receiver-board wiring only: connect RX to target TX, TX to target RX and GND.
constexpr int UART_RX=18,UART_TX=17;
constexpr const char* SERVICE="6e400001-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* UUID_RX="6e400002-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* UUID_TX="6e400003-b5a3-f393-e0a9-e50e24dcca9e";
constexpr const char* BAUD="6e400004-b5a3-f393-e0a9-e50e24dcca9e";
struct Data {uint16_t length;uint8_t bytes[192];};
QueueHandle_t queue;NimBLECharacteristic *reply,*baudCharacteristic;
std::atomic<uint32_t> lastReceived{0};std::atomic<int> wireFormat{ControlPacket::Binary};
struct NeutralInputs {
 int LX=0,LY=0,RX=0,RY=0,L_knob=0,R_knob=0,angleX=0,angleY=0;
 bool a=true,b=true,x=true,o=true,L_up=true,L_down=true,R_up=true,R_down=true,board_L=true,board_R=true;
 bool up=true,down=true,left=true,right=true,switch_L1=true,switch_L2=true,switch_R1=true,switch_R2=true;
};
uint32_t actualBaud=115200;std::atomic<uint32_t> requestedBaud{0},dropped{0};
bool validBaud(uint32_t b){return b==9600||b==19200||b==38400||b==57600||b==115200;}
void publishBaud(){uint8_t b[]={uint8_t(actualBaud),uint8_t(actualBaud>>8),uint8_t(actualBaud>>16),uint8_t(actualBaud>>24)};baudCharacteristic->setValue(b,4);}
class RxCallbacks:public NimBLECharacteristicCallbacks {
 void onWrite(NimBLECharacteristic* c,NimBLEConnInfo&)override {
  auto v=c->getValue();if(v.size()>192)return;
  Data d;d.length=v.size();memcpy(d.bytes,v.data(),d.length);
  if(d.length&&d.bytes[0]==0xa5)wireFormat.store(ControlPacket::Binary);
  else if(d.length&&d.bytes[0]=='{')wireFormat.store(ControlPacket::Json);
  else if(d.length>=3&&d.bytes[0]=='A'&&d.bytes[1]=='P'&&d.bytes[2]=='1')wireFormat.store(ControlPacket::Text);
  else if(d.length>=3&&d.bytes[0]=='A'&&d.bytes[1]=='5'&&d.bytes[2]==' ')wireFormat.store(ControlPacket::Hex);
  lastReceived.store(millis());
  if(xQueueSend(queue,&d,0)!=pdTRUE)dropped++;
 }
};
class BaudCallbacks:public NimBLECharacteristicCallbacks {
 void onWrite(NimBLECharacteristic* c,NimBLEConnInfo&)override {
  auto v=c->getValue();if(v.size()==4){const uint8_t* b=v.data();uint32_t rate=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);if(validBaud(rate))requestedBaud.store(rate);}
  publishBaud(); // Readback remains the actual rate until the UART changes.
 }
};
RxCallbacks rxCallbacks;BaudCallbacks baudCallbacks;
void setup(){
 Serial.begin(115200);Preferences prefs;prefs.begin("uart",true);actualBaud=prefs.getUInt("baud",115200);prefs.end();if(!validBaud(actualBaud))actualBaud=115200;
 Serial1.setTxBufferSize(512);Serial1.begin(actualBaud,SERIAL_8N1,UART_RX,UART_TX);queue=xQueueCreate(16,sizeof(Data));
 NimBLEDevice::init("AeroPad UART Receiver");NimBLEDevice::setMTU(185);
 auto server=NimBLEDevice::createServer();server->advertiseOnDisconnect(true);auto service=server->createService(SERVICE);
 auto input=service->createCharacteristic(UUID_RX,NIMBLE_PROPERTY::WRITE|NIMBLE_PROPERTY::WRITE_NR);input->setCallbacks(&rxCallbacks);
 reply=service->createCharacteristic(UUID_TX,NIMBLE_PROPERTY::NOTIFY);
 baudCharacteristic=service->createCharacteristic(BAUD,NIMBLE_PROPERTY::READ|NIMBLE_PROPERTY::WRITE);baudCharacteristic->setCallbacks(&baudCallbacks);publishBaud();
 server->start();auto advertising=NimBLEDevice::getAdvertising();advertising->addServiceUUID(SERVICE);advertising->enableScanResponse(true);advertising->setName("AeroPad UART Receiver");advertising->start();
 Serial.printf("Receiver ready: UART1 RX=%d TX=%d baud=%u\n",UART_RX,UART_TX,actualBaud);
}
void loop(){
 uint32_t requested=requestedBaud.exchange(0);if(requested){Serial1.flush();Serial1.updateBaudRate(requested);actualBaud=requested;Preferences prefs;prefs.begin("uart",false);prefs.putUInt("baud",actualBaud);prefs.end();publishBaud();Serial.printf("UART baud applied: %u\n",actualBaud);}
 static Data pending;static bool waiting=false;static bool stopped=true;
 uint32_t last=lastReceived.load();
 if(last&&millis()-last<5000)stopped=false;
 if(last&&millis()-last>=5000&&!stopped) {
  xQueueReset(queue);waiting=true;memset(&pending,0,sizeof(pending));
  // A newline discards any interrupted ASCII frame before the neutral frame.
  size_t prefix=wireFormat.load()==ControlPacket::Binary?0:1;if(prefix)pending.bytes[0]='\n';
  pending.length=prefix+ControlPacket::encode(NeutralInputs{},0,wireFormat.load(),pending.bytes+prefix,sizeof(pending.bytes)-prefix,true);
  stopped=true;
 }
 if(!waiting)waiting=xQueueReceive(queue,&pending,0)==pdTRUE;
 if(waiting&&Serial1.availableForWrite()>=pending.length){Serial1.write(pending.bytes,pending.length);Serial.write(pending.bytes,pending.length);waiting=false;}
 if(Serial1.available()&&NimBLEDevice::getServer()->getConnectedCount()) {
  uint8_t b[20];size_t n=0;while(n<sizeof(b)&&Serial1.available())b[n++]=Serial1.read();reply->setValue(b,n);reply->notify();
 }
 delay(1);
}
