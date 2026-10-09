// Configurable raw NRF24L01 interoperability tool. Never interprets arbitrary RX as actuator feedback.
bool nrfReadProfile(Preferences& prefs,const char* key,NrfCustom::Profile& result){
 unsigned length=prefs.getBytesLength(key);if(length!=sizeof(result)&&length!=NrfCustom::LegacySize)return false;
 uint8_t bytes[sizeof(result)];return prefs.getBytes(key,bytes,length)==length&&NrfCustom::restore(bytes,length,result);
}
bool nrfReadSelection(Preferences& prefs,NrfCustom::Selection& result){
 return prefs.getBytesLength("selected")==sizeof(result)&&prefs.getBytes("selected",&result,sizeof(result))==sizeof(result)&&NrfCustom::valid(result);
}
bool nrfWriteSelection(Preferences& prefs,const NrfCustom::Selection& selected){return NrfCustom::valid(selected)&&prefs.putBytes("selected",&selected,sizeof(selected))==sizeof(selected);}
int nrfGenericPage(uint8_t vehicle,int firstPage=0,bool useApplied=false){
 RadioSession radioSession(RadioNrf);
 using namespace NrfGeneric;NrfCustom::Profile custom;Config& cfg=custom.radio;Preferences prefs;int route=0;uint8_t appliedSlot=255;bool loadedApplied=false;
 const char* space=vehicle==1?"nrfDrone":vehicle==2?"nrfCar":"nrfRaw";
 if(vehicle){custom.forwardAxis=vehicle==1?3:1;preset(cfg,1);cfg.source=1;memcpy(cfg.txAddress,vehicle==1?"NDRN1":"NCAR1",5);memcpy(cfg.rxAddress,cfg.txAddress,5);}
 if(prefs.begin(space,true)){Config stored;if(prefs.getBytesLength("config")==sizeof(stored)&&prefs.getBytes("config",&stored,sizeof(stored))==sizeof(stored)&&valid(stored))cfg=stored;if(vehicle){nrfReadProfile(prefs,"active",custom);if(useApplied){NrfCustom::Selection selected;if(nrfReadSelection(prefs,selected)&&selected.kind==1){custom=selected.profile;appliedSlot=selected.slot;loadedApplied=true;}}}prefs.end();}
 auto& radio=nrf.radio;NrfButtons buttons;NrfActionLatch oLatch;NrfAsyncTx asyncTx;NrfExitChord exitChord;NrfUiPolicy::HoldB holdB;auto oldRate=radio.getDataRate();auto oldCrc=radio.getCRCLength();uint8_t oldChannel=radio.getChannel(),oldPower=radio.getPALevel(),oldSize=radio.getPayloadSize();
 const rf24_datarate_e rates[]={RF24_1MBPS,RF24_2MBPS,RF24_250KBPS};const uint16_t periods[]={20,50,100,200,250,500,1000,1500,2000};
 bool present=radio.isChipConnected(),running=false,savePending=false,saveOk=true;uint32_t changed=0,paint=0,lastTx=0,lastCheck=0,tx=0,ok=0,rx=0,bad=0,lastRx=0;
 const int pageCount=vehicle?14:9;int page=firstPage,field=0,presetIndex=0,slot=0,bankAction=firstPage==9?1:0,modeChoice=0,settingsGroup=0;bool occupiedSlots[5]={};bool deleteArmed=false;uint32_t noticeUntil=0;const uint8_t* notice=nullptr;uint16_t noticeColor=TFT_GREEN;
 auto notify=[&](const uint8_t* text,uint16_t color=TFT_GREEN){notice=text;noticeColor=color;noticeUntil=millis()+1600;};uint8_t received[32]={},receivedLength=0;JoystickNavigation nav;UiRefresh refresh;
 if(vehicle&&prefs.begin(space,true)){for(int i=0;i<5;i++){char key[8];snprintf(key,sizeof(key),"slot%d",i+1);NrfCustom::Profile stored=custom;occupiedSlots[i]=nrfReadProfile(prefs,key,stored);}slot=prefs.getUChar("lastSlot",0);if(slot>4)slot=0;if(page==9&&!occupiedSlots[slot]){for(int i=0;i<5;i++)if(occupiedSlots[i]){slot=i;break;}}if(page==9)field=slot;prefs.end();}
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
 auto save=[&](){saveOk=false;if(prefs.begin(space,false)){saveOk=vehicle?prefs.putBytes("active",&custom,sizeof(custom))==sizeof(custom):prefs.putBytes("config",&cfg,sizeof(cfg))==sizeof(cfg);prefs.end();}savePending=!saveOk;changed=millis();if(!saveOk)notify(NrfUi::failed,TFT_RED);else if(!vehicle)notify(NrfUi::saved,TFT_GREEN);};
 auto changedConfig=[&](){deleteArmed=false;if(!vehicle)notify(NrfUi::draft,TFT_YELLOW);savePending=true;changed=millis();apply();refresh.invalidate();};
 auto selectDefault=[&](uint8_t kind,uint8_t selectedSlot){NrfCustom::Selection selected;selected.kind=kind;selected.slot=selectedSlot;selected.profile=custom;bool success=false;if(prefs.begin(space,false)){success=nrfWriteSelection(prefs,selected);prefs.end();}if(success){savePending=true;changed=millis();notify(NrfUi::defaultsaved);route=1;}else notify(NrfUi::failed,TFT_RED);return success;};
 apply();if(loadedApplied)notify(NrfUi::defaultsaved);screen.spr.unloadFont();screen.spr.resetViewport();screen.spr.setTextSize(1);screen.spr.setTextDatum(TC_DATUM);
 auto label=[&](const uint8_t* b,int y,uint16_t color=TFT_WHITE){screen.spr.drawBitmap(10,y,b,220,36,color);};
 auto value=[&](const String& s,int y,uint16_t color=TFT_CYAN,int font=4){screen.spr.setTextColor(color,TFT_BLACK);screen.spr.drawString(s,120,y,font);};
 auto hex=[&](const uint8_t* bytes,int start,int count){String s;for(int i=0;i<count;i++){char b[4];snprintf(b,sizeof(b),"%02X",bytes[start+i]);if(i)s+=' ';s+=b;}return s;};
 auto samplePacket=[&](uint8_t* bytes){if(vehicle){NrfCustom::encode(custom,keys.kvs,ControlPacket::buttons(keys.kvs),uint16_t(tx),bytes);return;}memset(bytes,0,32);bytes[0]=axis(keys.kvs.LX);bytes[1]=axis(keys.kvs.LY);bytes[2]=axis(keys.kvs.RX);bytes[3]=axis(keys.kvs.RY);bytes[4]=axis(keys.kvs.L_knob);bytes[5]=axis(keys.kvs.R_knob);uint32_t mask=ControlPacket::buttons(keys.kvs);bytes[6]=mask;bytes[7]=mask>>8;bytes[8]=mask>>16;};
 String line;bool collecting=false,lineBlocked=false,stream=true;
 while(true){
  if(present&&asyncTx.poll(radio,millis(),cfg.rxLength)==1)ok++;
  keys.kvs_update();buttons.sample(keys.kvs,millis());char c=Serial.available()?Serial.read():0;
  // Consume the complete command even when rejected; its hex letters must never become control keys.
  if(collecting){
   if(c=='\n'||c=='\r'){
    if(lineBlocked)Serial.println("[NRF] stop control before editing payload");
    else{uint8_t bytes[32];int n=0,high=-1;bool good=true;for(unsigned i=0;i<line.length();i++){char ch=line[i];if(ch==' '||ch=='\t')continue;int v=ch>='0'&&ch<='9'?ch-'0':ch>='a'&&ch<='f'?ch-'a'+10:ch>='A'&&ch<='F'?ch-'A'+10:-1;if(v<0){good=false;break;}if(high<0)high=v;else{if(n>=32){good=false;break;}bytes[n++]=(high<<4)|v;high=-1;}}if(good&&n&&high<0){memset(cfg.payload,0,32);memcpy(cfg.payload,bytes,n);cfg.txLength=n;cfg.source=0;changedConfig();Serial.printf("[NRFRAW] payload accepted length=%d\n",n);}else Serial.println("[NRFRAW] invalid hex payload");}
    collecting=false;lineBlocked=false;line="";
   }else if(c&&!lineBlocked){if(line.length()<96)line+=c;else{lineBlocked=true;line="";}}
   c=0;
  }else if(c=='@'){collecting=true;lineBlocked=vehicle&&running;line="";c=0;}
  auto move=(!vehicle||(!running&&page!=0))?nav.update(keys.kvs.LX,keys.kvs.LY,millis()):JoystickNavigation::None;
  bool chordExit=exitChord.update(!keys.kvs.b,!keys.kvs.x,millis());
  bool longB=holdB.update(!keys.kvs.b,!keys.kvs.x,millis());
  if(c=='Q'||(vehicle&&page==0&&chordExit)||(!vehicle&&buttons.pressed(NrfButtons::X)))break;
  if(vehicle&&(buttons.pressed(NrfButtons::X)||c=='X')&&page!=0){if(page==12){route=1;break;}page=12;field=0;deleteArmed=false;nav.reset();refresh.invalidate();}
  if(c=='S'){if(vehicle&&running)Serial.println("[SCREEN] busy: stop control before capture");else uiScreenshot(true);}if(c=='V')stream=!stream;
  bool requestedPage=c=='B'||(vehicle&&page==0?longB:buttons.pressed(NrfButtons::B));
  if(vehicle){
   if(requestedPage&&page==0){if(running)notify(NrfUi::stopfirst,TFT_YELLOW);else{page=12;field=0;nav.reset();refresh.invalidate();}}
   else if(requestedPage&&page!=9&&page!=12&&page!=13){if(settingsGroup==0)page=page==1?4:page==4?5:1;else if(settingsGroup==1)page=page==2?3:page==3?7:page==7?8:2;else if(settingsGroup==2)page=page==6?10:6;else if(settingsGroup==4)page=page==11?10:11;field=0;deleteArmed=false;nav.reset();refresh.invalidate();}
  }else if(requestedPage){page=(page+1)%pageCount;field=0;deleteArmed=false;nav.reset();refresh.invalidate();}
  bool stopRequested=vehicle&&(buttons.pressed(NrfButtons::A)||c=='A'||!keys.kvs.a);bool oAction=oLatch.update(!keys.kvs.o,millis(),running);
  if(stopRequested&&page!=0&&page!=9&&page!=13&&vehicle){running=false;if(present)asyncTx.cancel(radio,cfg.rxLength);page=9;field=slot;bankAction=0;deleteArmed=false;nav.reset();refresh.invalidate();}
  if(stopRequested){running=false;if(present)asyncTx.cancel(radio,cfg.rxLength);refresh.invalidate();}
  else if(!vehicle&&(buttons.pressed(NrfButtons::A)||c=='A')){if(page==0){if(present)asyncTx.cancel(radio,cfg.rxLength);tx=ok=rx=bad=0;}else if(page==6){cfg.source=1-cfg.source;changedConfig();}}
  if((oAction||c=='E')&&!stopRequested){
   if(running){running=false;if(present)asyncTx.cancel(radio,cfg.rxLength);}
   else if(vehicle&&page==12){const int destinations[]={1,2,6,9,11,13};settingsGroup=field;page=destinations[field];field=page==9?slot:0;bankAction=0;nav.reset();refresh.invalidate();}
   else if(vehicle&&page==13){if(selectDefault(modeChoice?1:0,255))break;}
   else if(page==9&&vehicle){
    const int requestedAction=bankAction;char key[8];snprintf(key,sizeof(key),"slot%d",slot+1);bool success=false;
    if(bankAction==2&&!deleteArmed){deleteArmed=true;notify(NrfUi::deleteconfirm,TFT_YELLOW);}
    else if(prefs.begin(space,false)){
     if(requestedAction==0){success=prefs.putBytes(key,&custom,sizeof(custom))==sizeof(custom);if(success){bankAction=1;const uint8_t* savedNames[]={NrfUi::savedslot1,NrfUi::savedslot2,NrfUi::savedslot3,NrfUi::savedslot4,NrfUi::savedslot5};notify(savedNames[slot]);}else notify(NrfUi::failed,TFT_RED);}
     if(requestedAction==1){NrfCustom::Profile loaded;loaded.forwardAxis=vehicle==1?3:1;if(nrfReadProfile(prefs,key,loaded)){NrfCustom::Selection selected;selected.kind=1;selected.slot=slot;selected.profile=loaded;success=nrfWriteSelection(prefs,selected);if(success){custom=loaded;route=1;}}notify(success?NrfUi::defaultsaved:NrfUi::failed,success?TFT_GREEN:TFT_RED);}
     if(requestedAction==2){NrfCustom::Selection selected;bool canDelete=true;if(nrfReadSelection(prefs,selected)&&selected.kind==1&&selected.slot==slot){selected.kind=0;selected.slot=255;canDelete=nrfWriteSelection(prefs,selected);}success=canDelete&&(!prefs.isKey(key)||prefs.remove(key));deleteArmed=false;notify(success?NrfUi::deletedok:NrfUi::failed,success?TFT_GREEN:TFT_RED);}
     if(success&&requestedAction!=2)prefs.putUChar("lastSlot",slot);prefs.end();if(success&&requestedAction!=1)occupiedSlots[slot]=requestedAction==0;if(requestedAction==1&&success){savePending=true;changed=millis();apply();break;}
    }else notify(NrfUi::failed,TFT_RED);
   }else if(page==7){preset(cfg,presetIndex);changedConfig();if(vehicle){if(selectDefault(1,255))break;}else notify(NrfUi::applied);}
   else if(vehicle&&page!=0){if(selectDefault(1,255))break;}
   else if(present){running=true;lastTx=millis()-periods[cfg.interval];}paint=millis()-100;refresh.invalidate();
  }
  int count=page==12?6:page==13?2:page==9?5:page==10?32:page==4||page==5?cfg.width:page==6?(vehicle?33:32):page==7?3:page==8?8:4;
  if(page&&!running){if(buttons.pressed(NrfButtons::Up)||c=='U'||move==JoystickNavigation::Up){field=(field+count-1)%count;refresh.invalidate();}if(buttons.pressed(NrfButtons::Down)||c=='D'||move==JoystickNavigation::Down){field=(field+1)%count;refresh.invalidate();}}
  int d=(!vehicle||(!running&&page!=0))?((buttons.pressed(NrfButtons::Right)||c=='R'||move==JoystickNavigation::Right)?1:(buttons.pressed(NrfButtons::Left)||c=='L'||move==JoystickNavigation::Left)?-1:0):0;
  if(page==7)presetIndex=field;
  if(page==13)modeChoice=field;
  if(page==9){if(slot!=field){deleteArmed=false;slot=field;}if(d){bankAction=(bankAction+d+3)%3;deleteArmed=false;refresh.invalidate();}}
  if(d&&!running&&page==10){custom.map[field]=(custom.map[field]+d+32)%32;changedConfig();}
  if(d&&!running&&page==11){if(field==0)custom.forwardAxis=custom.forwardAxis==1?3:1;if(field==1)custom.turnAxis=custom.turnAxis==0?2:0;if(field==2)custom.forwardReverse=1-custom.forwardReverse;if(field==3)custom.turnReverse=1-custom.turnReverse;changedConfig();}
  if(d&&!running&&page>0&&page<7&&!(page==6&&cfg.source&&!(vehicle&&field==0))){
   auto cycle=[&](uint8_t& v,int low,int high){v=low+(int(v)-low+d+high-low+1)%(high-low+1);};
   if(page==1){uint8_t* vars[]={&cfg.channel,&cfg.rate,&cfg.power,&cfg.width};int lows[]={0,0,0,3},highs[]={83,2,3,5};cycle(*vars[field],lows[field],highs[field]);}
   if(page==2){uint8_t* vars[]={&cfg.crc,&cfg.ack,&cfg.dynamic,&cfg.rxLength};int lows[]={0,0,0,1},highs[]={2,1,1,32};cycle(*vars[field],lows[field],highs[field]);if(cfg.ack&&cfg.crc==0)cfg.crc=1;}
   if(page==3){uint8_t* vars[]={&cfg.txLength,&cfg.retryDelay,&cfg.retryCount,&cfg.interval};int lows[]={1,0,0,0},highs[]={32,15,15,8};cycle(*vars[field],lows[field],highs[field]);}
   if(page==4)cycle(cfg.txAddress[field],0,255);if(page==5)cycle(cfg.rxAddress[field],0,255);if(page==6){if(vehicle&&field==0)cycle(cfg.source,0,1);else if(!cfg.source)cycle(cfg.payload[vehicle?field-1:field],0,255);}
   changedConfig();
  }
  // Statistics page: left/right switches raw receive representation.
  if(d&&(page==0||page==8)){cfg.format=(cfg.format+d+3)%3;savePending=true;changed=millis();refresh.invalidate();}
  uint32_t now=millis();const uint32_t paintInterval=vehicle&&page==0?33:100;if(savePending&&now-changed>=800)save();
  if(now-lastCheck>=1000){lastCheck=now;bool found=radio.isChipConnected();if(found!=present){present=found;apply();refresh.invalidate();}}
  if(present){if(asyncTx.poll(radio,now,cfg.rxLength)==1)ok++;for(int i=0;i<3&&radio.available();i++){
   uint8_t n=cfg.dynamic?radio.getDynamicPayloadSize():cfg.rxLength;if(n<1||n>32){radio.flush_rx();bad++;break;}radio.read(received,n);receivedLength=n;rx++;lastRx=now;
   if(stream){char message[128];int used=snprintf(message,sizeof(message),"[NRFRAW RX %u] ",n);for(int j=0;j<n;j++)used+=snprintf(message+used,sizeof(message)-used,"%02X%s",received[j],j+1==n?"\n":" ");if(Serial.availableForWrite()>=used)Serial.write((const uint8_t*)message,used);}
  }
  if(running&&!asyncTx.busy()&&now-paint<paintInterval&&now-lastTx>=periods[cfg.interval]){lastTx=now;uint8_t bytes[32];memcpy(bytes,cfg.payload,32);
   if(cfg.source)samplePacket(bytes);
   asyncTx.begin(radio,bytes,cfg.txLength,now);tx++;
  }}
  if(c=='T')Serial.printf("[NRFRAW] present=%d running=%d page=%d ch=%u rate=%u width=%u crc=%u ack=%u dynamic=%u rxlen=%u txlen=%u saved=%d TX=%lu OK=%lu RX=%lu BAD=%lu\n",present,running,page,cfg.channel,cfg.rate,cfg.width,cfg.crc,cfg.ack,cfg.dynamic,cfg.rxLength,cfg.txLength,!savePending&&saveOk,(unsigned long)tx,(unsigned long)ok,(unsigned long)rx,(unsigned long)bad);
  if(now-paint>=paintInterval&&!asyncTx.busy()){paint=now;screen.spr.resetViewport();screen.spr.fillSprite(TFT_BLACK);label(vehicle?(page==0?(vehicle==1?NrfUi::drone:NrfUi::car):(vehicle==1?NrfUi::customdrone:NrfUi::customcar)):NrfUi::generic,8);screen.spr.drawFastHLine(20,55,200,TFT_WHITE);
   const uint8_t* headings[]={NrfUi::rawstats,NrfUi::settings,NrfUi::packet,NrfUi::timing,NrfUi::txaddr,NrfUi::rxaddr,NrfUi::payload,NrfUi::presets,NrfUi::data,NrfUi::profilebank,NrfUi::mapping,NrfUi::joydir,NrfUi::hub,NrfUi::defaultmode};if(vehicle&&page==0&&loadedApplied&&appliedSlot<5){const uint8_t* names[]={NrfUi::using1,NrfUi::using2,NrfUi::using3,NrfUi::using4,NrfUi::using5};label(names[appliedSlot],62,TFT_GREEN);}else label(headings[page],62,TFT_CYAN);screen.spr.drawRoundRect(6,433,228,81,7,TFT_WHITE);
   if(page==12){
    const uint8_t* names[]={NrfUi::radiomenu,NrfUi::packetmenu,NrfUi::contentmenu,NrfUi::profilebank,NrfUi::joymenu,NrfUi::defaultmode};
    for(int i=0;i<6;i++){int y=107+i*48;screen.spr.fillRoundRect(9,y,222,45,6,field==i?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,45,6,field==i?TFT_CYAN:TFT_WHITE);label(names[i],y+4,field==i?TFT_CYAN:TFT_WHITE);}label(NrfUi::menuoperate,439);
   }else if(page==13){const uint8_t* names[]={NrfUi::standardmode,NrfUi::custommode};for(int i=0;i<2;i++){int y=140+i*100;screen.spr.fillRoundRect(9,y,222,82,8,field==i?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,82,8,field==i?TFT_CYAN:TFT_WHITE);label(names[i],y+23,field==i?TFT_CYAN:TFT_WHITE);}label(NrfUi::menuoperate,439);
   }else if(page==9){
    const uint8_t* storedNames[]={NrfUi::slot1saved,NrfUi::slot2saved,NrfUi::slot3saved,NrfUi::slot4saved,NrfUi::slot5saved};
    const uint8_t* emptyNames[]={NrfUi::slot1empty,NrfUi::slot2empty,NrfUi::slot3empty,NrfUi::slot4empty,NrfUi::slot5empty};
    for(int i=0;i<5;i++){int y=107+i*51;bool selected=slot==i;screen.spr.fillRoundRect(9,y,222,48,6,selected?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,48,6,selected?TFT_CYAN:TFT_WHITE);label(occupiedSlots[i]?storedNames[i]:emptyNames[i],y+6,selected?TFT_CYAN:occupiedSlots[i]?TFT_GREEN:TFT_WHITE);}
    const uint8_t* actions[]={NrfUi::saveslot,NrfUi::loadslot,NrfUi::delslot};label(actions[bankAction],359,bankAction==2?TFT_YELLOW:TFT_CYAN);label(bankAction==0?NrfUi::saveguide:bankAction==1?NrfUi::applyguide:NrfUi::deleteguide,400);label(NrfUi::bankoperations,439);
   }else if(vehicle&&page==0){
    label(!present?NrfUi::absent:running?NrfUi::controlrunning:NrfUi::controlstopped,103,!present?TFT_YELLOW:running?TFT_GREEN:TFT_WHITE);
    screen.spr.drawRoundRect(10,140,220,78,8,TFT_WHITE);label(NrfUi::rawtx,144);value(String(tx)+" / "+String(ok),181,running?TFT_GREEN:TFT_WHITE);
    value("LX "+String(keys.kvs.LX)+" LY "+String(keys.kvs.LY),228,TFT_CYAN);value("RX "+String(keys.kvs.RX)+" RY "+String(keys.kvs.RY),262,TFT_CYAN);value("L "+String(keys.kvs.L_knob)+" R "+String(keys.kvs.R_knob),294,TFT_WHITE);
    UiLayout::joystick(screen.spr,10,325,keys.kvs.LX,keys.kvs.LY);UiLayout::joystick(screen.spr,129,325,keys.kvs.RX,keys.kvs.RY);label(running?NrfUi::customstop:NrfUi::customstart,439);
   }else if(page==0){label(!present?NrfUi::absent:running?NrfUi::running:NrfUi::stopped,103,!present?TFT_YELLOW:running?TFT_GREEN:TFT_WHITE);screen.spr.drawRoundRect(10,140,220,78,8,TFT_WHITE);screen.spr.drawRoundRect(10,226,220,78,8,TFT_WHITE);screen.spr.drawRoundRect(10,312,220,112,8,TFT_WHITE);
    label(NrfUi::rawtx,144,TFT_WHITE);value(String(tx)+" / "+String(ok),181,running?TFT_GREEN:TFT_WHITE);label(NrfUi::rawrx,230,TFT_WHITE);value(String(rx)+" / "+String(receivedLength),267,TFT_CYAN);label(cfg.format==0?NrfUi::rxhex:cfg.format==1?NrfUi::rxtext:NrfUi::rxbin,314,TFT_WHITE);
    if(!receivedLength)label(NrfUi::nosignal,357,TFT_WHITE);else if(cfg.format==0){for(int i=0;i<2;i++){int start=i*4;if(start<receivedLength)value(hex(received,start,min(4,int(receivedLength)-start)),355+i*33,TFT_WHITE,4);}}
    else if(cfg.format==1){String text;for(int i=0;i<receivedLength;i++)text+=received[i]>=32&&received[i]<=126?char(received[i]):'.';for(int i=0;i<2;i++)value(text.substring(i*12,(i+1)*12),355+i*33,TFT_WHITE,4);}
    else{for(int i=0;i<2&&i<receivedLength;i++){String bits;for(int b=7;b>=0;b--)bits+=received[i]&(1<<b)?'1':'0';value(String(i)+": "+bits,355+i*33,TFT_WHITE);}}
    label(vehicle?(running?NrfUi::customstop:NrfUi::customstart):running?NrfUi::gstop:NrfUi::gstart,439);
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
   }else if(vehicle&&page==6){
    int start=(field/4)*4;uint8_t bytes[32];if(cfg.source)samplePacket(bytes);else memcpy(bytes,cfg.payload,32);
    for(int i=0;i<min(4,33-start);i++){int index=start+i,y=112+i*71;bool editable=index==0||!cfg.source;bool selected=editable&&field==index;screen.spr.fillRoundRect(9,y,222,68,7,selected?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,68,7,selected?TFT_CYAN:TFT_WHITE);
     if(index==0){label(NrfUi::sourceData,y-2,selected?TFT_CYAN:TFT_WHITE);label(cfg.source?NrfUi::sampledata:NrfUi::manualdata,y+31,selected?TFT_CYAN:TFT_WHITE);}
     else{value("BYTE "+String(index-1),y+5,selected?TFT_CYAN:TFT_WHITE);value(hex(bytes,index-1,1)+" / "+String(bytes[index-1]),y+36,selected?TFT_CYAN:TFT_WHITE);}}
    label(savePending?NrfUi::saving:NrfUi::saveentry,400,TFT_WHITE);label(NrfUi::gadjust,439);
   }else{
    const uint8_t* names[4];String vals[4];int start=0,rows=4;
    if(page==1){names[0]=NrfUi::channel;names[1]=NrfUi::rate;names[2]=NrfUi::power;names[3]=NrfUi::width;vals[0]=String(cfg.channel)+" / "+String(2400+cfg.channel)+" MHz";vals[1]=cfg.rate==0?"1 Mbps":cfg.rate==1?"2 Mbps":"250 kbps";vals[2]=String(cfg.power)+" / "+String(-18+6*cfg.power)+" dBm";vals[3]=String(cfg.width)+" bytes";}
    if(page==2){names[0]=NrfUi::crc;names[1]=NrfUi::autoack;names[2]=NrfUi::dynamic;names[3]=NrfUi::rxlength;vals[0]=cfg.crc?String(cfg.crc*8)+" bit":"OFF";vals[1]=cfg.ack?"ON":"OFF";vals[2]=cfg.dynamic?"ON":"OFF";vals[3]=String(cfg.rxLength)+" bytes";}
    if(page==3){names[0]=NrfUi::txlength;names[1]=NrfUi::retrydelay;names[2]=NrfUi::retrycount;names[3]=NrfUi::period;vals[0]=String(cfg.txLength)+" bytes";vals[1]=String((cfg.retryDelay+1)*250)+" us";vals[2]=String(cfg.retryCount);vals[3]=String(periods[cfg.interval])+" ms";}
    if(page>=4&&page!=11){start=(field/4)*4;rows=min(4,count-start);}
    for(int i=0;i<rows;i++){int index=start+i,y=112+i*71;bool editable=!(page==6&&cfg.source);bool selected=editable&&field==index;screen.spr.fillRoundRect(9,y,222,68,7,selected?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,68,7,selected?TFT_CYAN:TFT_WHITE);
     if(page==11){
      const uint8_t* directionNames[]={NrfUi::forwardaxis,NrfUi::turnaxis,NrfUi::forwardrev,NrfUi::turnrev};
      const uint8_t* directionValues[]={custom.forwardAxis==1?NrfUi::leftvertical:NrfUi::rightvertical,custom.turnAxis==0?NrfUi::lefthorizontal:NrfUi::righthorizontal,custom.forwardReverse?NrfUi::reversedir:NrfUi::normaldir,custom.turnReverse?NrfUi::reversedir:NrfUi::normaldir};
      label(directionNames[i],y-2,selected?TFT_CYAN:TFT_WHITE);label(directionValues[i],y+31,selected?TFT_CYAN:TFT_WHITE);
     }else if(page==10){
      const char* sourceNames[]={"CONST","LX S8","LY S8","RX S8","RY S8","KNOB L S8","KNOB R S8","LX +100","LY +100","RX +100","RY +100","KNOB L +100","KNOB R +100","LX U8","LY U8","RX U8","RY U8","KNOB L U8","KNOB R U8","KEY 0-7","KEY 8-15","KEY 16-17","GYRO X","GYRO Y","SEQ LOW","SEQ HIGH","FORWARD S8","TURN S8","FORWARD +100","TURN +100","FORWARD U8","TURN U8"};value("BYTE "+String(index),y+5,selected?TFT_CYAN:TFT_WHITE);value(sourceNames[custom.map[index]],y+36,selected?TFT_CYAN:TFT_WHITE);
     }else if(page<4){label(names[i],y,selected?TFT_CYAN:TFT_WHITE);value(vals[i],y+36,selected?TFT_CYAN:TFT_WHITE);}else{value("BYTE "+String(index),y+5,!editable?TFT_WHITE:selected?TFT_CYAN:TFT_WHITE);uint8_t preview[32];if(page==6&&cfg.source)samplePacket(preview);const uint8_t* bytes=page==4?cfg.txAddress:page==5?cfg.rxAddress:cfg.source?preview:cfg.payload;value(hex(bytes,index,1)+" / "+String(bytes[index]),y+36,!editable?TFT_WHITE:selected?TFT_CYAN:TFT_WHITE);}}
    label(page==6?(cfg.source?NrfUi::sampledata:NrfUi::manualdata):savePending?NrfUi::saving:vehicle?NrfUi::saveentry:NrfUi::saved,400,savePending?TFT_YELLOW:saveOk?TFT_GREEN:TFT_RED);label(vehicle?(page==6?NrfUi::sourceconfirm:NrfUi::gadjust):running?NrfUi::gstop:page==6?NrfUi::sourcehint:NrfUi::gadjust,439);
   }
   label(vehicle?(page==0?NrfUi::controlnav:page==9?NrfUi::bankback:page==12?NrfUi::hubback:page==13?NrfUi::menunav:NrfUi::settingnav):NrfUi::genericnav,480);value(String(page+1)+" / "+String(pageCount),511,TFT_WHITE,4);if(notice&&int32_t(noticeUntil-now)>0){screen.spr.fillRoundRect(6,433,228,40,6,0x0843);screen.spr.drawRoundRect(6,433,228,40,6,noticeColor);label(notice,435,noticeColor);}
   refresh.push((uint16_t*)screen.spr.getPointer());
  }
  if(c=='T'&&vehicle)Serial.printf("[NRFPROFILE] vehicle=%u slot=%d action=%d map0=%u source=%u occupied=%d confirm=%d forward=%u turn=%u frev=%u trev=%u applied=%d defaultslot=%u\n",vehicle,slot+1,bankAction,custom.map[0],cfg.source,occupiedSlots[slot],deleteArmed,custom.forwardAxis,custom.turnAxis,custom.forwardReverse,custom.turnReverse,loadedApplied,appliedSlot);
  delay(1);
 }
 running=false;if(present)asyncTx.cancel(radio,cfg.rxLength);if(savePending)save();radio.stopListening();radio.flush_tx();radio.flush_rx();radio.disableAckPayload();radio.disableDynamicPayloads();radio.setAddressWidth(5);radio.setAutoAck(true);for(int i=0;i<6;i++)radio.closeReadingPipe(i);nrf.init(ID,oldPower);radio.setChannel(oldChannel);radio.setDataRate(oldRate);radio.setPayloadSize(oldSize);radio.setCRCLength(oldCrc);radio.setRetries(5,15);screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);return route;
}

void nrfGeneric(){nrfGenericPage(0);}
