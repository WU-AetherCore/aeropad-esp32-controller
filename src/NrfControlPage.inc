int nrfControlPage(uint8_t mode){
 RadioSession radioSession(RadioNrf);
 int route=0;int menuField=0;NrfUiPolicy::HoldB holdB;uint32_t blockedUntil=0;
 auto& radio=nrf.radio;NrfButtons buttons;NrfExitChord exitChord;NrfCustom::Profile directions;directions.forwardAxis=mode==NrfControl::Drone?3:1;NrfGeneric::preset(directions.radio,1);directions.radio.source=1;memcpy(directions.radio.txAddress,mode==NrfControl::Drone?"NDRN1":"NCAR1",5);memcpy(directions.radio.rxAddress,directions.radio.txAddress,5);
 const char* directionSpace=mode==NrfControl::Drone?"nrfDrone":"nrfCar";Preferences directionPrefs;if(directionPrefs.begin(directionSpace,true)){nrfReadProfile(directionPrefs,"active",directions);NrfCustom::Selection selected;if(nrfReadSelection(directionPrefs,selected)&&selected.kind==0)directions=selected.profile;directionPrefs.end();}
 uint32_t settingNotice=0;bool directionSaved=true;const uint8_t oldChannel=radio.getChannel(),oldPower=radio.getPALevel(),oldSize=radio.getPayloadSize();
 const auto oldRate=radio.getDataRate();const auto oldCrc=radio.getCRCLength();
 int channel=76,rate=0,power=0,limit=mode==NrfControl::Drone?100:25,page=0,field=0;bool present=radio.isChipConnected(),armed=false,haveFeedback=false,hold=false,canArm=false;
 uint16_t seq=0;uint32_t session=esp_random(),sent=0,ack=0,lastSend=0,lastPaint=0,lastFeedback=0,holdStart=0,lastCheck=0,armedAt=0;
 Preferences profile;profile.begin("nrfCtl",false);const char* profileKey=mode==NrfControl::Drone?"drone":"car";
 uint8_t saved[5]={};if(profile.getBytesLength(profileKey)==5){profile.getBytes(profileKey,saved,5);if(saved[0]==1&&saved[1]<=83&&saved[2]<=2&&saved[3]<=3&&saved[4]>=25&&saved[4]<=100&&saved[4]%25==0){channel=saved[1];rate=saved[2];power=saved[3];limit=saved[4];}}
 uint8_t feedback[32]={};const rf24_datarate_e rates[]={RF24_1MBPS,RF24_2MBPS,RF24_250KBPS};
 auto configure=[&](){
  radio.stopListening();radio.flush_rx();radio.flush_tx();radio.setAddressWidth(5);radio.setPayloadSize(32);radio.setChannel(channel);radio.setPALevel(power);
  if(!radio.setDataRate(rates[rate])){rate=0;radio.setDataRate(rates[rate]);}
  radio.setCRCLength(RF24_CRC_16);radio.setAutoAck(true);radio.setRetries(0,3);radio.enableAckPayload();
  radio.openWritingPipe(reinterpret_cast<const uint8_t*>(mode==NrfControl::Drone?"NDRN1":"NCAR1"));
 };
 if(present)configure();JoystickNavigation navigation;UiRefresh refresh;
 screen.spr.unloadFont();screen.spr.resetViewport();screen.spr.setTextDatum(TC_DATUM);screen.spr.setTextSize(1);
 auto label=[&](const uint8_t* b,int y,uint16_t color=TFT_WHITE){screen.spr.drawBitmap(10,y,b,220,36,color);};
 auto value=[&](const String& text,int y,uint16_t color=TFT_CYAN){screen.spr.setTextColor(color,TFT_BLACK);screen.spr.drawString(text,120,y,4);};
 auto send=[&](){
  uint8_t packet[32];auto controls=NrfCustom::controlSample(directions,keys.kvs,mode);NrfControl::encode(controls,mode,armed,seq++,session,millis(),limit,packet);sent++;
  if(radio.write(packet,32))ack++;
  for(int i=0;i<3&&radio.available();i++){
   uint8_t n=radio.getDynamicPayloadSize(),payload[32];if(n!=32){if(n&&n<=32)radio.read(payload,n);else radio.flush_rx();continue;}
   radio.read(payload,32);uint16_t age=uint16_t(uint16_t(seq-1)-NrfControl::r16(payload+4));
   uint32_t stamp=payload[27]|uint32_t(payload[28])<<8|uint32_t(payload[29])<<16;
   if(NrfControl::feedbackValid(payload,mode)&&age<=8&&stamp==(session&0xffffff)){memcpy(feedback,payload,32);lastFeedback=millis();haveFeedback=true;}
  }
 };
 while(true){keys.kvs_update();buttons.sample(keys.kvs,millis());uint32_t now=millis();char c=Serial.available()?Serial.read():0;
  if(c=='S'||c=='W'){if(armed||hold)Serial.println("[SCREEN] busy: stop control before capture");else uiScreenshot(true);}
  bool online=present&&haveFeedback&&uint32_t(now-lastFeedback)<NrfControl::Timeout&&!(feedback[7]&2);
  bool remoteArmed=haveFeedback&&uint32_t(now-lastFeedback)<NrfControl::Timeout&&(feedback[7]&1);
  if(!online||(armed&&uint32_t(now-armedAt)>=100&&!(feedback[7]&1))){armed=false;hold=false;canArm=false;}
  bool stopRequested=buttons.pressed(NrfButtons::A)||c=='A'||!keys.kvs.a;
  if(stopRequested){armed=false;hold=false;canArm=false;lastSend=now-20;}
  bool controlPage=NrfUiPolicy::standardControl(page);bool longB=holdB.update(!keys.kvs.b,!keys.kvs.x,now);
  bool exitRequested=exitChord.update(!keys.kvs.b,!keys.kvs.x,now);
  if((controlPage&&exitRequested)||c=='Q'){armed=false;if(present)for(int i=0;i<3;i++)send();break;}
  if(!controlPage&&(buttons.pressed(NrfButtons::X)||c=='X')){page=0;field=0;exitChord=NrfExitChord();holdB=NrfUiPolicy::HoldB();navigation.reset();refresh.invalidate();}
  bool switchPage=c=='B'||(keys.kvs.x&&(controlPage?longB:buttons.pressed(NrfButtons::B)));
  if(switchPage){bool active=armed||hold||remoteArmed;page=NrfUiPolicy::standardNext(page,active);if(active)blockedUntil=now+1600;field=0;menuField=0;navigation.reset();refresh.invalidate();}
  if(page==3&&NrfUiPolicy::canConfigure(armed,hold,remoteArmed)){
   auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,now);
   if(buttons.pressed(NrfButtons::Up)||c=='U'||move==JoystickNavigation::Up)menuField=(menuField+2)%3;
   if(buttons.pressed(NrfButtons::Down)||c=='D'||move==JoystickNavigation::Down)menuField=(menuField+1)%3;
   if(buttons.pressed(NrfButtons::O)||c=='E'){route=menuField==0?2:menuField==1?3:4;break;}
  }
  auto controls=NrfCustom::controlSample(directions,keys.kvs,mode);
  int8_t axes[]={controls.LX,controls.LY,controls.RX,controls.RY,controls.L_knob,controls.R_knob};
  bool safe=NrfControl::neutral(mode,axes);
  bool held=!keys.kvs.o;if(!held&&!stopRequested)canArm=true;
  if(canArm&&held&&!stopRequested&&!armed&&!remoteArmed&&online&&safe&&page<2){if(!hold){hold=true;holdStart=now;}if(uint32_t(now-holdStart)>=1000){armed=true;armedAt=now;}}
  else if(!held||!safe)hold=false;
  if(page==4&&NrfUiPolicy::canConfigure(armed,hold,remoteArmed)){
   auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,now);
   if(buttons.pressed(NrfButtons::Up)||c=='U'||move==JoystickNavigation::Up)field=(field+3)%4;
   if(buttons.pressed(NrfButtons::Down)||c=='D'||move==JoystickNavigation::Down)field=(field+1)%4;
   bool change=buttons.pressed(NrfButtons::Right)||buttons.pressed(NrfButtons::Left)||c=='R'||c=='L'||move==JoystickNavigation::Right||move==JoystickNavigation::Left;
   if(change){if(field==0)directions.forwardAxis=directions.forwardAxis==1?3:1;if(field==1)directions.turnAxis=directions.turnAxis==0?2:0;if(field==2)directions.forwardReverse=1-directions.forwardReverse;if(field==3)directions.turnReverse=1-directions.turnReverse;
    directionSaved=false;if(directionPrefs.begin(directionSpace,false)){NrfCustom::Selection selected;selected.kind=0;selected.profile=directions;directionSaved=nrfWriteSelection(directionPrefs,selected)&&directionPrefs.putBytes("active",&directions,sizeof(directions))==sizeof(directions);directionPrefs.end();}settingNotice=now;refresh.invalidate();
   }
  }
  if(page==2&&NrfUiPolicy::canConfigure(armed,hold,remoteArmed)){
   auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,now);
   if(buttons.pressed(NrfButtons::Up)||c=='U'||move==JoystickNavigation::Up)field=(field+3)%4;
   if(buttons.pressed(NrfButtons::Down)||c=='D'||move==JoystickNavigation::Down)field=(field+1)%4;
   int direction=(buttons.pressed(NrfButtons::Right)||c=='R'||move==JoystickNavigation::Right)?1:(buttons.pressed(NrfButtons::Left)||c=='L'||move==JoystickNavigation::Left)?-1:0;
   if(direction){if(field==0)channel=(channel+direction+84)%84;if(field==1)rate=(rate+direction+3)%3;if(field==2)power=(power+direction+4)%4;if(field==3)limit=25*((limit/25+direction+3)%4+1);
    haveFeedback=false;hold=false;if(present)configure();uint8_t settings[]={1,uint8_t(channel),uint8_t(rate),uint8_t(power),uint8_t(limit)};profile.putBytes(profileKey,settings,5);refresh.invalidate();}
  }
  if(now-lastCheck>=1000){lastCheck=now;bool found=radio.isChipConnected();if(found!=present){present=found;armed=false;haveFeedback=false;if(present)configure();}}
  if(present&&uint32_t(now-lastSend)>=20){lastSend=now;send();}
  if(c=='T')Serial.printf("[NRFCTRL] mode=%u page=%d present=%d online=%d armed=%d neutral=%d throttle=%d channel=%d rate=%d power=%d limit=%d TX=%lu ACK=%lu\n",mode,page,present,online,armed,safe,NrfControl::throttle(keys.kvs.L_knob),channel,rate,power,limit,(unsigned long)sent,(unsigned long)ack);
  if(c=='T')Serial.printf("[NRFDIR] forward=%u turn=%u frev=%u trev=%u saved=%d\n",directions.forwardAxis,directions.turnAxis,directions.forwardReverse,directions.turnReverse,directionSaved);
  if(now-lastPaint>=33){lastPaint=now;screen.spr.resetViewport();screen.spr.fillSprite(TFT_BLACK);
   label(mode==NrfControl::Drone?NrfUi::drone:NrfUi::car,8);screen.spr.drawFastHLine(20,55,200,TFT_WHITE);
   label(online?NrfUi::online:NrfUi::offline,63,online?TFT_GREEN:TFT_YELLOW);
   label(armed&&(feedback[7]&1)?NrfUi::armed:armed||hold?NrfUi::arming:remoteArmed?NrfUi::stopwaiting:NrfUi::locked,105,armed&&(feedback[7]&1)?TFT_GREEN:TFT_WHITE);
   if(page==0){
    if(mode==NrfControl::Drone){label(NrfUi::throttleoutput,153);value(String(NrfControl::throttle(keys.kvs.L_knob))+" / "+String(armed?NrfControl::throttle(keys.kvs.L_knob):0)+" %",193);value("ROLL "+String(controls.RX)+" P "+String(-controls.RY),242);value("YAW "+String(controls.LX),279);}
    else {int left=0,right=0;if(armed)NrfControl::mix(-controls.LY,controls.RX,limit,left,right);label(NrfUi::steering,153);value("T "+String(-controls.LY)+" S "+String(controls.RX),193);value("L "+String(left)+" R "+String(right),240);label(NrfUi::speedlimit,272);value(String(limit)+" %",309);}
    // Both plots use the same size and geometry as the smooth key-test screen.
    UiLayout::joystick(screen.spr,10,348,keys.kvs.LX,keys.kvs.LY);UiLayout::joystick(screen.spr,129,348,keys.kvs.RX,keys.kvs.RY);
    label(int32_t(blockedUntil-now)>0?NrfUi::stopfirst:NrfUi::armhint,452,int32_t(blockedUntil-now)>0?TFT_YELLOW:TFT_WHITE);label(NrfUi::controlnav,492);
   }else if(page==1){
    label(NrfUi::tx,155);value(String(sent)+" / "+String(ack),193);
    label(NrfUi::success,230);value(sent?String(100.0f*ack/sent,1)+" %":"--",268);
    label(NrfUi::telemetry,307);if(haveFeedback){label((feedback[7]&1)?NrfUi::rxarmed:NrfUi::rxlocked,346,(feedback[7]&1)?TFT_GREEN:TFT_WHITE);value("RX "+String(NrfControl::r32(feedback+8)),389);}else label(NrfUi::none,348,TFT_YELLOW);
    label(int32_t(blockedUntil-now)>0?NrfUi::stopfirst:NrfUi::neutralhint,446,int32_t(blockedUntil-now)>0?TFT_YELLOW:TFT_WHITE);label(NrfUi::controlnav,492);
   }else if(page==3){
    label(NrfUi::hub,146,TFT_CYAN);const uint8_t* names[]={NrfUi::custom,NrfUi::profilebank,NrfUi::defaultmode};
    for(int i=0;i<3;i++){int y=196+i*69;screen.spr.fillRoundRect(10,y,220,62,7,i==menuField?0x0843:TFT_BLACK);screen.spr.drawRoundRect(10,y,220,62,7,i==menuField?TFT_CYAN:TFT_WHITE);label(names[i],y+13,i==menuField?TFT_CYAN:TFT_WHITE);}
    label(NrfUi::menuoperate,447);label(NrfUi::menunav,492);
   }else if(page==4){
    label(NrfUi::joydir,146,TFT_CYAN);
    const uint8_t* names[]={NrfUi::forwardaxis,NrfUi::turnaxis,NrfUi::forwardrev,NrfUi::turnrev};
    const uint8_t* values[]={directions.forwardAxis==1?NrfUi::leftvertical:NrfUi::rightvertical,directions.turnAxis==0?NrfUi::lefthorizontal:NrfUi::righthorizontal,directions.forwardReverse?NrfUi::reversedir:NrfUi::normaldir,directions.turnReverse?NrfUi::reversedir:NrfUi::normaldir};
    for(int i=0;i<4;i++){int y=188+i*62;screen.spr.fillRoundRect(9,y,222,60,6,i==field?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,60,6,i==field?TFT_CYAN:TFT_WHITE);label(names[i],y-4,i==field?TFT_CYAN:TFT_WHITE);label(values[i],y+26,i==field?TFT_CYAN:TFT_WHITE);}
    label(settingNotice&&now-settingNotice<1400?(directionSaved?NrfUi::saved:NrfUi::failed):NrfUi::directionhint,447,directionSaved&&settingNotice&&now-settingNotice<1400?TFT_GREEN:TFT_WHITE);label(NrfUi::settingnav,492);
   }else{
    const uint8_t* labels[]={NrfUi::channel,NrfUi::rate,NrfUi::power,mode==NrfControl::Drone?NrfUi::ratelimit:NrfUi::speedlimit};const char* names[]={"1 Mbps","2 Mbps","250 kbps"};const char* powers[]={"MIN","LOW","HIGH","MAX"};
    String values[]={String(channel)+" / "+String(2400+channel)+" MHz",String(names[rate]),String(powers[power]),String(limit)+" %"};
    for(int i=0;i<4;i++){int y=151+i*70;if(i==field)screen.spr.drawRoundRect(9,y,222,67,7,TFT_CYAN);label(labels[i],y);value(values[i],y+35);}
    label(NrfUi::adjust,447);label(NrfUi::settingnav,492);
   }
   refresh.push((uint16_t*)screen.spr.getPointer());
  }
  delay(1);
 }
 profile.end();radio.stopListening();radio.flush_rx();radio.flush_tx();radio.disableAckPayload();radio.disableDynamicPayloads();nrf.init(ID,oldPower);
 radio.setChannel(oldChannel);radio.setDataRate(oldRate);radio.setPayloadSize(oldSize);radio.setCRCLength(oldCrc);radio.setRetries(5,15);
 screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);return route;
}

// Entry always restores the last explicitly applied route and complete protocol snapshot.
void nrfVehiclePage(uint8_t mode){
 const char* space=mode==NrfControl::Drone?"nrfDrone":"nrfCar";
 int editor=0;
 while(true){
  Preferences prefs;NrfCustom::Selection selected;selected.profile.forwardAxis=mode==NrfControl::Drone?3:1;bool found=false;
  if(prefs.begin(space,false)){
   found=nrfReadSelection(prefs,selected);
   // Upgrade an old applied preset only when its complete saved contents match active.
   if(!prefs.isKey("selected")&&prefs.isKey("lastSlot")){
    uint8_t slot=prefs.getUChar("lastSlot",255);NrfCustom::Profile active=selected.profile,stored=selected.profile;char key[8];snprintf(key,sizeof(key),"slot%u",unsigned(slot)+1);
    if(slot<5&&nrfReadProfile(prefs,"active",active)&&nrfReadProfile(prefs,key,stored)&&memcmp(&active,&stored,sizeof(active))==0){selected.kind=1;selected.slot=slot;selected.profile=stored;found=nrfWriteSelection(prefs,selected);}
   }
   prefs.end();
  }
  int result;
  if(editor){result=nrfGenericPage(mode,editor==2?12:editor==3?9:13,false);editor=0;}
  else result=found&&selected.kind==1?nrfGenericPage(mode,0,true):nrfControlPage(mode);
  if(result==0)return;
  if(result>=2)editor=result;
 }
}
