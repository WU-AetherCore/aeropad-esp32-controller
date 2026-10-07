void nrfControlPage(uint8_t mode){
 auto& radio=nrf.radio;NrfButtons buttons;const uint8_t oldChannel=radio.getChannel(),oldPower=radio.getPALevel(),oldSize=radio.getPayloadSize();
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
  uint8_t packet[32];NrfControl::encode(keys.kvs,mode,armed,seq++,session,millis(),limit,packet);sent++;
  if(radio.write(packet,32))ack++;
  for(int i=0;i<3&&radio.available();i++){
   uint8_t n=radio.getDynamicPayloadSize(),payload[32];if(n!=32){if(n&&n<=32)radio.read(payload,n);else radio.flush_rx();continue;}
   radio.read(payload,32);uint16_t age=uint16_t(uint16_t(seq-1)-NrfControl::r16(payload+4));
   uint32_t stamp=payload[27]|uint32_t(payload[28])<<8|uint32_t(payload[29])<<16;
   if(NrfControl::feedbackValid(payload,mode)&&age<=8&&stamp==(session&0xffffff)){memcpy(feedback,payload,32);lastFeedback=millis();haveFeedback=true;}
  }
 };
 while(true){keys.kvs_update();buttons.sample(keys.kvs,millis());uint32_t now=millis();char c=Serial.available()?Serial.read():0;
  if(c=='S'||c=='W')uiScreenshot(true);
  bool online=present&&haveFeedback&&uint32_t(now-lastFeedback)<NrfControl::Timeout&&!(feedback[7]&2);
  if(!online||(armed&&uint32_t(now-armedAt)>=100&&!(feedback[7]&1))){armed=false;hold=false;canArm=false;}
  if(buttons.pressed(NrfButtons::A)||c=='A'){armed=false;hold=false;canArm=false;lastSend=now-20;}
  if(buttons.pressed(NrfButtons::X)||c=='Q'){armed=false;if(present)for(int i=0;i<3;i++)send();break;}
  if(buttons.pressed(NrfButtons::B)||c=='B'){page=(page+1)%3;navigation.reset();refresh.invalidate();}
  int8_t axes[]={keys.kvs.LX,keys.kvs.LY,keys.kvs.RX,keys.kvs.RY,keys.kvs.L_knob,keys.kvs.R_knob};
  bool safe=NrfControl::neutral(mode,axes);
  bool held=!keys.kvs.o;if(!held)canArm=true;
  if(canArm&&held&&!armed&&online&&safe&&page!=2){if(!hold){hold=true;holdStart=now;}if(uint32_t(now-holdStart)>=1000){armed=true;armedAt=now;}}
  else if(!held||!safe)hold=false;
  if(page==2&&!armed){
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
  if(now-lastPaint>=33){lastPaint=now;screen.spr.resetViewport();screen.spr.fillSprite(TFT_BLACK);
   label(mode==NrfControl::Drone?NrfUi::drone:NrfUi::car,8);screen.spr.drawFastHLine(20,55,200,TFT_DARKGREY);
   label(online?NrfUi::online:NrfUi::offline,63,online?TFT_GREEN:TFT_YELLOW);
   label(armed&&(feedback[7]&1)?NrfUi::armed:armed||hold?NrfUi::arming:NrfUi::locked,105,armed&&(feedback[7]&1)?TFT_GREEN:TFT_WHITE);
   if(page==0){
    if(mode==NrfControl::Drone){label(NrfUi::throttleoutput,153);value(String(NrfControl::throttle(keys.kvs.L_knob))+" / "+String(armed?NrfControl::throttle(keys.kvs.L_knob):0)+" %",193);value("ROLL "+String(keys.kvs.RX)+" P "+String(-keys.kvs.RY),242);value("YAW "+String(keys.kvs.LX),279);}
    else {int left=0,right=0;if(armed)NrfControl::mix(-keys.kvs.LY,keys.kvs.RX,limit,left,right);label(NrfUi::steering,153);value("T "+String(-keys.kvs.LY)+" S "+String(keys.kvs.RX),193);value("L "+String(left)+" R "+String(right),240);label(NrfUi::speedlimit,272);value(String(limit)+" %",309);}
    // Both plots use the same size and geometry as the smooth key-test screen.
    UiLayout::joystick(screen.spr,10,348,keys.kvs.LX,keys.kvs.LY);UiLayout::joystick(screen.spr,129,348,keys.kvs.RX,keys.kvs.RY);
    label(NrfUi::armhint,452);label(NrfUi::exitcontrol,492);
   }else if(page==1){
    label(NrfUi::tx,155);value(String(sent)+" / "+String(ack),193);
    label(NrfUi::success,230);value(sent?String(100.0f*ack/sent,1)+" %":"--",268);
    label(NrfUi::telemetry,307);if(haveFeedback){label((feedback[7]&1)?NrfUi::rxarmed:NrfUi::rxlocked,346,(feedback[7]&1)?TFT_GREEN:TFT_WHITE);value("RX "+String(NrfControl::r32(feedback+8)),389);}else label(NrfUi::none,348,TFT_YELLOW);
    label(NrfUi::neutralhint,446);label(NrfUi::exitcontrol,492);
   }else{
    const uint8_t* labels[]={NrfUi::channel,NrfUi::rate,NrfUi::power,mode==NrfControl::Drone?NrfUi::ratelimit:NrfUi::speedlimit};const char* names[]={"1 Mbps","2 Mbps","250 kbps"};const char* powers[]={"MIN","LOW","HIGH","MAX"};
    String values[]={String(channel)+" / "+String(2400+channel)+" MHz",String(names[rate]),String(powers[power]),String(limit)+" %"};
    for(int i=0;i<4;i++){int y=151+i*70;if(i==field)screen.spr.drawRoundRect(9,y,222,67,7,TFT_CYAN);label(labels[i],y);value(values[i],y+35);}
    label(NrfUi::adjust,447);label(NrfUi::exitcontrol,492);
   }
   refresh.push((uint16_t*)screen.spr.getPointer());
  }
  delay(1);
 }
 profile.end();radio.stopListening();radio.flush_rx();radio.flush_tx();radio.disableAckPayload();radio.disableDynamicPayloads();nrf.init(ID,oldPower);
 radio.setChannel(oldChannel);radio.setDataRate(oldRate);radio.setPayloadSize(oldSize);radio.setCRCLength(oldCrc);radio.setRetries(5,15);
 screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
