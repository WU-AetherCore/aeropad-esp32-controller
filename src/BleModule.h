#pragma once
#include "Keys.h"
#include <NimBLEDevice.h>
#include <atomic>

class BleModule {
public:
    enum State { Idle,Scanning,Ready,Connecting,Connected,Unsupported,Failed,Disconnecting };
    struct Device { char name[249]={};char address[18]={};uint8_t type=0;int rssi=0;bool uart=false; };
    struct Snapshot {
        State state=Idle;int count=0;Device devices[12];char peer[249]={};char profile[12]={};
        uint32_t tx=0,rx=0,errors=0;char received[49]={};bool notifications=false;
        int period=50;int format=0;bool streaming=false;
        uint32_t baud=0;bool baudSupported=false;bool baudWritable=false;int baudResult=0;int errorCode=0;
        char stage[24]={};
        int connectionHandle=-1;uint32_t busySince=0;
    };
    void begin();
    void scan();
    void connect(int index);
    void disconnect();
    void stream(bool enabled);
    void update(const KVS& keys);
    void configure(int format,int period);
    Snapshot snapshot();
    void readBaud();
    void setBaud(uint32_t baud);
    void configureUsb(int mode);
    int usbMode();
    void flushUsb();
private:
    Snapshot _state;
    KVS _keys;
    portMUX_TYPE _lock=portMUX_INITIALIZER_UNLOCKED;
    std::atomic<int> _command{0};
    TaskHandle_t _task=nullptr;
    NimBLEClient* _client=nullptr;
    NimBLERemoteCharacteristic* _write=nullptr;
    NimBLERemoteCharacteristic* _baud=nullptr;
    uint8_t _seq=0;uint32_t _last=0;
    int _usbMode=0;
    uint8_t _usbRx[512]={};size_t _usbLength=0;uint32_t _usbDropped=0;
    static void task(void* self);
    static void guard(void* self);
    void run();
    bool send(bool neutral=false);
    void setState(State state);
    void close();
    void open(int index);
    void notify(uint8_t* data,size_t length);
    void refreshBaud();
    void stage(const char* text);
    void busy(bool active);
};
