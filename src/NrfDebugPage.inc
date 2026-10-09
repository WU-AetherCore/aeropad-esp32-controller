// NRF diagnostic screen: NDG1 dedicated test or Jiangxie raw four-byte preset.
void nrfDebug(){
 RadioSession radioSession(RadioNrf);
 auto& radio=nrf.radio;NrfButtons buttons;NrfActionLatch oLatch;NrfAsyncTx asyncTx;
 const uint8_t oldChannel=radio.getChannel(),oldPower=radio.getPALevel(),oldSize=radio.getPayloadSize();
 const auto oldRate=radio.getDataRate();const auto oldCrc=radio.getCRCLength();
 const uint8_t addresses[2][6]={"ADBG0","ADBG1"};
 int channel=76,rate=0,power=0,role=0,period=200,page=0,field=0;bool reference=false;int selectedPreset=0;
 const int intervals[]={100,200,500,1000};const rf24_datarate_e rates[]={RF24_1MBPS,RF24_2MBPS,RF24_250KBPS};
 uint32_t tx=0,ack=0,rx=0,bad=0,seq=0,lastTx=0,lastPaint=0,lastCheck=0,lastRx=0,lastProbe=0;
 uint8_t received[32]={};bool haveRx=false,running=false,present=radio.isChipConnected(),energy=false,dirty=true;
 JoystickNavigation navigation;UiRefresh refresh;
 auto configure=[&](){
  asyncTx.cancel(radio,reference?4:32);radio.stopListening();radio.flush_tx();radio.flush_rx();radio.disableAckPayload();radio.disableDynamicPayloads();for(int i=0;i<6;i++)radio.closeReadingPipe(i);radio.setChannel(channel);radio.setPALevel(power);
  if(!radio.setDataRate(rates[rate])){rate=0;radio.setDataRate(rates[rate]);}
  radio.setAddressWidth(5);radio.setPayloadSize(reference?4:32);radio.setCRCLength(reference?RF24_CRC_8:RF24_CRC_16);radio.setAutoAck(true);
  radio.setRetries(reference?0:1,3);const uint8_t rawAddress[]={0x11,0x22,0x33,0x44,0x55};radio.openWritingPipe(reference?rawAddress:addresses[1-role]);radio.openReadingPipe(1,reference?rawAddress:addresses[role]);radio.startListening();
 };
 Preferences debugPrefs;if(debugPrefs.begin("nrfDbg",true)){reference=debugPrefs.getBool("reference",false);debugPrefs.end();}
 selectedPreset=reference?1:0;if(reference){channel=2;rate=1;}
 if(present)configure();
 screen.spr.unloadFont();screen.spr.resetViewport();screen.spr.setTextSize(1);screen.spr.setTextDatum(TC_DATUM);
 auto label=[&](const uint8_t* bitmap,int y,uint16_t color=TFT_WHITE){screen.spr.drawBitmap(10,y,bitmap,220,36,color);};
 auto value=[&](const String& text,int y,uint16_t color=TFT_WHITE){screen.spr.setTextColor(color,TFT_BLACK);screen.spr.drawString(text,120,y,4);};
 while(true){
  if(present&&asyncTx.poll(radio,millis(),reference?4:32)==1)ack++;
  keys.kvs_update();buttons.sample(keys.kvs,millis());char c=Serial.available()?Serial.read():0;auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());
  if(buttons.pressed(NrfButtons::X)||c=='Q')break;
  if(c=='S'||c=='W')uiScreenshot(true);
  if(buttons.pressed(NrfButtons::B)||c=='B'){page=(page+1)%4;field=0;dirty=true;navigation.reset();refresh.invalidate();}
  if(buttons.pressed(NrfButtons::A)||c=='A'){if(present)asyncTx.cancel(radio,reference?4:32);tx=ack=rx=bad=0;haveRx=false;dirty=true;}
  if(oLatch.update(!keys.kvs.o,millis(),running)||c=='E'){
   if(running){running=false;if(present)asyncTx.cancel(radio,reference?4:32);}
   else if(page==3){reference=selectedPreset==1;channel=reference?2:76;rate=reference?1:0;power=0;role=0;period=200;running=false;tx=ack=rx=bad=seq=0;haveRx=false;if(present)configure();if(debugPrefs.begin("nrfDbg",false)){debugPrefs.putBool("reference",reference);debugPrefs.end();}}
   else if(present){running=!running;if(!running)asyncTx.cancel(radio,reference?4:32);}
   dirty=true;lastTx=millis()-period;
  }
  if(page==3&&(buttons.pressed(NrfButtons::Up)||buttons.pressed(NrfButtons::Down)||c=='U'||c=='D'||move==JoystickNavigation::Up||move==JoystickNavigation::Down)){selectedPreset=1-selectedPreset;dirty=true;}
  if(page==1){
   if(buttons.pressed(NrfButtons::Up)||c=='U'||move==JoystickNavigation::Up){field=(field+4)%5;if(reference&&field==3)field=2;dirty=true;}
   if(buttons.pressed(NrfButtons::Down)||c=='D'||move==JoystickNavigation::Down){field=(field+1)%5;if(reference&&field==3)field=4;dirty=true;}
   int direction=(buttons.pressed(NrfButtons::Right)||c=='R'||move==JoystickNavigation::Right)?1:(buttons.pressed(NrfButtons::Left)||c=='L'||move==JoystickNavigation::Left)?-1:0;
   if(direction&&!(reference&&field==3)){
    if(field==0)channel=(channel+direction+84)%84;
    if(field==1)rate=(rate+direction+3)%3;
    if(field==2)power=(power+direction+4)%4;
    if(field==3&&!reference)role=1-role;
    if(field==4){int i=0;while(i<3&&intervals[i]!=period)i++;period=intervals[(i+direction+4)%4];}
    running=false;if(present)configure();dirty=true;
   }
  }
  uint32_t now=millis();
  if(now-lastCheck>=1000){lastCheck=now;bool found=radio.isChipConnected();if(found!=present){present=found;running=false;if(present)configure();dirty=true;}}
  if(present){
   // Bounded FIFO draining keeps input handling responsive.
   if(asyncTx.poll(radio,now,reference?4:32)==1)ack++;
   for(int i=0;i<3&&radio.available();i++){uint8_t payload[32]={};radio.read(payload,reference?4:32);if(reference||NrfDebugPacket::valid(payload)){memcpy(received,payload,32);rx++;haveRx=true;lastRx=now;}else bad++;}
   if(!asyncTx.busy()&&now-lastProbe>=100){energy=radio.testRPD();lastProbe=now;if(!running){radio.stopListening();radio.startListening();}}
   if(running&&!asyncTx.busy()&&!dirty&&now-lastPaint<100&&uint32_t(now-lastTx)>=uint32_t(period)){
    lastTx=now;uint8_t payload[32];if(reference){for(int i=0;i<4;i++)payload[i]=uint8_t(seq+i);seq++;}else NrfDebugPacket::encode(keys.kvs,seq++,now,payload);
    asyncTx.begin(radio,payload,reference?4:32,now);tx++;
   }
  }
  if(c=='T')Serial.printf("[NRFDBG] present=%d running=%d page=%d channel=%d rate=%d power=%d role=%d period=%d mode=%d TX=%lu ACK=%lu RX=%lu BAD=%lu\n",present,running,page,channel,rate,power,role,period,reference,(unsigned long)tx,(unsigned long)ack,(unsigned long)rx,(unsigned long)bad);
  if(!asyncTx.busy()&&(dirty||now-lastPaint>=100)){dirty=false;lastPaint=now;screen.spr.resetViewport();screen.spr.fillSprite(TFT_BLACK);if(!UiNames::draw(screen.spr,"nrfdebug",10,8,220,false))label(NrfUi::title,8);screen.spr.drawFastHLine(20,55,200,TFT_WHITE);
   label(present?NrfUi::present:NrfUi::absent,62,present?TFT_GREEN:TFT_YELLOW);
   if(page==0){
    screen.spr.drawRoundRect(10,144,220,78,8,TFT_WHITE);screen.spr.drawRoundRect(10,225,220,78,8,TFT_WHITE);screen.spr.drawRoundRect(10,302,220,78,8,TFT_WHITE);
    label(running?NrfUi::running:NrfUi::stopped,106,running?TFT_GREEN:TFT_WHITE);
    label(NrfUi::tx,154,TFT_WHITE);value(String(tx)+" / "+String(ack),194,running?TFT_GREEN:TFT_WHITE);
    label(NrfUi::success,228,TFT_WHITE);value(tx?String(100.0f*ack/tx,1)+" %":"--",268,running?TFT_GREEN:TFT_WHITE);
    label(NrfUi::rx,302,TFT_WHITE);value(String(rx)+" / "+String(bad),342);
    label(energy?NrfUi::hit:NrfUi::quiet,394,energy?TFT_YELLOW:TFT_WHITE);
    label(running?NrfUi::gstop:NrfUi::gstart,440);label(NrfUi::nav,480);
   }else if(page==1){
    const uint8_t* labels[]={NrfUi::channel,NrfUi::rate,NrfUi::power,reference?NrfUi::presetaddr:NrfUi::role,NrfUi::period};
    const char* names[]={"1 Mbps","2 Mbps","250 kbps"};const char* powers[]={"MIN (-18 dBm)","LOW (-12 dBm)","HIGH (-6 dBm)","MAX (0 dBm)"};
    String values[]={String(channel)+" / "+String(2400+channel)+" MHz",String(names[rate]),String(powers[power]),reference?String("1122334455"):String(role)+" : ADBG"+String(role),String(period)+" ms"};
    for(int i=0;i<5;i++){int y=107+i*66;bool editable=!(reference&&i==3);bool selected=editable&&i==field;screen.spr.fillRoundRect(9,y,222,64,7,selected?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,64,7,selected?TFT_CYAN:TFT_WHITE);label(labels[i],y-2,!editable?TFT_WHITE:selected?TFT_CYAN:TFT_WHITE);value(values[i],y+35,!editable?TFT_WHITE:selected?TFT_CYAN:TFT_WHITE);}
    label(running?NrfUi::gstop:NrfUi::gadjust,440);label(NrfUi::nav,480);
   }else if(page==3){
    const uint8_t* names[]={NrfUi::diagpreset,NrfUi::diagreference};for(int i=0;i<2;i++){int y=133+i*126;screen.spr.fillRoundRect(9,y,222,111,8,selectedPreset==i?0x0843:TFT_BLACK);screen.spr.drawRoundRect(9,y,222,111,8,selectedPreset==i?TFT_CYAN:TFT_WHITE);label(names[i],y+8,selectedPreset==i?TFT_CYAN:TFT_WHITE);value(i==0?"CH76 1M 32B":"CH2 2M 4B",y+54);}
    label(running?NrfUi::gstop:NrfUi::presetapply,405);label(NrfUi::gadjust,440);label(NrfUi::nav,480);
   }else if(reference){
    label(NrfUi::diagreference,111,TFT_CYAN);screen.spr.drawRoundRect(10,162,220,82,8,TFT_WHITE);label(NrfUi::tx,168,TFT_WHITE);value(String(tx)+" / "+String(ack),209,running?TFT_GREEN:TFT_WHITE);
    screen.spr.drawRoundRect(10,258,220,154,8,TFT_WHITE);label(NrfUi::rxhex,268,TFT_WHITE);
    if(haveRx){String data;for(int i=0;i<4;i++){char item[4];snprintf(item,sizeof(item),"%02X",received[i]);if(i)data+=' ';data+=item;}value(data,315);value(String(now-lastRx)+" ms",366);}else label(NrfUi::none,322,TFT_WHITE);
    label(running?NrfUi::gstop:NrfUi::ack,440);label(NrfUi::nav,480);
   }else{
    label(NrfUi::local,105,TFT_WHITE);value("LX "+String(keys.kvs.LX)+" LY "+String(keys.kvs.LY),151);value("RX "+String(keys.kvs.RX)+" RY "+String(keys.kvs.RY),189);
    label(NrfUi::remote,227,TFT_WHITE);if(haveRx){value("LX "+String(int8_t(received[12]))+" LY "+String(int8_t(received[13])),271);value("RX "+String(int8_t(received[14]))+" RY "+String(int8_t(received[15])),309);label(NrfUi::last,342,TFT_WHITE);value(String(NrfDebugPacket::get32(received+4))+" / "+String(now-lastRx)+" ms",384);}else label(NrfUi::none,290,TFT_WHITE);
    label(running?NrfUi::gstop:NrfUi::ack,440);label(NrfUi::nav,480);
   }
   screen.spr.resetViewport();if(!UiNames::draw(screen.spr,"nrfdebug",10,8,220,false))label(NrfUi::title,8);screen.spr.drawFastHLine(20,55,200,TFT_WHITE);label(NrfUi::nav,480);
   refresh.push((uint16_t*)screen.spr.getPointer());
  }
  delay(1);
 }
 asyncTx.cancel(radio,reference?4:32);radio.stopListening();radio.flush_tx();radio.flush_rx();nrf.init(ID,oldPower);
 radio.setChannel(oldChannel);radio.setDataRate(oldRate);radio.setPayloadSize(oldSize);radio.setCRCLength(oldCrc);radio.setRetries(5,15);
 screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
