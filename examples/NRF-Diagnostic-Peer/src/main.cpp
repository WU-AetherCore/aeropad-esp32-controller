#include <Arduino.h>
#include <SPI.h>
#include <RF24.h>
#include "../../../src/NrfDebugPacket.h"
// Separate ESP32-S3 peer: change these pins to match your wiring.
SPIClass bus(HSPI);RF24 radio(43,42);
void setup(){
 Serial.begin(115200);bus.begin(12,13,11);
 if(!radio.begin(&bus)){Serial.println("NRF not detected");return;}
 radio.setChannel(76);radio.setDataRate(RF24_1MBPS);radio.setPALevel(RF24_PA_MIN);
 radio.setAddressWidth(5);radio.setPayloadSize(32);radio.setAutoAck(true);radio.setRetries(1,3);radio.setCRCLength(RF24_CRC_16);
 radio.openReadingPipe(1,reinterpret_cast<const uint8_t*>("ADBG1"));
 radio.openWritingPipe(reinterpret_cast<const uint8_t*>("ADBG0"));radio.startListening();
}
void loop(){
 if(radio.available()){
  uint8_t packet[32];radio.read(packet,32);
  if(NrfDebugPacket::valid(packet)){
   Serial.printf("NDG1 seq=%lu LX=%d LY=%d RX=%d RY=%d\n",(unsigned long)NrfDebugPacket::get32(packet+4),int8_t(packet[12]),int8_t(packet[13]),int8_t(packet[14]),int8_t(packet[15]));
   // Return the exact diagnostic payload. No motors or actuator output.
   radio.stopListening();radio.write(packet,32);radio.startListening();
  }
 }
 delay(1);
}
