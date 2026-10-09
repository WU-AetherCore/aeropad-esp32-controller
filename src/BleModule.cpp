#include "BleModule.h"
#include "ControlPacket.h"
#include <Preferences.h>

void BleModule::begin() {
    _command=0;_paused=false;if(_task)return;
    Preferences prefs;prefs.begin("blemodule",true);
    int format=prefs.getInt("format",prefs.getBool("json",false)?1:0);_state.format=format>=0&&format<=3?format:0;
    int usb=prefs.getInt("usb",0);_usbMode=usb>=0&&usb<=4?usb:0;
    int period=prefs.getInt("period",50);_state.period=ControlPacket::validPeriod(period)?period:50;
    prefs.end();
    xTaskCreate(task,"ble_module",24576,this,1,&_task);
    xTaskCreate(guard,"ble_guard",8192,this,1,nullptr);
}
void BleModule::task(void* self){static_cast<BleModule*>(self)->run();}
void BleModule::guard(void* self) {
    auto module=static_cast<BleModule*>(self);
    for(;;) {
        auto s=module->snapshot();
        bool timeout=s.busySince&&millis()-s.busySince>uint32_t(s.streaming?2000:20000);
        int pending=module->_command.load();
        bool cancel=(pending==2&&s.state==Connecting)||(pending==9&&millis()-module->_pauseAt.load()>500);
        if(timeout||cancel) {
            if(s.connectionHandle>=0)ble_gap_terminate(s.connectionHandle,BLE_ERR_REM_USER_CONN_TERM);
            else if(s.state==Connecting)ble_gap_conn_cancel();
        }
        vTaskDelay(pdMS_TO_TICKS(25));
    }
}
BleModule::Snapshot BleModule::snapshot(){portENTER_CRITICAL(&_lock);Snapshot s=_state;portEXIT_CRITICAL(&_lock);return s;}
void BleModule::setState(State s){portENTER_CRITICAL(&_lock);_state.state=s;portEXIT_CRITICAL(&_lock);}
void BleModule::stage(const char* text){portENTER_CRITICAL(&_lock);snprintf(_state.stage,sizeof(_state.stage),"%s",text);portEXIT_CRITICAL(&_lock);}
void BleModule::busy(bool active){portENTER_CRITICAL(&_lock);_state.busySince=active?millis():0;portEXIT_CRITICAL(&_lock);}
void BleModule::scan(){_command.store(1);}
void BleModule::connect(int index){if(index>=0&&index<12)_command.store(100+index);}
void BleModule::disconnect(){_command.store(2);}
void BleModule::readBaud(){_command.store(3);}
void BleModule::setBaud(uint32_t baud){if(baud==9600||baud==19200||baud==38400||baud==57600||baud==115200)_command.store(200000+baud);}
void BleModule::stream(bool enabled){
 portENTER_CRITICAL(&_lock);bool previous=_state.streaming;
 if(enabled&&(_state.state!=Connected||_stopFrames.load()>0||_state.stopUnconfirmed))enabled=false;
 if(enabled&&_protocolActive&&_protocol.kind!=BleControl::Custom&&(!_state.carFeedback||millis()-_state.lastCarFeedback>1000)){enabled=false;_state.stopReason=FeedbackStop;}
 if(enabled&&_protocolActive&&_protocol.kind!=BleControl::Custom&&((_state.feedbackKind==C30dFeedback::RosState&&_protocol.kind<=BleControl::AppSteering)||(_state.feedbackKind==C30dFeedback::AppWheels&&_protocol.kind==BleControl::RosVelocity))){enabled=false;_state.stopReason=ProtocolStop;}
 if(enabled)_state.stopReason=NoStop;else if(previous)_state.stopReason=ManualStop;
 _state.streaming=enabled;
 if(!enabled&&_state.state==Connected&&(previous||_protocolActive)&&_stopFrames.load()==0){_stopAt=millis();_stopFrames=20;_state.stopUnconfirmed=previous||!_state.receiverZero||millis()-_state.lastCarFeedback>200;}
 portEXIT_CRITICAL(&_lock);if(!previous&&enabled)_protocolInit=true;
}
void BleModule::stop(StopReason reason){
 stream(false);portENTER_CRITICAL(&_lock);_state.stopReason=reason;portEXIT_CRITICAL(&_lock);
 auto status=snapshot();Serial.printf("[CONTROL STOP] reason=%d inputAge=%u feedbackAge=%u RX=%u TX=%u ERR=%u heap=%u\n",int(reason),millis()-_inputAt,status.carFeedback?millis()-status.lastCarFeedback:0,status.rx,status.tx,status.errors,ESP.getFreeHeap());
}
void BleModule::update(const KVS& keys){portENTER_CRITICAL(&_lock);_keys=keys;_inputAt=millis();portEXIT_CRITICAL(&_lock);}
bool BleModule::protocol(const BleControl::Profile& profile,bool enabled){
    if(enabled&&!BleControl::valid(profile))return false;
    if(enabled)_protocolDisablePending.store(false);
    if(!enabled&&_stopFrames.load()>0){_protocolDisablePending.store(true);return false;}
    if(!enabled&&snapshot().state==Disconnecting){_protocolDisablePending.store(true);return false;}
    portENTER_CRITICAL(&_lock);if(_state.streaming){portEXIT_CRITICAL(&_lock);return false;}_protocol=profile;_protocolActive=enabled;portEXIT_CRITICAL(&_lock);_protocolInit.store(true);return true;
}
void BleModule::configure(int format,int period) {
    if(format<0||format>3||!ControlPacket::validPeriod(period))return;
    portENTER_CRITICAL(&_lock);_state.format=format;_state.period=period;portEXIT_CRITICAL(&_lock);
    Preferences prefs;prefs.begin("blemodule",false);prefs.putInt("format",format);prefs.putInt("period",period);prefs.end();
}
void BleModule::notify(uint8_t* data,size_t length) {
    char hex[49]={};size_t count=min(size_t(16),length);
    for(size_t i=0;i<count;i++)snprintf(hex+i*3,4,"%02X ",data[i]);
    char preview[49]={}; int previewMode;
    portENTER_CRITICAL(&_lock);previewMode=_usbMode;_state.rx+=length;_state.lastFeedback=millis();
    for(size_t i=0;i<length;i++){
        C30dFeedback::Frame frame;if(!_feedbackParser.feed(data[i],frame))continue;
        if(frame.kind==C30dFeedback::AppParameters){_state.carSetSpeed=frame.speed;continue;}
        _state.carFeedback=true;_state.feedbackKind=frame.kind;_state.receiverZero=frame.zero;_state.lastCarFeedback=millis();
        if(frame.kind==C30dFeedback::AppWheels){_state.wheelLeft=frame.left;_state.wheelRight=frame.right;}
        else{_state.robotVx=frame.vx;_state.robotVy=frame.vy;_state.robotVz=frame.vz;_state.batteryMv=frame.battery;}
    }
    if(previewMode==3){size_t p=0;for(size_t i=0;i<length&&p<sizeof(preview)-1;i++){uint8_t c=data[i];if(c>=32&&c<=126)preview[p++]=char(c);else if(p+4<sizeof(preview)){snprintf(preview+p,sizeof(preview)-p,"\\x%02X",c);p+=4;}}}
    else if(previewMode==4)snprintf(preview,sizeof(preview),"JSON len=%u",unsigned(length));
    else memcpy(preview,hex,sizeof(preview));
    memcpy(_state.received,preview,sizeof(preview));
    if(_usbMode){size_t n=min(length,sizeof(_usbRx)-_usbLength);memcpy(_usbRx+_usbLength,data,n);_usbLength+=n;_usbDropped+=length-n;}
    portEXIT_CRITICAL(&_lock);
}
int BleModule::usbMode(){portENTER_CRITICAL(&_lock);int m=_usbMode;portEXIT_CRITICAL(&_lock);return m;}
void BleModule::writeUsb(const uint8_t* data,size_t length){
    if(!_client||!_client->isConnected()||!_write||!length)return;
    size_t chunk=min(size_t(180),size_t(max(23,int(_client->getMTU()))-3));
    for(size_t off=0;off<length;off+=chunk)_write->writeValue(data+off,min(chunk,length-off),!_write->canWriteNoResponse());
}
void BleModule::configureUsb(int mode){if(mode<0||mode>4)return;portENTER_CRITICAL(&_lock);_usbMode=mode;_usbLength=0;_usbDropped=0;portEXIT_CRITICAL(&_lock);Preferences p;p.begin("blemodule",false);p.putInt("usb",mode);p.end();}
void BleModule::flushUsb(){
    uint8_t bytes[512];size_t n;int mode;uint32_t dropped;
    portENTER_CRITICAL(&_lock);n=_usbLength;mode=_usbMode;dropped=_usbDropped;memcpy(bytes,_usbRx,n);_usbLength=0;_usbDropped=0;portEXIT_CRITICAL(&_lock);
    if(n){
        if(mode==1){Serial.write(bytes,n);}
        else if(mode==2){Serial.print("[BLE RX HEX] ");for(size_t i=0;i<n;i++)Serial.printf("%02X ",bytes[i]);Serial.println();}
        else if(mode==3){Serial.print("[BLE RX TEXT] ");for(size_t i=0;i<n;i++){uint8_t c=bytes[i];if((c>=32&&c<=126)||c=='\r'||c=='\n'||c=='\t')Serial.write(c);else Serial.printf("\\x%02X",c);}Serial.println();}
        else if(mode==4){Serial.print("{\"len\":");Serial.print(n);Serial.print(",\"hex\":\"");for(size_t i=0;i<n;i++)Serial.printf("%02X",bytes[i]);Serial.print("\",\"text\":\"");for(size_t i=0;i<n;i++){uint8_t c=bytes[i];if(c=='\\'||c=='\"')Serial.printf("\\\\%c",c);else if(c>=32&&c<=126)Serial.write(c);else Serial.printf("\\u%04X",c);}Serial.println("\"}");}
    }
    if(dropped)Serial.printf("[BLE RX] USB queue dropped %u bytes\n",dropped);
}
bool BleModule::send(bool neutral) {
    if(!_client||!_client->isConnected()||!_write)return false;
    KVS s;int format;bool custom;BleControl::Profile profile;int feedbackKind;
    portENTER_CRITICAL(&_lock);s=_keys;format=_state.format;custom=_protocolActive;profile=_protocol;feedbackKind=_state.feedbackKind;portEXIT_CRITICAL(&_lock);
    if(neutral&&custom&&profile.kind!=BleControl::Custom){
        if(feedbackKind==C30dFeedback::RosState)profile.kind=BleControl::RosVelocity;
        else if(feedbackKind==C30dFeedback::AppWheels&&profile.kind==BleControl::RosVelocity)profile.kind=BleControl::AppDirection;
    }
    bool setup=custom&&!neutral&&_protocolInit.load();
    int target=neutral?0:BleControl::appSpeed(profile,s);
    bool speedUpdate=neutral||setup||(target!=_appSpeed&&(target==0||millis()-_lastSpeedSync>=100))||millis()-_lastSpeedSync>=1000;
    uint8_t data[192];size_t length=custom?BleControl::encode(profile,s,_seq++,data,sizeof(data),neutral,setup,speedUpdate):ControlPacket::encode(s,_seq++,format,data,sizeof(data),neutral);
    if(!length)return false;
    if(neutral&&custom&&profile.kind<=BleControl::AppSteering){
        const uint8_t clear[]={0,'K',0,'I',0};memmove(data+sizeof(clear),data,length);memcpy(data,clear,sizeof(clear));length+=sizeof(clear);
    }
    size_t chunk=min(size_t(180),size_t(max(23,int(_client->getMTU()))-3));
    bool confirmed=_write->canWrite()&&(neutral||setup||millis()-_lastConfirmed>=500);
    busy(true);
    for(size_t offset=0;offset<length;offset+=chunk) {
        if(!_client->isConnected()||!_write->writeValue(data+offset,min(chunk,length-offset),confirmed||!_write->canWriteNoResponse())) {
            busy(false);portENTER_CRITICAL(&_lock);_state.errors++;portEXIT_CRITICAL(&_lock);return false;
        }
    }
    if(setup)_protocolInit.store(false);
    if(custom&&speedUpdate){_lastSpeedSync=millis();_appSpeed=target;}
    if(confirmed)_lastConfirmed=millis();
    busy(false);portENTER_CRITICAL(&_lock);_state.tx++;portEXIT_CRITICAL(&_lock);return true;
}
void BleModule::close() {
    if(_client&&_client->isConnected()){
        bool custom;portENTER_CRITICAL(&_lock);custom=_protocolActive;portEXIT_CRITICAL(&_lock);
        if(custom){
            uint32_t start=millis();bool verified=false;
            do{send(true);vTaskDelay(pdMS_TO_TICKS(50));auto status=snapshot();verified=status.carFeedback&&status.lastCarFeedback>=start&&status.receiverZero;}while(_client->isConnected()&&!verified&&millis()-start<1500);
            Serial.printf("[STOP] verified=%d (fresh receiver zero-speed feedback)\n",verified);
        }else send(true);
        _client->disconnect();
    }
    _write=nullptr;_baud=nullptr;
    _stopFrames.store(0);
    if(_protocolDisablePending.exchange(false)){portENTER_CRITICAL(&_lock);_protocolActive=false;portEXIT_CRITICAL(&_lock);}
    portENTER_CRITICAL(&_lock);_state.connectionHandle=-1;_state.busySince=0;portEXIT_CRITICAL(&_lock);
    if(_client){NimBLEDevice::deleteClient(_client);_client=nullptr;}
    portENTER_CRITICAL(&_lock);_state.streaming=false;_state.notifications=false;_state.baudSupported=false;_state.baudWritable=false;_state.baud=0;portEXIT_CRITICAL(&_lock);
}
void BleModule::open(int index) {
    Snapshot s=snapshot();if(index>=s.count)return;
    close();NimBLEDevice::getScan()->stop();
    setState(Connecting);
    stage("radio connect");
    _client=NimBLEDevice::createClient();
    if(!_client){setState(Failed);return;}
    _client->setClientCallbacks(&_clientEvents,false);
    _client->setConnectTimeout(3000);_client->setConnectRetries(1);
    _client->setConnectionParams(24,40,0,400);
    busy(true);
    if(!_client->connect(NimBLEAddress(s.devices[index].address,s.devices[index].type),true,false,false)) {
        int error=_client->getLastError();close();portENTER_CRITICAL(&_lock);_state.errorCode=error;portEXIT_CRITICAL(&_lock);setState(Failed);return;
    }
    busy(false);portENTER_CRITICAL(&_lock);_state.connectionHandle=_client->getConnHandle();_state.errorCode=0;portEXIT_CRITICAL(&_lock);
    if(NimBLEDevice::isBonded(_client->getPeerAddress())) {
        stage("restore encryption");
        if(_client->secureConnection(true)) {
            uint32_t start=millis();
            while(_client->isConnected()&&!_client->getConnInfo().isEncrypted()&&millis()-start<4000)vTaskDelay(pdMS_TO_TICKS(20));
        }
    }
    struct Profile {const char* service;const char* tx;const char* rx;const char* name;};
    const Profile profiles[]={
        {"6e400001-b5a3-f393-e0a9-e50e24dcca9e","6e400002-b5a3-f393-e0a9-e50e24dcca9e","6e400003-b5a3-f393-e0a9-e50e24dcca9e","NUS"},
        {"ffe0","ffe1","ffe1","FFE0"}
    };
    bool receive=false;const char* profile=nullptr;
    for(const auto& p:profiles) {
        stage("discover UART");busy(true);
        auto service=_client->getService(NimBLEUUID(p.service));busy(false);
        if(!service)continue;
        stage("discover channels");busy(true);
        auto writer=service->getCharacteristic(NimBLEUUID(p.tx));
        auto reader=strcmp(p.tx,p.rx)==0?writer:service->getCharacteristic(NimBLEUUID(p.rx));
        if(strcmp(p.name,"NUS")==0)_baud=service->getCharacteristic(NimBLEUUID("6e400004-b5a3-f393-e0a9-e50e24dcca9e"));
        busy(false);
        if(!writer||(!writer->canWrite()&&!writer->canWriteNoResponse()))continue;
        _write=writer;profile=p.name;
        stage("subscribe feedback");
        busy(true);
        if(reader&&(reader->canNotify()||reader->canIndicate()))receive=reader->subscribe(reader->canNotify(),[this](NimBLERemoteCharacteristic*,uint8_t* data,size_t length,bool){notify(data,length);});
        busy(false);
        break;
    }
    if(!_write||!_client->isConnected()){int error=_client->getLastError();bool linked=_client->isConnected();close();portENTER_CRITICAL(&_lock);_state.errorCode=error;portEXIT_CRITICAL(&_lock);setState(linked&&error==0?Unsupported:Failed);return;}
    portENTER_CRITICAL(&_lock);
    snprintf(_state.peer,sizeof(_state.peer),"%s",s.devices[index].name[0]?s.devices[index].name:s.devices[index].address);
    snprintf(_state.profile,sizeof(_state.profile),"%s",profile);
    _state.lastFeedback=millis();_state.carFeedback=false;_state.feedbackKind=0;_state.receiverZero=false;_state.stopUnconfirmed=false;_state.lastCarFeedback=0;_state.stopReason=NoStop;_feedbackParser.reset();_lastSpeedSync=_lastConfirmed=0;_appSpeed=-1;
    _state.notifications=receive;_state.tx=0;_state.rx=0;_state.errors=0;_state.received[0]=0;
    _state.state=Connected;_state.streaming=false;
    portEXIT_CRITICAL(&_lock);
    Serial.printf("[BLE UART] write=%d noResponse=%d notify=%d\n",_write->canWrite(),_write->canWriteNoResponse(),receive);
    stage("read UART settings");_last=millis();refreshBaud();stage("ready");
}
void BleModule::refreshBaud() {
    uint32_t baud=0;bool supported=false;
    if(_client&&_client->isConnected()&&_baud&&_baud->canRead()) {
        busy(true);
        auto value=_baud->readValue();
        busy(false);
        if(value.size()==4) {
            const uint8_t* b=value.data();baud=uint32_t(b[0])|(uint32_t(b[1])<<8)|(uint32_t(b[2])<<16)|(uint32_t(b[3])<<24);
            supported=baud==9600||baud==19200||baud==38400||baud==57600||baud==115200;
        }
    }
    portENTER_CRITICAL(&_lock);_state.baud=supported?baud:0;_state.baudSupported=supported;_state.baudWritable=supported&&_baud&&_baud->canWrite();_state.baudResult=supported?1:-1;portEXIT_CRITICAL(&_lock);
}
void BleModule::run() {
    // Gamepad owns BLE initialization; do not deinitialize its HID server.
    while(!NimBLEDevice::getServer())vTaskDelay(pdMS_TO_TICKS(20));
    auto scanner=NimBLEDevice::getScan();
    scanner->setActiveScan(true);scanner->setInterval(100);scanner->setWindow(60);scanner->setMaxResults(32);
    bool wasStreaming=false;
    for(;;) {
        int command=_command.exchange(0);
        if(command==9){scanner->stop();stream(false);close();setState(Idle);busy(false);_paused=true;}
        if(_paused){vTaskDelay(pdMS_TO_TICKS(10));continue;}
        if(command==1) {
            scanner->stop();close();scanner->clearResults();
            portENTER_CRITICAL(&_lock);_state.count=0;portEXIT_CRITICAL(&_lock);
            setState(Scanning);if(!scanner->start(4000))setState(Failed);
        } else if(command==2){scanner->stop();setState(Disconnecting);close();setState(Ready);}
        else if(command==3)refreshBaud();
        else if(command>=200000) {
            uint32_t baud=command-200000;uint8_t b[]={uint8_t(baud),uint8_t(baud>>8),uint8_t(baud>>16),uint8_t(baud>>24)};
            busy(true);bool ok=_client&&_client->isConnected()&&_baud&&snapshot().baudWritable&&_baud->writeValue(b,4,true);busy(false);
            if(ok)for(int attempt=0;attempt<5;attempt++){vTaskDelay(pdMS_TO_TICKS(50));refreshBaud();if(snapshot().baud==baud)break;}
            portENTER_CRITICAL(&_lock);_state.baudResult=ok&&_state.baud==baud?2:-2;portEXIT_CRITICAL(&_lock);
        }
        else if(command>=100)open(command-100);
        Snapshot s=snapshot();
        if(s.state==Scanning&&!scanner->isScanning()) {
            auto results=scanner->getResults();Snapshot fresh=s;fresh.count=0;
            for(int i=0;i<results.getCount()&&fresh.count<12;i++) {
                auto d=results.getDevice(i);if(!d->isConnectable())continue;
                auto& target=fresh.devices[fresh.count++];
                snprintf(target.address,sizeof(target.address),"%s",d->getAddress().toString().c_str());target.type=d->getAddressType();target.rssi=d->getRSSI();
                auto name=d->getName();size_t j=0;
                for(unsigned char c:name){if(j>=sizeof(target.name)-1)break;target.name[j++]=(c>=32&&c<=126)?c:'?';}target.name[j]=0;
                target.uart=d->isAdvertisingService(NimBLEUUID("ffe0"))||d->isAdvertisingService(NimBLEUUID("6e400001-b5a3-f393-e0a9-e50e24dcca9e"));
            }
            // Known UART services first, then stronger radio signal.
            for(int i=0;i<fresh.count;i++)for(int j=i+1;j<fresh.count;j++)if(fresh.devices[j].uart>fresh.devices[i].uart||(fresh.devices[j].uart==fresh.devices[i].uart&&fresh.devices[j].rssi>fresh.devices[i].rssi)){Device t=fresh.devices[i];fresh.devices[i]=fresh.devices[j];fresh.devices[j]=t;}
            // Scanning must not overwrite format/period changed concurrently in a settings page.
            portENTER_CRITICAL(&_lock);_state.count=fresh.count;memcpy(_state.devices,fresh.devices,sizeof(fresh.devices));_state.state=Ready;portEXIT_CRITICAL(&_lock);
        }
        if(_client&&snapshot().state!=Connected&&!_client->isConnected()){close();wasStreaming=false;}
        if(_client&&snapshot().state==Connected) {
            if(!_client->isConnected()){close();setState(Ready);}
            else {
                bool streaming=snapshot().streaming;bool custom;uint32_t inputAt;int period;
                portENTER_CRITICAL(&_lock);custom=_protocolActive;inputAt=_inputAt;period=custom?ControlPacket::periods[_protocol.interval]:_state.period;if(custom&&_protocol.kind<=BleControl::AppSteering)period=max(50,period);portEXIT_CRITICAL(&_lock);
                if(custom&&streaming&&uint32_t(millis()-inputAt)>500){stop(InputStop);streaming=false;}
                auto health=snapshot();
                if(custom&&streaming&&health.carFeedback&&uint32_t(millis()-health.lastCarFeedback)>1000){stop(FeedbackStop);stage("feedback lost; STOP");streaming=false;}
                if(_stopFrames.load()>0&&millis()-_last>=50){
                    bool sent=send(true);_last=millis();auto feedback=snapshot();
                    bool verified=feedback.carFeedback&&feedback.lastCarFeedback>=_stopAt&&feedback.receiverZero;
                    if(verified){_stopFrames=0;portENTER_CRITICAL(&_lock);_state.stopUnconfirmed=false;portEXIT_CRITICAL(&_lock);Serial.println("[STOP] verified=1");}
                    else if(sent&&_stopFrames.fetch_sub(1)==1){_stopFrames=5;stage("STOP unconfirmed");}
                }
                else if(streaming&&millis()-_last>=uint32_t(period)){_last=millis();if(!send()){stop(WriteStop);close();setState(Failed);stage("write failed");}}
                else if(wasStreaming&&!streaming)send(true);
                wasStreaming=streaming;
            }
        } else wasStreaming=false;
        if(_stopFrames.load()==0&&snapshot().state!=Disconnecting&&_protocolDisablePending.exchange(false)){portENTER_CRITICAL(&_lock);_protocolActive=false;portEXIT_CRITICAL(&_lock);}
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}

void BleModule::pause(){if(!_task)return;if(!NimBLEDevice::getServer()){_paused=true;_command=0;return;}stream(false);_pauseAt=millis();_paused=false;_command=9;uint32_t at=millis();while(!_paused&&millis()-at<3000)delay(5);}
