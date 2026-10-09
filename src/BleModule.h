#pragma once
#include "Keys.h"
#include <NimBLEDevice.h>
#include <atomic>
#include "BleControlProtocol.h"
#include "C30dFeedback.h"

class BleModule {
public:
    enum StopReason { NoStop,ManualStop,FeedbackStop,InputStop,WriteStop,LinkStop,ProtocolStop };
    enum State { Idle,Scanning,Ready,Connecting,Connected,Unsupported,Failed,Disconnecting };
    struct Device { char name[249]={};char address[18]={};uint8_t type=0;int rssi=0;bool uart=false; };
    struct Snapshot {
        State state=Idle;int count=0;Device devices[12];char peer[249]={};char profile[12]={};
        uint32_t tx=0,rx=0,errors=0,lastFeedback=0,lastCarFeedback=0;bool carFeedback=false,receiverZero=false,stopUnconfirmed=false;int feedbackKind=0,robotVx=0,robotVy=0,robotVz=0,batteryMv=0;char received[49]={};bool notifications=false;
        StopReason stopReason=NoStop;int period=50;int format=0;bool streaming=false;
        uint32_t baud=0;bool baudSupported=false;bool baudWritable=false;int baudResult=0;int errorCode=0;
        char stage[24]={};
        int wheelLeft=0,wheelRight=0,carSetSpeed=-1;int connectionHandle=-1;uint32_t busySince=0;
    };
    void begin();
    void pause();
    void scan();
    void connect(int index);
    void disconnect();
    void stream(bool enabled);
    void stop(StopReason reason);
    void update(const KVS& keys);
    void configure(int format,int period);
    bool protocol(const BleControl::Profile& profile,bool enabled);
    Snapshot snapshot();
    void readBaud();
    void setBaud(uint32_t baud);
    void configureUsb(int mode);
    int usbMode();
    void flushUsb();
    void writeUsb(const uint8_t* data,size_t length);
private:
    Snapshot _state;
    KVS _keys;
    portMUX_TYPE _lock=portMUX_INITIALIZER_UNLOCKED;
    std::atomic<int> _command{0};
    std::atomic<bool> _paused{true};
    std::atomic<uint32_t> _pauseAt{0};
    TaskHandle_t _task=nullptr;
    NimBLEClient* _client=nullptr;
    NimBLERemoteCharacteristic* _write=nullptr;
    NimBLERemoteCharacteristic* _baud=nullptr;
    uint16_t _seq=0;uint32_t _last=0;
    int _appSpeed=-1;uint32_t _lastSpeedSync=0,_lastConfirmed=0;
    C30dFeedback _feedbackParser;
    class ClientEvents : public NimBLEClientCallbacks {
      BleModule* owner;
    public:
      explicit ClientEvents(BleModule* p):owner(p){}
      void onDisconnect(NimBLEClient* client,int reason) override {
        if(client!=owner->_client)return;
        portENTER_CRITICAL(&owner->_lock);owner->_state.state=Ready;owner->_state.streaming=false;owner->_state.connectionHandle=-1;owner->_state.errorCode=reason;portEXIT_CRITICAL(&owner->_lock);
      }
    } _clientEvents{this};
    BleControl::Profile _protocol;
    bool _protocolActive=false;
    uint32_t _inputAt=0;
    std::atomic<bool> _protocolInit{true};
    std::atomic<int> _stopFrames{0};
    uint32_t _stopAt=0;
    std::atomic<bool> _protocolDisablePending{false};
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
