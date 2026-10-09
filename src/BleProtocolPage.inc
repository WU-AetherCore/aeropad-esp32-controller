// BLE UART protocol workspace: editor draft, five slots and applied snapshot are separate.
struct BleProtocolSelection {uint8_t version=1,slot=255;BleControl::Profile profile;};
void bluetoothProtocolControl(){
 RadioSession radioSession(RadioModule);bleModule.stream(false);
 BleControl::Profile draft,active;BleProtocolSelection selected;Preferences prefs;uint8_t activeSlot=255;
 auto read=[&](const char* name,BleControl::Profile& result){uint8_t bytes[sizeof(BleControl::Profile)];size_t length=prefs.getBytesLength(name);return length<=sizeof(bytes)&&length>0&&prefs.getBytes(name,bytes,length)==length&&BleControl::restore(bytes,length,result);};
 if(prefs.begin("bleprotocol",true)){read("draft",draft);uint8_t bytes[sizeof(selected)];size_t length=prefs.getBytesLength("selected");if((length==sizeof(selected)||length==112)&&prefs.getBytes("selected",bytes,length)==length&&bytes[0]==1&&(bytes[1]<5||bytes[1]==255)&&BleControl::restore(bytes+2,length-2,active))activeSlot=bytes[1];prefs.end();}
 bleModule.protocol(active,true);bool occupied[5]={};
 auto reload=[&](){if(!prefs.begin("bleprotocol",true))return;for(int i=0;i<5;i++){char name[8];snprintf(name,sizeof(name),"slot%d",i+1);BleControl::Profile p;occupied[i]=read(name,p);}prefs.end();};reload();
 int page=0,field=0,slot=0,action=0,device=0;bool pending=false,confirm=false;uint32_t changed=0,paint=0,noticeEnd=0;const uint8_t* notice=nullptr;uint16_t noticeColor=TFT_GREEN;
 NrfButtons buttons;NrfExitChord chord;NrfUiPolicy::HoldB holdB;NrfActionLatch oLatch;JoystickNavigation nav;UiRefresh refresh;
 auto message=[&](const uint8_t* text,uint16_t color=TFT_GREEN){notice=text;noticeColor=color;noticeEnd=millis()+1800;refresh.invalidate();};
 auto saveDraft=[&](){bool ok=prefs.begin("bleprotocol",false);if(ok){ok=prefs.putBytes("draft",&draft,sizeof(draft))==sizeof(draft);prefs.end();}pending=!ok;changed=millis();if(!ok)message(NrfUi::failed,TFT_RED);};
 auto edit=[&](){pending=true;changed=millis();confirm=false;refresh.invalidate();};
 auto applyProfile=[&](const BleControl::Profile& p,uint8_t index){if(!BleControl::valid(p)){message(NrfUi::failed,TFT_RED);return;}BleProtocolSelection s;s.profile=p;s.slot=index;bool ok=prefs.begin("bleprotocol",false);if(ok){ok=prefs.putBytes("selected",&s,sizeof(s))==sizeof(s);prefs.end();}if(ok&&bleModule.protocol(p,true)){active=p;activeSlot=index;page=1;field=0;nav.reset();message(NrfUi::defaultsaved);}else message(NrfUi::failed,TFT_RED);};
 // Draw our fixed-size glyphs directly into RGB565 sprite memory. Name marquee
 // viewports must never clip unrelated Chinese titles or operation guides.
 auto label=[&](const uint8_t* text,int y,uint16_t color=TFT_WHITE){
  auto* pixels=static_cast<uint16_t*>(screen.spr.getPointer());if(!pixels||y<0||y+36>536)return;
  uint16_t swapped=uint16_t((color>>8)|(color<<8));
  for(int row=0;row<36;row++)for(int x=0;x<220;x++)if(pgm_read_byte(text+row*28+x/8)&(128>>(x&7)))pixels[(y+row)*240+10+x]=swapped;
 };
 auto value=[&](const String& text,int y,uint16_t color=TFT_CYAN){screen.spr.setTextColor(color,TFT_BLACK);screen.spr.drawString(text,120,y,4);};
 const uint8_t* kinds[]={NrfUi::bpc30d,NrfUi::bpc30d,NrfUi::bpc30d,NrfUi::bpcustom};
 const uint8_t* savedNames[]={NrfUi::savedslot1,NrfUi::savedslot2,NrfUi::savedslot3,NrfUi::savedslot4,NrfUi::savedslot5};
 screen.spr.unloadFont();screen.spr.resetViewport();screen.spr.setTextSize(1);screen.spr.setTextDatum(TC_DATUM);
 while(true){
  uint32_t now=millis();keys.kvs_update();bleModule.update(keys.kvs);buttons.sample(keys.kvs,now);auto status=bleModule.snapshot();bool sending=status.streaming;
  char cmd=Serial.available()?Serial.read():0;bool bHold=holdB.update(!keys.kvs.b,!keys.kvs.x,now);bool exitChord=chord.update(!keys.kvs.b,!keys.kvs.x,now);bool o=oLatch.update(!keys.kvs.o,now,sending)||cmd=='E';
  if(cmd=='S'||cmd=='W'){if(sending)Serial.println("[SCREEN] busy: stop first");else uiScreenshot(true);}
  if(cmd=='T')Serial.printf("[BLEPROTO] page=%d field=%d slot=%d action=%d kind=%u active=%u defaultslot=%u length=%u checksum=%u interval=%u speed=%u angular=%u map0=%u byte0=%u saved=%d occupied=%d streaming=%d state=%d TX=%u ERR=%u limit=%u target=%d speedknob=%u turnknob=%u Lknob=%d Rknob=%d ready=%d error=%d RX=%u feedbackAge=%u\n",page,field,slot+1,action,draft.kind,active.kind,activeSlot,draft.length,draft.checksum,draft.interval,BleControl::speed(draft),BleControl::angular(draft),draft.map[0],draft.bytes[0],!pending,occupied[slot],sending,status.state,status.tx,status.errors,BleControl::limit(active,keys.kvs),sending?BleControl::linear(active,keys.kvs):0,active.speedKnob,active.turnKnob,keys.kvs.L_knob,keys.kvs.R_knob,BleControl::startReady(active,keys.kvs),status.errorCode,status.rx,status.lastFeedback?millis()-status.lastFeedback:0);
  if(cmd=='T')Serial.printf("[CONTROL HEALTH] reason=%d carAge=%u wheelL=%d wheelR=%d stage=%s turn=%d unconfirmed=%d\n",int(status.stopReason),status.carFeedback?millis()-status.lastCarFeedback:0,status.wheelLeft,status.wheelRight,status.stage,BleControl::turnPercent(active,keys.kvs),status.stopUnconfirmed);
  if(cmd=='T')Serial.printf("[CONTROL HEALTH] reason=%d carAge=%u wheelL=%d wheelR=%d stage=%s turn=%d unconfirmed=%d\n",int(status.stopReason),status.carFeedback?millis()-status.lastCarFeedback:0,status.wheelLeft,status.wheelRight,status.stage,BleControl::turnPercent(active,keys.kvs),status.stopUnconfirmed);
  if(cmd=='T'&&page==2){for(int i=0;i<status.count;i++)Serial.printf("[DEVICE %d] %s %s RSSI=%d UART=%d\n",i,status.devices[i].address,status.devices[i].name,status.devices[i].rssi,status.devices[i].uart);}
  bool a=buttons.pressed(NrfButtons::A)||cmd=='A';
  if(page==1){
   if(!keys.kvs.a||a){bleModule.stream(false);sending=false;o=false;}
   if(exitChord||cmd=='Q'){bleModule.stream(false);page=0;field=0;nav.reset();refresh.invalidate();}
   else if(bHold||cmd=='B'){if(sending)message(NrfUi::stopfirst,TFT_YELLOW);else{page=0;field=0;nav.reset();refresh.invalidate();}}
   else if(o){if(sending)bleModule.stream(false);else if(status.state!=BleModule::Connected)message(NrfUi::bplinkfirst,TFT_YELLOW);else if(!BleControl::centered(active,keys.kvs))message(NrfUi::bpcenter,TFT_YELLOW);else if(!BleControl::startReady(active,keys.kvs))message(NrfUi::bpknobminimum,TFT_YELLOW);else bleModule.stream(true);}
  }else{
   auto move=nav.update(keys.kvs.LX,keys.kvs.LY,now);bool up=buttons.pressed(NrfButtons::Up)||cmd=='U'||move==JoystickNavigation::Up;bool down=buttons.pressed(NrfButtons::Down)||cmd=='D'||move==JoystickNavigation::Down;
   int d=(buttons.pressed(NrfButtons::Right)||cmd=='R'||move==JoystickNavigation::Right)?1:(buttons.pressed(NrfButtons::Left)||cmd=='L'||move==JoystickNavigation::Left)?-1:0;
   if(buttons.pressed(NrfButtons::X)||cmd=='Q'){if(page==0)break;page=0;field=0;confirm=false;nav.reset();refresh.invalidate();o=false;}
   int count=page==0?6:page==2?max(1,status.count):page==3?9:page==4||page==8||page==9?32:page==5?5:page==6?2:4;
   if(up||down){field=(field+count+(down?1:-1))%count;confirm=false;refresh.invalidate();}
   if(page==2){device=min(field,max(0,status.count-1));if(a){bleModule.scan();field=device=0;}}
   else if(page==5){slot=field;if(d){action=(action+d+3)%3;confirm=false;refresh.invalidate();}}
   else if(page==3&&d){
    bool usable=(field==0&&draft.kind!=BleControl::Custom)||(field==1&&draft.kind==BleControl::RosVelocity)||field==2||field==5||(field>=6&&draft.kind!=BleControl::Custom)||((field==3||field==4)&&draft.kind==BleControl::Custom);
    if(!usable)message(NrfUi::bpunused,TFT_YELLOW);
    else{
     if(field==0)BleControl::setSpeed(draft,constrain(int(BleControl::speed(draft))+d*50,0,3500));
     if(field==1)BleControl::setAngular(draft,constrain(int(BleControl::angular(draft))+d*100,0,3000));
     if(field==2)draft.interval=(draft.interval+d+9)%9;
     if(field==3)draft.length=constrain(int(draft.length)+d,draft.checksum?2:1,32);
     if(field==4){draft.checksum=(draft.checksum+d+4)%4;if(draft.checksum&&draft.length<2)draft.length=2;}
     if(field==5)draft.deadzone=constrain(int(draft.deadzone)+d,0,50);
     if(field==6)draft.speedKnob=(draft.speedKnob+d+3)%3;
     if(field==7)draft.turnKnob=(draft.turnKnob+d+3)%3;
     if(field==8)draft.kind=(draft.kind+d+3)%3;edit();
    }
   }else if(page==7&&d){if(field==0)draft.forwardAxis=draft.forwardAxis==1?3:1;if(field==1)draft.turnAxis=draft.turnAxis==0?2:0;if(field==2)draft.forwardReverse^=1;if(field==3)draft.turnReverse^=1;edit();}
   else if((page==4||page==8||page==9)&&d&&draft.kind==BleControl::Custom){if(page==4)draft.map[field]=(draft.map[field]+d+32)%32;else if(page==8)draft.bytes[field]=uint8_t(draft.bytes[field]+d);else draft.stopBytes[field]=uint8_t(draft.stopBytes[field]+d);edit();}
   if((buttons.pressed(NrfButtons::B)||cmd=='B')&&(page==3||page==7||page==4||page==8||page==9)){page=page==3?7:page==7?3:page==4?8:page==8?9:4;field=0;refresh.invalidate();o=false;}
   if(a&&(page==3||page==4||page==7||page==8||page==9)){page=5;field=slot;action=0;confirm=false;refresh.invalidate();o=false;}
   if(o){
    if(page==0){const int pages[]={1,2,3,4,5,6};page=pages[field];field=0;action=0;confirm=false;if(page==2&&status.state!=BleModule::Connected)bleModule.scan();}
    else if(page==2){if(status.state==BleModule::Connected){bleModule.disconnect();message(NrfUi::bpdisconnect);}else if(status.count&&status.state!=BleModule::Scanning&&status.state!=BleModule::Connecting)bleModule.connect(device);}
    else if(page==6){draft=BleControl::preset(field==0?0:3);edit();applyProfile(draft,255);}
    else if(page==5){
     char name[8];snprintf(name,sizeof(name),"slot%d",slot+1);int requested=action;
     if(requested==2&&!confirm){confirm=true;message(NrfUi::deleteconfirm,TFT_YELLOW);}
     else if(requested==0&&occupied[slot]&&!confirm){confirm=true;message(NrfUi::bpoverwrite,TFT_YELLOW);}
     else if(prefs.begin("bleprotocol",false)){
      bool ok=false;BleControl::Profile loaded;
      if(requested==0){ok=prefs.putBytes(name,&draft,sizeof(draft))==sizeof(draft);if(ok){occupied[slot]=true;action=1;}}
      if(requested==1)ok=read(name,loaded);
      if(requested==2){bool clearDefault=true;if(activeSlot==slot){BleProtocolSelection fallback;fallback.profile=BleControl::preset(0);clearDefault=prefs.putBytes("selected",&fallback,sizeof(fallback))==sizeof(fallback);if(clearDefault){active=fallback.profile;activeSlot=255;bleModule.protocol(active,true);}}ok=clearDefault&&(!prefs.isKey(name)||prefs.remove(name));if(ok)occupied[slot]=false;}
      prefs.end();confirm=false;
      if(requested==1&&ok){draft=loaded;edit();applyProfile(loaded,slot);}
      else message(ok?(requested==0?savedNames[slot]:NrfUi::deletedok):NrfUi::failed,ok?TFT_GREEN:TFT_RED);
     }else message(NrfUi::failed,TFT_RED);
    }else if(page!=1){applyProfile(draft,255);}
    nav.reset();refresh.invalidate();
   }
  }
  if(pending&&uint32_t(now-changed)>=800)saveDraft();
  if(uint32_t(now-paint)>=33){paint=now;status=bleModule.snapshot();screen.spr.resetViewport();screen.spr.fillSprite(TFT_BLACK);label(NrfUi::bptitle,8);screen.spr.drawFastHLine(20,55,200,TFT_WHITE);
   if(page==0){label(NrfUi::bphub,62,TFT_CYAN);const uint8_t* rows[]={NrfUi::bpcontrol,NrfUi::bpconnect,NrfUi::bpparams,NrfUi::bpmapping,NrfUi::profilebank,NrfUi::bpbuiltin};for(int i=0;i<6;i++){int y=107+48*i;screen.spr.drawRoundRect(9,y,222,45,6,field==i?TFT_CYAN:TFT_WHITE);label(rows[i],y+4,field==i?TFT_CYAN:TFT_WHITE);}label(NrfUi::menuoperate,439);label(NrfUi::bphubback,480);}
   else if(page==1){
    if(activeSlot<5){const uint8_t* names[]={NrfUi::using1,NrfUi::using2,NrfUi::using3,NrfUi::using4,NrfUi::using5};label(names[activeSlot],62,TFT_GREEN);}else label(kinds[active.kind],62,TFT_CYAN);
    label(status.state==BleModule::Connected?NrfUi::bpconnected:NrfUi::bpdisconnected,102,status.state==BleModule::Connected?TFT_GREEN:TFT_YELLOW);
    label(status.stopUnconfirmed?NrfUi::bpstopunconfirmed:status.streaming?NrfUi::controlrunning:status.stopReason==BleModule::FeedbackStop?NrfUi::bpfeedbackstop:status.stopReason==BleModule::InputStop?NrfUi::bpinputstop:status.stopReason==BleModule::WriteStop?NrfUi::bpwritestop:status.stopReason==BleModule::ProtocolStop?NrfUi::bpprotocolstop:NrfUi::controlstopped,140,status.stopUnconfirmed?TFT_RED:status.streaming?TFT_GREEN:status.stopReason>=BleModule::FeedbackStop?TFT_YELLOW:TFT_WHITE);
    if(active.kind!=BleControl::Custom){label(NrfUi::bplimitnow,174,TFT_CYAN);value(String(BleControl::limit(active,keys.kvs))+" / "+String(BleControl::speed(active))+" mm/s",209,TFT_CYAN);label(NrfUi::bptargetspeed,244);value(String(status.streaming?BleControl::linear(active,keys.kvs):0)+" mm/s",279,status.streaming?TFT_GREEN:TFT_WHITE);}
    else{    screen.spr.setTextDatum(TL_DATUM);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);drawSelectedName(status.peer[0]?status.peer:"--",15,179,210,true,status.peer);screen.spr.resetViewport();screen.spr.setTextDatum(TC_DATUM);
    auto count=[](uint32_t n){return n>=10000?String(n/1000)+"k":String(n);};value("TX "+count(status.tx)+" RX "+count(status.rx),218);value("ERR "+String(status.errors)+" "+String(ControlPacket::periods[active.interval])+"ms",253,TFT_WHITE);
    value("F "+String(BleControl::forward(active,keys.kvs))+" T "+String(BleControl::turn(active,keys.kvs)),289);}

    UiLayout::joystick(screen.spr,10,325,keys.kvs.LX,keys.kvs.LY);UiLayout::joystick(screen.spr,129,325,keys.kvs.RX,keys.kvs.RY);label(status.stopUnconfirmed?NrfUi::bpcutpower:status.streaming?NrfUi::customstop:(!BleControl::startReady(active,keys.kvs)?NrfUi::bpknobminimum:NrfUi::customstart),439);label(NrfUi::controlnav,480);
   }else if(page==2){
    label(status.state==BleModule::Connected?NrfUi::bpconnected:status.state==BleModule::Scanning?NrfUi::bpscanning:status.state==BleModule::Connecting?NrfUi::bpconnecting:NrfUi::bpdisconnected,62,status.state==BleModule::Connected?TFT_GREEN:TFT_YELLOW);
    if(!status.count)label(NrfUi::bpnodevices,200,TFT_WHITE);
    int start=device/4*4;for(int i=start;i<min(start+4,status.count);i++){int y=111+(i-start)*70;bool chosen=i==device;screen.spr.drawRoundRect(9,y,222,64,6,chosen?TFT_CYAN:TFT_WHITE);String name=status.devices[i].name[0]?status.devices[i].name:status.devices[i].address;screen.spr.setTextDatum(TL_DATUM);screen.spr.setTextColor(chosen?TFT_CYAN:TFT_WHITE,TFT_BLACK);drawSelectedName(name.c_str(),15,y+3,210,chosen,status.devices[i].address);screen.spr.resetViewport();screen.spr.setTextDatum(TC_DATUM);value(String(status.devices[i].rssi)+" dBm "+(status.devices[i].uart?"UART":"BLE"),y+34,TFT_WHITE);}
    label(NrfUi::bpleonly,400);label(status.state==BleModule::Connected?NrfUi::bpdisconnectkey:NrfUi::bpscankeys,439);label(NrfUi::bpxback,480);
   }else if(page==5){
    label(NrfUi::profilebank,62,TFT_CYAN);const uint8_t* filled[]={NrfUi::slot1saved,NrfUi::slot2saved,NrfUi::slot3saved,NrfUi::slot4saved,NrfUi::slot5saved};const uint8_t* empty[]={NrfUi::slot1empty,NrfUi::slot2empty,NrfUi::slot3empty,NrfUi::slot4empty,NrfUi::slot5empty};for(int i=0;i<5;i++){int y=107+51*i;screen.spr.drawRoundRect(9,y,222,48,6,i==slot?TFT_CYAN:TFT_WHITE);label(occupied[i]?filled[i]:empty[i],y+6,i==slot?TFT_CYAN:occupied[i]?TFT_GREEN:TFT_WHITE);}const uint8_t* actions[]={NrfUi::saveslot,NrfUi::loadslot,NrfUi::delslot};label(actions[action],359,action==2?TFT_YELLOW:TFT_CYAN);label(action==0?NrfUi::saveguide:action==1?NrfUi::applyguide:NrfUi::deleteguide,400);label(NrfUi::bankoperations,439);label(NrfUi::bpxback,480);
   }else if(page==6){label(NrfUi::bpbuiltin,62,TFT_CYAN);const uint8_t* models[]={NrfUi::bpc30d,NrfUi::bpcustom};for(int i=0;i<2;i++){int y=133+107*i;screen.spr.drawRoundRect(9,y,222,84,9,field==i?TFT_CYAN:TFT_WHITE);label(models[i],y+24,field==i?TFT_CYAN:TFT_WHITE);}label(NrfUi::bppresetnote,400);label(NrfUi::menuoperate,439);label(NrfUi::bpxback,480);}
   else{
    label(page==3?NrfUi::bpparams:page==7?NrfUi::joydir:page==4?NrfUi::bpmapping:page==9?NrfUi::bpstopbytes:NrfUi::bpconstants,62,TFT_CYAN);
    if((page==4||page==8||page==9)&&draft.kind!=BleControl::Custom){label(NrfUi::bpfixed,184,TFT_CYAN);label(NrfUi::bpchoosecustom,237);}
    else{int start=page==7?0:(field/4)*4,count=page==3?9:page==7?4:32;const uint8_t* params[]={NrfUi::bpspeed,NrfUi::bpangular,NrfUi::period,NrfUi::txlength,NrfUi::bpchecksum,NrfUi::bpdead,NrfUi::bpspeedknob,NrfUi::bpturnknob,NrfUi::bpwire};
     for(int i=0;i<min(4,count-start);i++){int k=start+i,y=112+i*71;bool chosen=field==k;screen.spr.drawRoundRect(9,y,222,68,7,chosen?TFT_CYAN:TFT_WHITE);
      if(page==3){label(params[k],y-2,chosen?TFT_CYAN:TFT_WHITE);String v=k==0?String(BleControl::speed(draft))+" mm/s":k==1?String(BleControl::angular(draft))+" mrad/s":k==2?String(ControlPacket::periods[draft.interval])+" ms":k==3?String(draft.kind==BleControl::RosVelocity?11:draft.kind==BleControl::Custom?draft.length:1)+" bytes":k==4?String(draft.kind==BleControl::Custom?(draft.checksum==0?"NONE":draft.checksum==1?"XOR":draft.checksum==2?"SUM8":"NEG SUM8"):"FIXED"):String(draft.deadzone)+" %";if(k>=6){const uint8_t* knobs[]={NrfUi::bpknoboff,NrfUi::bpknobleft,NrfUi::bpknobright};const uint8_t* wires[]={NrfUi::bpapp,NrfUi::bpsteer,NrfUi::bpros};label(draft.kind==BleControl::Custom?NrfUi::bpunusedvalue:k==8?wires[draft.kind]:knobs[k==6?draft.speedKnob:draft.turnKnob],y+31,chosen?TFT_CYAN:TFT_WHITE);}else if((k==0&&draft.kind==BleControl::Custom)||(k==1&&draft.kind!=BleControl::RosVelocity))label(NrfUi::bpunusedvalue,y+31,TFT_WHITE);else if(k==4&&draft.kind!=BleControl::Custom)label(NrfUi::bpfixedvalue,y+31,TFT_WHITE);else value(v,y+32,chosen?TFT_CYAN:TFT_WHITE);}
      else if(page==7){const uint8_t* names[]={NrfUi::forwardaxis,NrfUi::turnaxis,NrfUi::forwardrev,NrfUi::turnrev};const uint8_t* vals[]={draft.forwardAxis==1?NrfUi::leftvertical:NrfUi::rightvertical,draft.turnAxis==0?NrfUi::lefthorizontal:NrfUi::righthorizontal,draft.forwardReverse?NrfUi::reversedir:NrfUi::normaldir,draft.turnReverse?NrfUi::reversedir:NrfUi::normaldir};label(names[k],y-2,chosen?TFT_CYAN:TFT_WHITE);label(vals[k],y+31,chosen?TFT_CYAN:TFT_WHITE);}
      else{value("BYTE "+String(k),y+4,chosen?TFT_CYAN:TFT_WHITE);const char* names[]={"CONST","LX S8","LY S8","RX S8","RY S8","KNOB L S8","KNOB R S8","LX +100","LY +100","RX +100","RY +100","KNOB L +100","KNOB R +100","LX U8","LY U8","RX U8","RY U8","KNOB L U8","KNOB R U8","KEY 0-7","KEY 8-15","KEY 16-17","GYRO X","GYRO Y","SEQ LOW","SEQ HIGH","FORWARD S8","TURN S8","FORWARD +100","TURN +100","FORWARD U8","TURN U8"};char byte[24];uint8_t v=page==9?draft.stopBytes[k]:draft.bytes[k];snprintf(byte,sizeof(byte),"%02X / %u",v,v);value(page==4?String(names[draft.map[k]]):String(byte),y+35,chosen?TFT_CYAN:TFT_WHITE);}
     }
    }
    const uint8_t* groups[]={NrfUi::bpgroupona,NrfUi::bpgroupduo,NrfUi::bpgroupthree};label(pending?NrfUi::saving:page==3?groups[field/4]:NrfUi::saveentry,400);label(NrfUi::gadjust,439);label(page==3?NrfUi::bpparamnav:NrfUi::settingnav,480);
   }
   if(notice&&int32_t(noticeEnd-now)>0){screen.spr.fillRoundRect(6,433,228,40,6,0x0843);screen.spr.drawRoundRect(6,433,228,40,6,noticeColor);label(notice,435,noticeColor);}refresh.push((uint16_t*)screen.spr.getPointer());
  }
  delay(1);
 }
 bleModule.stream(false);if(pending)saveDraft();bleModule.disconnect();uint32_t until=millis();while(bleModule.snapshot().state==BleModule::Connected&&millis()-until<2000)delay(5);bleModule.protocol(active,false);screen.spr.setTextSize(1);screen.spr.setTextDatum(TC_DATUM);screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
