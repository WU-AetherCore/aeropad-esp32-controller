#pragma once
#include <Arduino.h>
#include "Keys.h"
// Web and DNS run in a separate task only while the WiFi management page is active.
class WIFI {
public:
    void begin();
    void end();
    void toggleHotspot();
    bool hotspot();
    bool autoConnect();
    void setAutoConnect(bool enabled);
    static void publish(const KVS& data);
};
