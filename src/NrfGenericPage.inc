// Configurable raw NRF24L01 interoperability tool. Never interprets arbitrary RX as actuator feedback.
void nrfGeneric(){
 using namespace NrfGeneric;Config cfg;Preferences prefs;
 if(prefs.begin("nrfRaw",true)){Config stored;if(prefs.getBytesLength("config")==sizeof(stored)&&prefs.getBytes("config",&stored,sizeof(stored))==sizeof(stored)&&valid(stored))cfg=stored;prefs.end();}
 auto& radio=nrf.radio;NrfButtons buttons;NrfActionLatch oLatch;NrfAsyncTx asyncTx;auto oldRate=radio.getDataRate();auto oldCrc=radio.getCRCLength();uint8_t oldChannel=radio.getChannel(),oldPower=radio.getPALevel(),oldSize=radio.getPayloadSize();
 const rf24_datarate_e rates[]={RF24_1MBPS,RF24_2MBPS,RF24_250KBPS};const uint16_t periods[]={20,50,100,200,250,500,1000,1500,2000};
 bool present=radio.isChipConnected(),running=false,savePending=false,saveOk=true;uint32_t changed=0,paint=0,lastTx=0,lastCheck=0,tx=0,ok=0,rx=0,bad=0,lastRx=0;
 int page=0,field=0,presetIndex=0;uint8_t received[32]={},receivedLength=0;JoystickNavigation nav;UiRefresh refresh;
 auto apply=[&](){
  running=false;if(!present)return;asyncTx.cancel(radio,cfg.rxLength);
  radio.stopListening();radio.flush_tx();radio.flush_rx();radio.disableAckPayload();radio.disableDynamicPayloads();
  for(int i=0;i<6;i++)radio.closeReadingPipe(i);
  radio.setAddressWidth(cfg.width);radio.setChannel(cfg.channel);radio.setPALevel(cfg.power);
  if(!radio.setDataRate(rates[cfg.rate])){cfg.rate=0;radio.setDataRate(RF24_1MBPS);}
  radio.setAutoAck(cfg.ack);if(cfg.crc==0)radio.disableCRC();else radio.setCRCLength(cfg.crc==1?RF24_CRC_8:RF24_CRC_16);
  radio.setRetries(cfg.retryDelay,cfg.retryCount);radio.setPayloadSize(cfg.rxLength);if(cfg.dynamic)radio.enableDynamicPayloads();
  radio.openWritingPipe(cfg.txAddress);radio.openReadingPipe(1,cfg.rxAddress);radio.startListening();
 };
 auto save=[&](){saveOk=false;if(prefs.begin("nrfRaw",false)){saveOk=prefs.putBytes("config",&cfg,sizeof(cfg))==sizeof(cfg);prefs.end();}savePending=!saveOk;changed=millis();};
 auto changedConfig=[&](){savePending=true;changed=millis();apply();refresh.invalidate();};
 apply();screen.spr.unloadFont();screen.spr.resetViewport();screen.spr.setTextSize(1);screen.spr.setTextDatum(TC_DATUM);
 auto label=[&](const uint8_t* b,int y,uint16_t color=TFT_WHITE){screen.spr.drawBitmap(10,y,b,220,36,color);};
 auto value=[&](const String& s,int y,uint16_t color=TFT_CYAN,int font=4){screen.spr.setTextColor(color,TFT_BLACK);screen.spr.drawString(s,120,y,font);};
 auto hex=[&](const uint8_t* bytes,int start,int count){String s;for(int i=0;i<count;i++){char b[4];snprintf(b,sizeof(b),"%02X",bytes[start+i]);if(i)s+=' ';s+=b;}return s;};
 auto samplePacket=[&](uint8_t* bytes){memset(bytes,0,32);bytes[0]=axis(keys.kvs.LX);bytes[1]=axis(keys.kvs.LY);bytes[2]=axis(keys.kvs.RX);bytes[3]=axis(keys.kvs.RY);bytes[4]=axis(keys.kvs.L_knob);bytes[5]=axis(keys.kvs.R_knob);uint32_t mask=ControlPacket::buttons(keys.kvs);bytes[6]=mask;bytes[7]=mask>>8;bytes[8]=mask>>16;};
 String line;bool collecting=false,stream=true;
 while(true){
  if(present&&asyncTx.poll(radio,millis(),cfg.rxLength)==1)ok++;
  keys.kvs_update();buttons.sample(keys.kvs,millis());char c=Serial.available()?Serial.read():0;
  // @ followed by up to 32 hex bytes and newline updates manual TX contents atomically.
  if(collecting){if(c=='\n'||c=='\r'){uint8_t bytes[32];int n=0,high=-1;bool good=true;for(unsigned i=0;i<line.length();i++){char ch=line[i];if(ch==' '||ch=='\t')continue;int v=ch>='0'&&ch<='9'?ch-'0':ch>='a'&&ch<='f'?ch-'a'+10:ch>='A'&&ch<='F'?ch-'A'+10:-1;if(v<0){good=false;break;}if(high<0)high=v;else{if(n>=32){good=false;break;}bytes[n++]=(high<<4)|v;high=-1;}}if(good&&n&&high<0){memset(cfg.payload,0,32);memcpy(cfg.payload,bytes,n);cfg.txLength=n;cfg.source=0;changedConfig();Serial.printf("[NRFRAW] payload accepted length=%d\n",n);}else Serial.println("[NRFRAW] invalid hex payload");collecting=false;line="";}else if(c){if(line.length()<96)line+=c;else{collecting=false;line="";Serial.println("[NRFRAW] input too long");}}c=0;
  }else if(c=='@'){collecting=true;line="";c=0;}
  auto move=nav.update(keys.kvs.LX,keys.kvs.LY,millis());
  if(buttons.pressed(NrfButtons::X)||c=='Q')break;
  if(c=='S')uiScreenshot(true);if(c=='V')stream=!stream;
  if(buttons.pressed(NrfButtons::B)||c=='B'){page=(page+1)%9;field=0;nav.reset();refresh.invalidate();}
  if(buttons.pressed(NrfButtons::A)||c=='A'){if(page==0){if(present)asyncTx.cancel(radio,cfg.rxLength);tx=ok=rx=bad=0;}else if(page==6){cfg.source=1-cfg.source;changedConfig();}}
  if(oLatch.update(!keys.kvs.o,millis(),running)||c=='E'){if(running){running=false;if(present)asyncTx.cancel(radio,cfg.rxLength);}else if(page==7){preset(cfg,presetIndex);changedConfig();}else if(present){running=true;lastTx=millis()-periods[cfg.interval];}paint=millis()-100;refresh.invalidate();}
  int count=page==4||page==5?cfg.width:page==6?32:page==7?3:page==8?8:4;
  if(page){if(buttons.pressed(NrfButtons::Up)||c=='U'||move==JoystickNavigation::Up){field=(field+count-1)%count;refresh.invalidate();}if(buttons.pressed(NrfButtons::Down)||c=='D'||move==JoystickNavigation::Down){field=(field+1)%count;refresh.invalidate();}}
  int d=(buttons.pressed(NrfButtons::Right)||c=='R'||move==JoystickNavigation::Right)?1:(buttons.pressed(NrfButtons::Left)||c=='L'||move==JoystickNavigation::Left)?-1:0;
  if(page==7)presetIndex=field;
  if(d&&page>0&&page<7&&!(page==6&&cfg.source)){
   auto cycle=[&](uint8_t& v,int low,int high){v=low+(int(v)-low+d+high-low+1)%(high-low+1);};
   if(page==1){uint8_t* vars[]={&cfg.channel,&cfg.rate,&cfg.power,&cfg.width};int lows[]={0,0,0,3},highs[]={83,2,3,5};cycle(*vars[field],lows[field],highs[field]);}
   if(page==2){uint8_t* vars[]={&cfg.crc,&cfg.ack,&cfg.dynamic,&cfg.rxLength};int lows[]={0,0,0,1},highs[]={2,1,1,32};cycle(*vars[field],lows[field],highs[field]);if(cfg.ack&&cfg.crc==0)cfg.crc=1;}
   if(page==3){uint8_t* vars[]={&cfg.txLength,&cfg.retryDelay,&cfg.retryCount,&cfg.interval};int lows[]={1,0,0,0},highs[]={32,15,15,8};cycle(*vars[field],lows[field],highs[field]);}
   if(page==4)cycle(cfg.txAddress[field],0,255);if(page==5)cycle(cfg.rxAddress[field],0,255);if(page==6&&!cfg.source)cycle(cfg.payload[field],0,255);
   changedConfig();
  }
  // Statistics page: left/right switches raw receive representation.
  if(d&&(page==0||page==8)){cfg.format=(cfg.format+d+3)%3;savePending=true;changed=millis();refresh.invalidate();}
  uint32_t now=millis();if(savePending&&now-changed>=800)save();
  if(now-lastCheck>=1000){lastCheck=now;bool found=radio.isChipConnected();if(found!=present){present=found;apply();refresh.invalidate();}}
  if(present){if(asyncTx.poll(radio,now,cfg.rxLength)==1)ok++;for(int i=0;i<3&&radio.available();i++){
   uint8_t n=cfg.dynamic?radio.getDynamicPayloadSize():cfg.rxLength;if(n<1||n>32){radio.flush_rx();bad++;break;}radio.read(received,n);receivedLength=n;rx++;lastRx=now;
   if(stream){char message[128];int used=snprintf(message,sizeof(message),"[NRFRAW RX %u] ",n);for(int j=0;j<n;j++)used+=snprintf(message+used,sizeof(message)-used,"%02X%s",received[j],j+1==n?"\n":" ");if(Serial.availableForWrite()>=used)Serial.write((const uint8_t*)message,used);}
  }
  if(running&&!asyncTx.busy()&&now-paint<100&&now-lastTx>=periods[cfg.interval]){lastTx=now;uint8_t bytes[32];memcpy(bytes,cfg.payload,32);
   if(cfg.source)samplePacket(bytes);
   asyncTx.begin(radio,bytes,cfg.txLength,now);tx++;
  }}
  if(c=='T')Serial.printf("[NRFRAW] present=%d running=%d page=%d ch=%u rate=%u width=%u crc=%u ack=%u dynamic=%u rxlen=%u txlen=%u saved=%d TX=%lu OK=%lu RX=%lu BAD=%lu\n",present,running,page,cfg.channel,cfg.rate,cfg.width,cfg.crc,cfg.ack,cfg.dynamic,cfg.rxLength,cfg.txLength,!savePending&&saveOk,(unsigned long)tx,(unsigned long)ok,(unsigned long)rx,(unsigned long)bad);
  if(now-paint>=100&&!asyncTx.busy()){paint=now;screen.spr.resetViewport();screen.spr.fillSprite(TFT_BLACK);label(NrfUi::generic,8);screen.spr.drawFastHLine(20,55,200,TFT_WHITE);
   const uint8_t* headings[]={NrfUi::rawstats,NrfUi::settings,NrfUi::packet,NrfUi::timing,NrfUi::txaddr,NrfUi::rxaddr,NrfUi::payload,NrfUi::presets,NrfUi::data};label(headings[page],62,TFT_CYAN);screen.spr.drawRoundRect(6,433,228,81,7,TFT_WHITE);
   if(page==0){label(!present?NrfUi::absent:running?NrfUi::running:NrfUi::stopped,103,!present?TFT_YELLOW:running?TFT_GREEN:TFT_WHITE);screen.spr.drawRoundRect(10,140,220,78,8,TFT_WHITE);screen.spr.drawRoundRect(10,226,220,78,8,TFT_WHITE);screen.spr.drawRoundRect(10,312,220,112,8,TFT_WHITE);
    label(NrfUi::rawtx,144,TFT_WHITE);value(String(tx)+" / "+String(ok),181,running?TFT_GREEN:TFT_WHITE);label(NrfUi::rawrx,230,TFT_WHITE);value(String(rx)+" / "+String(receivedLength),267,TFT_WHITE);label(cfg.format==0?NrfUi::rxhex:cfg.format==1?NrfUi::rxtext:NrfUi::rxbin,314,TFT_WHITE);
    if(!receivedLength)label(NrfUi::nosignal,357,TFT_WHITE);else if(cfg.format==0){for(int i=0;i<2;i++){int start=i*4;if(start<receivedLength)value(hex(received,start,min(4,int(receivedLength)-start)),355+i*33,TFT_WHITE,4);}}
    else if(cfg.format==1){String text;for(int i=0;i<receivedLength;i++)text+=received[i]>=32&&received[i]<=126?char(received[i]):'.';for(int i=0;i<2;i++)value(text.substring(i*12,(i+1)*12),355+i*33,TFT_WHITE,4);}
    else{for(int i=0;i<2&&i<receivedLength;i++){String bits;for(int b=7;b>=0;b--)bits+=received[i]&(1<<b)?'1':'0';value(String(i)+": "+bits,355+i*33,TFT_WHITE);}}
    label(running?NrfUi::gstop:NrfUi::gstart,439);
   }else if(page==7){const uint8_t* names[]={NrfUi::reference,NrfUi::standard,NrfUi::oldaddress};for(int i=0;i<3;i++){int y=117+i*91;screen.spr.fillRoundRect(9,y,222,77,8,field==i?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,77,8,field==i?TFT_CYAN:TFT_WHITE);label(names[i],y+4,field==i?TFT_CYAN:TFT_WHITE);value(i==0?"CH2  2M  4B":i==1?"CH76 1M 32B":"00000 / 00001",y+43,TFT_WHITE,4);}label(NrfUi::presetwarn,397);label(running?NrfUi::gstop:NrfUi::presetapply,439);
   }else if(page==8){
    int start=field*4;
    label(cfg.format==0?NrfUi::ghex:cfg.format==1?NrfUi::gtext:NrfUi::gbin,107);
    value("BYTE "+String(start)+" - "+String(start+3),150,TFT_WHITE);screen.spr.drawRoundRect(10,187,220,201,8,TFT_WHITE);
    if(start>=receivedLength)label(NrfUi::nosignal,244,TFT_YELLOW);
    else for(int i=0;i<4&&start+i<receivedLength;i++){
     int index=start+i;String data;
     if(cfg.format==0)data=hex(received,index,1)+" / "+String(received[index]);
     else if(cfg.format==1)data=received[index]>=32&&received[index]<=126?String(char(received[index])):".";
     else for(int b=7;b>=0;b--)data+=received[index]&(1<<b)?'1':'0';
     value(String(index)+": "+data,201+i*44,TFT_WHITE);
    }
    label(running?NrfUi::gstop:NrfUi::gdatahint,439);
   }else{
    const uint8_t* names[4];String vals[4];int start=0,rows=4;
    if(page==1){names[0]=NrfUi::channel;names[1]=NrfUi::rate;names[2]=NrfUi::power;names[3]=NrfUi::width;vals[0]=String(cfg.channel)+" / "+String(2400+cfg.channel)+" MHz";vals[1]=cfg.rate==0?"1 Mbps":cfg.rate==1?"2 Mbps":"250 kbps";vals[2]=String(cfg.power)+" / "+String(-18+6*cfg.power)+" dBm";vals[3]=String(cfg.width)+" bytes";}
    if(page==2){names[0]=NrfUi::crc;names[1]=NrfUi::autoack;names[2]=NrfUi::dynamic;names[3]=NrfUi::rxlength;vals[0]=cfg.crc?String(cfg.crc*8)+" bit":"OFF";vals[1]=cfg.ack?"ON":"OFF";vals[2]=cfg.dynamic?"ON":"OFF";vals[3]=String(cfg.rxLength)+" bytes";}
    if(page==3){names[0]=NrfUi::txlength;names[1]=NrfUi::retrydelay;names[2]=NrfUi::retrycount;names[3]=NrfUi::period;vals[0]=String(cfg.txLength)+" bytes";vals[1]=String((cfg.retryDelay+1)*250)+" us";vals[2]=String(cfg.retryCount);vals[3]=String(periods[cfg.interval])+" ms";}
    if(page>=4){start=(field/4)*4;rows=min(4,count-start);}
    for(int i=0;i<rows;i++){int index=start+i,y=112+i*71;bool editable=!(page==6&&cfg.source);bool selected=editable&&field==index;screen.spr.fillRoundRect(9,y,222,68,7,selected?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,68,7,selected?TFT_CYAN:TFT_WHITE);
     if(page<4){label(names[i],y,selected?TFT_CYAN:TFT_WHITE);value(vals[i],y+36,selected?TFT_CYAN:TFT_WHITE);}else{value("BYTE "+String(index),y+5,!editable?TFT_WHITE:selected?TFT_CYAN:TFT_WHITE);uint8_t preview[32];if(page==6&&cfg.source)samplePacket(preview);const uint8_t* bytes=page==4?cfg.txAddress:page==5?cfg.rxAddress:cfg.source?preview:cfg.payload;value(hex(bytes,index,1)+" / "+String(bytes[index]),y+36,!editable?TFT_WHITE:selected?TFT_CYAN:TFT_WHITE);}}
    label(page==6?(cfg.source?NrfUi::sampledata:NrfUi::manualdata):savePending?NrfUi::saving:NrfUi::saved,400,savePending?TFT_YELLOW:TFT_WHITE);label(running?NrfUi::gstop:page==6?NrfUi::sourcehint:NrfUi::gadjust,439);
   }
   label(NrfUi::genericnav,480);value(String(page+1)+" / 9",511,TFT_WHITE,4);refresh.push((uint16_t*)screen.spr.getPointer());
  }
  delay(1);
 }
 running=false;if(present)asyncTx.cancel(radio,cfg.rxLength);if(savePending)save();radio.stopListening();radio.flush_tx();radio.flush_rx();radio.disableAckPayload();radio.disableDynamicPayloads();radio.setAddressWidth(5);radio.setAutoAck(true);for(int i=0;i<6;i++)radio.closeReadingPipe(i);nrf.init(ID,oldPower);radio.setChannel(oldChannel);radio.setDataRate(oldRate);radio.setPayloadSize(oldSize);radio.setCRCLength(oldCrc);radio.setRetries(5,15);screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
