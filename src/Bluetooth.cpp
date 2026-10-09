#include "Bluetooth.h"
#include <NimBLEDevice.h>
Bluetooth::Bluetooth() : _gamepad("AeroPad BLE Gamepad", "AeroPad", 100) {}
void Bluetooth::begin(bool advertise) {
    if(_started) {
        auto server=NimBLEDevice::getServer();
        if(server){server->advertiseOnDisconnect(advertise);if(advertise)NimBLEDevice::getAdvertising()->start();else NimBLEDevice::getAdvertising()->stop();}
        return;
    }
    BleGamepadConfiguration config;
    config.setAxesMin(-32767);config.setAxesMax(32767);
    config.setWhichAxes(true,true,false,true,true,false,true,true);
    config.setButtonCount(14);config.setHatSwitchCount(1);config.setAutoReport(false);
    _gamepad.begin(&config);_started=true;
    if(!advertise)suspendGamepad();
    Serial.println("[BLE] advertising: signed full-range axes, 14 buttons, D-pad");
}
bool Bluetooth::connected() {
    // HID library flags wait for authentication and can become stale. UI follows
    // the authoritative peripheral connection count in the BLE host.
    auto server=NimBLEDevice::getServer();
    return _started && server && server->getConnectedCount()>0;
}
void Bluetooth::update(const KVS &s) {
    if(!connected() || millis()-_lastReport<10)return;
    _lastReport=millis();
    auto axis=[](int value)->int16_t{return constrain(value,-100,100)*32767/100;};
    _gamepad.setAxes(axis(s.LX),axis(s.LY),0,axis(s.RX),axis(s.RY),0,axis(s.L_knob),axis(s.R_knob));
    const bool pressed[]={!s.a,!s.b,!s.x,!s.o,!s.L_up,!s.L_down,!s.R_up,!s.R_down,!s.board_L,!s.board_R,!s.switch_L1,!s.switch_L2,!s.switch_R1,!s.switch_R2};
    for(int i=0;i<14;i++){if(pressed[i])_gamepad.press(i+1);else _gamepad.release(i+1);}
    int dx=(!s.right)-(!s.left),dy=(!s.down)-(!s.up);
    int hat=dy<0?(dx<0?8:dx>0?2:1):dy>0?(dx<0?6:dx>0?4:5):dx<0?7:dx>0?3:0;
    _gamepad.setHat(hat);_gamepad.sendReport();
}
void Bluetooth::releaseAll() {
    if(!_started||!connected())return;
    _gamepad.resetButtons();_gamepad.setAxes();_gamepad.setHat(0);_gamepad.sendReport();
}
void Bluetooth::suspendGamepad() {
    if(!_started)return;
    uint32_t start=millis();
    while(millis()-start<3000) {
        auto server=NimBLEDevice::getServer();
        if(server&&(_serverReady||NimBLEDevice::getAdvertising()->isAdvertising()||server->getConnectedCount()))break;
        delay(10);
    }
    auto server=NimBLEDevice::getServer();if(!server)return;_serverReady=true;
    releaseAll();server->advertiseOnDisconnect(false);NimBLEDevice::getAdvertising()->stop();
    for(auto handle:server->getPeerDevices())server->disconnect(handle);
    start=millis();while(server->getConnectedCount()&&millis()-start<1000)delay(10);
}
