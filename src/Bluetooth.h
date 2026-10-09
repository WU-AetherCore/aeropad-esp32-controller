#pragma once
#include <Arduino.h>
#include <BleGamepad.h>
#include "Keys.h"

class Bluetooth {
public:
    Bluetooth();
    void begin(bool advertise=true);
    bool connected();
    void update(const KVS &state);
    void releaseAll();
    void suspendGamepad();
private:
    BleGamepad _gamepad;
    bool _started = false,_serverReady=false;
    uint32_t _lastReport=0;
};
