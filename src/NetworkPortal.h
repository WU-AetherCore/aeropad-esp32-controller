#pragma once
#include <Arduino.h>
#include "Keys.h"
// Web and DNS run in a separate task so the portal survives screen navigation.
class WIFI {
public:
    void begin();
    void toggleHotspot();
    bool hotspot();
    bool autoConnect();
    void setAutoConnect(bool enabled);
    static void publish(const KVS& data);
};
