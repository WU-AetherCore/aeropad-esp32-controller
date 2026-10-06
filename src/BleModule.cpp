#include "BleModule.h"
#include "ControlPacket.h"
#include <Preferences.h>

void BleModule::begin() {
    if(_task)return;
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
        bool timeout=s.busySince&&millis()-s.busySince>8000;
        bool cancel=module->_command.load()==2&&s.state==Connecting;
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
void BleModule::stream(bool enabled){portENTER_CRITICAL(&_lock);_state.streaming=enabled;portEXIT_CRITICAL(&_lock);}
void BleModule::update(const KVS& keys){portENTER_CRITICAL(&_lock);_keys=keys;portEXIT_CRITICAL(&_lock);}
void BleModule::configure(int format,int period) {
    if(format<0||format>3||!ControlPacket::validPeriod(period))return;
    portENTER_CRITICAL(&_lock);_state.format=format;_state.period=period;portEXIT_CRITICAL(&_lock);
    Preferences prefs;prefs.begin("blemodule",false);prefs.putInt("format",format);prefs.putInt("period",period);prefs.end();
}
void BleModule::notify(uint8_t* data,size_t length) {
    char hex[49]={};size_t count=min(size_t(16),length);
    for(size_t i=0;i<count;i++)snprintf(hex+i*3,4,"%02X ",data[i]);
    char preview[49]={}; int previewMode;
    portENTER_CRITICAL(&_lock);previewMode=_usbMode;_state.rx+=length;
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
    KVS s;int format;
    portENTER_CRITICAL(&_lock);s=_keys;format=_state.format;portEXIT_CRITICAL(&_lock);
    uint8_t data[192];size_t length=ControlPacket::encode(s,_seq++,format,data,sizeof(data),neutral);
    if(!length)return false;
    size_t chunk=min(size_t(180),size_t(max(23,int(_client->getMTU()))-3));
    busy(true);
    for(size_t offset=0;offset<length;offset+=chunk) {
        if(!_client->isConnected()||!_write->writeValue(data+offset,min(chunk,length-offset),!_write->canWriteNoResponse())) {
            busy(false);portENTER_CRITICAL(&_lock);_state.errors++;portEXIT_CRITICAL(&_lock);return false;
        }
    }
    busy(false);portENTER_CRITICAL(&_lock);_state.tx++;portEXIT_CRITICAL(&_lock);return true;
}
void BleModule::close() {
    if(_client&&_client->isConnected()){send(true);_client->disconnect();}
    _write=nullptr;_baud=nullptr;
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
    _client->setConnectTimeout(5000);_client->setConnectRetries(0);
    _client->setConnectionParams(12,24,0,200);
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
    stage("discover services");
    busy(true);
    const auto& services=_client->getServices(true);
    busy(false);
    for(const auto& p:profiles) {
        NimBLERemoteService* service=nullptr;
        for(auto found:services)if(found->getUUID()==NimBLEUUID(p.service))service=found;
        if(!service)continue;
        stage("discover channels");
        busy(true);
        const auto& chars=service->getCharacteristics(true);
        busy(false);
        NimBLERemoteCharacteristic *writer=nullptr,*reader=nullptr;
        for(auto c:chars){if(c->getUUID()==NimBLEUUID(p.tx))writer=c;if(c->getUUID()==NimBLEUUID(p.rx))reader=c;if(strcmp(p.name,"NUS")==0&&c->getUUID()==NimBLEUUID("6e400004-b5a3-f393-e0a9-e50e24dcca9e"))_baud=c;}
        if(!writer||(!writer->canWrite()&&!writer->canWriteNoResponse()))continue;
        _write=writer;profile=p.name;
        stage("subscribe feedback");
        busy(true);
        if(reader&&(reader->canNotify()||reader->canIndicate()))receive=reader->subscribe(reader->canNotify(),[this](NimBLERemoteCharacteristic*,uint8_t* data,size_t length,bool){notify(data,length);});
        busy(false);
        break;
    }
    if(!_write){int error=_client->getLastError();bool linked=_client->isConnected();close();portENTER_CRITICAL(&_lock);_state.errorCode=error;portEXIT_CRITICAL(&_lock);setState(linked&&error==0?Unsupported:Failed);return;}
    portENTER_CRITICAL(&_lock);
    snprintf(_state.peer,sizeof(_state.peer),"%s",s.devices[index].name[0]?s.devices[index].name:s.devices[index].address);
    snprintf(_state.profile,sizeof(_state.profile),"%s",profile);
    _state.notifications=receive;_state.tx=0;_state.rx=0;_state.errors=0;_state.received[0]=0;
    _state.state=Connected;_state.streaming=false;
    portEXIT_CRITICAL(&_lock);
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
        if(_client&&snapshot().state==Connected) {
            if(!_client->isConnected()){close();setState(Ready);}
            else {
                bool streaming=snapshot().streaming;
                if(streaming&&millis()-_last>=uint32_t(snapshot().period)){_last=millis();send();}
                else if(wasStreaming&&!streaming)send(true);
                wasStreaming=streaming;
            }
        } else wasStreaming=false;
        vTaskDelay(pdMS_TO_TICKS(5));
    }
}
