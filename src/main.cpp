#include <Arduino.h>
/*
 Name:		_1.ino
 Created:	2023/9/8 15:15:20
 Author:	bilibili-黑人黑科技
*/

// 包含库
//-------------------------------------------------------------------------------------------------------------
#include "NRF.h"
#include "NrfUiText.h"
#include "NrfDebugPacket.h"
#include "NrfGenericConfig.h"
#include "NrfButtons.h"
#include "NrfActionLatch.h"
#include "NrfAsyncTx.h"
#include "NrfControlProtocol.h"
#include "Keys.h"
#include "Screen.h"
#include "Bluetooth.h"
#include "BleModule.h"
#include "NetworkPortal.h"
#include <WiFi.h>
#include <esp_heap_caps.h>
#include <esp_ota_ops.h>
#include "LED.h"
#include "Buzzer.h"
#include "controller_keys.h" // 按键引脚定义

#include "chinese_32.h"  // 中文字库
#include "UiLayout.h"
#include "JoystickNavigation.h"
#include "CubeGeometry.h"
#include "ControlPacket.h"
#include "LocalGames.h"
#include "PuzzleGames.h"
#include "Sokoban.h"
#include <Preferences.h>
#include "generated_ui_text.h" // BLE 页面完整中文位图字形



// 包含图标：一级菜单
#include "icons/1nrf.h"
#include "icons/2game.h"
#include "icons/4info.h"
#include "icons/5ble.h"
#include "icons/6set.h"

// 包含图标：二级菜单
#include "icons/1nrf/4drone.h"
#include "icons/2game/2_1snake.h"
#include "icons/2game/2_2brick.h"
#include "icons/2game/2_3plane.h"
#include "icons/2game/2_4num2048.h"
#include "icons/2game/2_5tetris.h"
#include "icons/6set/6_1_keysTest.h"
#include "icons/6set/6_2_cube.h"


// 全局变量
//-------------------------------------------------------------------------------------------------------------
int ID = 0;		  // 遥控器ID  0:黑 1:壮(默认0)




// 实例化对象
//-------------------------------------------------------------------------------------------------------------
NRF nrf;		 // 通信模块
Keys keys;	     // 按键
Screen screen;   // 屏幕
Bluetooth bt;	 // 蓝牙
BleModule bleModule;
WIFI wifi;	     // WiFi
LED led;		 // 板载LED
Buzzer buzzer;   // 蜂鸣器



// 函数声明
//-------------------------------------------- 一级菜单  -------------------------------------------------------
//-------------------------------------------------------------------------------------------------------------
void menu();			// 0.主菜单     ->   1.NRF遥控  2.本机游戏    4.网络信息  5.蓝牙手柄  6.系统设置
void nrfControlPage(uint8_t mode);
void nrfDebug();
void nrfGeneric();
void NRFControl();		// 1.NRF遥控    ->   无人机 / 四驱车
void localGame();		// 贪吃蛇、打砖块、飞机大战、2048、俄罗斯方块、推箱子
void netInfo();			// 4.网络信息   ->   WiFi管理
void wifiSettings();                 // 4.4 WiFi 管理：热点、网页配网与设备数据
void deviceMonitor();                // 6.4 设备监测：内存、硬件、网络和蓝牙统计
void joystickCalibration();
void bluetoothMenu();
void bluetoothModules();
void bluetoothOutputSettings();
void bluetoothBaudSettings();
void serialSettings();
void bluetoothModuleSettings();
void bluetoothLayoutPreview();
void btGamepad();		// 5.蓝牙手柄
void systemSet();		// 6.系统设置   ->   1.按键测试 2.陀螺仪立方体

//--------------------------------------------- 二级菜单 -------------------------------------------------------
//---------------------------------------------1.NRF遥控--------------------------------------------------------
void drone();			// 1.4 无人机

//---------------------------------------------2.本机游戏-------------------------------------------------------
void snake();			// 2.1 贪吃蛇
void brick();			// 2.2 打砖块
void plane();			// 2.3 飞机大战
void num2048();			// 2.4 2048
void tetris();			// 2.5 俄罗斯方块
void sokoban();             // 2.6 推箱子：十关挑战、连续撤销、庆祝与自动下一关

//----------------------------------------------------------------------------------------------------

//----------------------------------------------4.网络信息------------------------------------------------------

//----------------------------------------------5.蓝牙手柄------------------------------------------------------
void btGamepad();                    // 5.1 蓝牙手柄：实时摇杆、按键与回显
void bluetoothMenu();                // 5.2 蓝牙中心主菜单
void bluetoothModules();             // 5.3 搜索、连接与模块控制
void bluetoothModuleSettings();      // 5.4 模块子菜单：串口/发送设置
void serialSettings();               // 5.5 本机回显格式与对方串口设置
void bluetoothOutputSettings();      // 5.6 BLE 遥控输出格式与间隔
void bluetoothBaudSettings();        // 5.7 对方 UART 波特率设置
void bluetoothLayoutPreview();       // 5.8 串口布局回归预览

//----------------------------------------------6.系统设置------------------------------------------------------
void keysTest();		// 6.1 按键测试
void cube();			// 6.2 陀螺仪立方体


//----------------------------------------------工具方法------------------------------------------------------
int getVolADC();					// 立刻获取电压ADC值
int getVol();						// 立刻获取电压值（mV）
int setGetVolTimer(int time);		// 设置周期性获取电压值（时间ms），返回定时器的ID
void closeGetVolTimer(int timerID); // 关闭周期性获取电压（定时器ID）



void setup() {
	Serial.begin(115200);
	delay(1000);
	Serial.println("[AeroPad] CLion firmware starting");
	Serial.printf("[Memory] Flash=%u PSRAM=%u FreeHeap=%u\n",
	              ESP.getFlashChipSize(), ESP.getPsramSize(), ESP.getFreeHeap());

	nrf.init(ID, 0);			 // (遥控器ID, 通信功率，0-3)
	keys.init(ID);
	screen.init();
	buzzer.init();
	led.init();
    wifi.begin();                    // 后台维护网页服务及已保存路由器连接

	// 基本功能测试
	{
		//nrf.testConToCon();		   // 连遥控器间的测试连接 √
		//keysTest();				   // 按键测试,显示kvs到小屏幕上 √
		//keys.dounnceTest();		   // 按键消抖测试 √
	}


	screen.spr.loadFont(chinese_32);   // 加载自定义中文字库
	Serial.println("[AeroPad] Ready: main menu");

	menu();									// 主菜单
}


void loop() {
	//con.keys.kvs_update();         // √
	//con.keys.ShowInSerial();		 // √	
	delay(1000);
}














// ----------------------------------------- 以下是功能的具体实现 -----------------------------------------------

//-------------------------------------------- 一级菜单  -------------------------------------------------------
// 0.主菜单     ->   1.NRF遥控  2.本机游戏    4.网络信息  5.蓝牙手柄  6.系统设置
// Shared menu layout and serial inspection: R/L selection, E enter, Q back, S screenshot.
// Compare small row bands in PSRAM, transferring only changed screen regions.
class UiRefresh {
public:
    UiRefresh():previous((uint16_t*)ps_malloc(240*536*2)) {}
    ~UiRefresh(){free(previous);}
    void invalidate(){first=true;}
    void push(uint16_t* pixels) {
        // Any other page's LCD writes invalidate this page's comparison cache.
        if(lcd_frame_epoch()!=observedEpoch)first=true;
        if(!previous) {lcd_PushColors(0,0,240,536,pixels);return;}
        for(int y=0;y<536;y+=16) {
            int rows=min(16,536-y);size_t bytes=rows*240*2;
            if(first || memcmp(previous+y*240,pixels+y*240,bytes)) {
                lcd_PushColors(0,y,240,rows,pixels+y*240);
                memcpy(previous+y*240,pixels+y*240,bytes);
            }
        }
        first=false;
        observedEpoch=lcd_frame_epoch();
    }
private:
    uint16_t* previous; bool first=true;uint32_t observedEpoch=0;
};

void uiScreenshot(bool sentFrame=false) {
    Serial.print("FRAME240x536\n");
    const uint8_t *pixels=(const uint8_t*)(sentFrame?lcd_last_transmitted_frame():screen.spr.getPointer());
    if(!pixels)return;
    size_t sent=0;const uint32_t start=millis();
    while(sent<257280 && millis()-start<15000) {
        size_t count=Serial.write(pixels+sent,min((size_t)1024,(size_t)257280-sent));
        sent+=count;if(!count)delay(1);
    }
    Serial.flush();
}

void uiFooter(bool menuPage) {
    screen.spr.setSwapBytes(true);
    if(menuPage) { screen.spr.pushImage(10,440,220,44,ui_switch); screen.spr.pushImage(10,488,220,44,ui_enter); }
    else screen.spr.pushImage(10,484,220,44,ui_exit);
}
const uint16_t nrfGenericTitle[1]={0};
const uint16_t nrfDebugTitle[1]={0}; // Menu sentinel: compact monochrome title.
void selectMenu(const uint16_t *const *titles, const uint16_t *const *icons, void (**actions)(), int count, bool root=false, const uint16_t* category=nullptr) {
    int index=0; bool dirty=true;
    JoystickNavigation navigation;
    while(true) {
        char cmd=Serial.available()?Serial.read():0;
        if(cmd=='S') uiScreenshot();
        JoystickNavigation::Direction move=JoystickNavigation::None;
        if(!root) {keys.kvs_update();move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());}
        if(keys.left.pressed() || keys.up.pressed() || cmd=='L' || move==JoystickNavigation::Left || move==JoystickNavigation::Up) { index=(index+count-1)%count; dirty=true; }
        if(keys.right.pressed() || keys.down.pressed() || cmd=='R' || move==JoystickNavigation::Right || move==JoystickNavigation::Down) { index=(index+1)%count; dirty=true; }
        if(!root && (keys.x.pressed() || cmd=='Q')) break;
        if(root && cmd=='J') { joystickCalibration(); dirty=true; }
        if(root && cmd=='B') { btGamepad(); dirty=true; }
        if(root && cmd=='K') { keysTest(); dirty=true; }
        if(root && cmd=='H') { cube(); dirty=true; }
        if(root && cmd=='V') { bluetoothModules(); dirty=true; }
        if(root && cmd=='I') { bluetoothLayoutPreview(); dirty=true; }
        if(cmd=='P') keys.printCalibration();
        if(dirty) {
            screen.spr.unloadFont(); screen.spr.setSwapBytes(true); screen.spr.fillSprite(TFT_BLACK);
            screen.spr.pushImage(10,12,220,44,category?category:root?ui_m0:ui_menugroup);
            screen.spr.drawFastHLine(20,68,200,TFT_DARKGREY);
            if(titles[index]==nrfGenericTitle) screen.spr.drawBitmap(10,94,NrfUi::generic,220,36,TFT_WHITE);
            else if(titles[index]==nrfDebugTitle) screen.spr.drawBitmap(10,94,NrfUi::title,220,36,TFT_WHITE);
            else screen.spr.pushImage(10,92,220,44,titles[index]);
            if(icons && icons[index]) screen.spr.pushImage(20,178,200,200,icons[index]);
            else {
                if(titles[index]==ui_module) {
                    screen.spr.fillRoundRect(78,208,84,136,12,0x0843);
                    screen.spr.drawRoundRect(78,208,84,136,12,TFT_CYAN);
                    screen.spr.fillTriangle(112,227,112,272,145,248,TFT_CYAN);
                    screen.spr.fillTriangle(112,282,112,327,145,305,TFT_CYAN);
                    screen.spr.fillRect(108,226,5,102,TFT_CYAN);
                    screen.spr.fillTriangle(96,278,113,266,113,290,TFT_CYAN);
                } else if(titles[index]==ui_sendsettings) {
                    const int knob[]={87,150,112};
                    for(int i=0;i<3;i++){screen.spr.fillRoundRect(48,230+i*42,144,5,2,TFT_CYAN);screen.spr.fillCircle(knob[i],232+i*42,12,TFT_WHITE);screen.spr.fillCircle(knob[i],232+i*42,5,TFT_CYAN);}
                } else if(titles[index]==nrfGenericTitle) {
                    for(int i=0;i<3;i++){screen.spr.drawFastHLine(44,218+i*54,152,TFT_CYAN);screen.spr.fillCircle(75+i*38,218+i*54,13,TFT_WHITE);screen.spr.fillCircle(75+i*38,218+i*54,6,TFT_CYAN);}
                } else if(titles[index]==nrfDebugTitle) {
                    screen.spr.drawRoundRect(42,204,156,146,14,TFT_CYAN);
                    screen.spr.drawRoundRect(48,210,144,134,10,TFT_CYAN);
                    screen.spr.fillRect(106,350,28,17,TFT_WHITE);
                    for(int j=0;j<4;j++) screen.spr.fillRect(70+j*27,312-j*20,15,16+j*20,j==3?TFT_GREEN:TFT_CYAN);
                    screen.spr.drawCircle(120,244,17,TFT_WHITE);
                    screen.spr.drawCircle(120,244,25,TFT_CYAN);
                } else if(titles[index]==ui_car) {
                    screen.spr.fillRoundRect(80,255,80,52,10,TFT_CYAN);
                    for(int x: {65,157}) for(int y: {250,291}) screen.spr.fillRoundRect(x,y,18,28,4,TFT_WHITE);
                } else {
                    screen.spr.drawCircle(120,288,42,TFT_CYAN);
                    screen.spr.drawFastHLine(65,288,110,TFT_CYAN);
                    screen.spr.drawFastVLine(120,233,110,TFT_CYAN);
                    screen.spr.fillCircle(120,288,8,TFT_WHITE);
                }
            }
            screen.spr.setTextDatum(TC_DATUM);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);
            char position[16];snprintf(position,sizeof(position),"%02d / %02d",index+1,count);screen.spr.drawString(position,120,394,4);
            for(int i=0;i<count;i++) screen.spr.fillCircle(120+(i*2-count+1)*16,428,3,index==i?TFT_CYAN:TFT_DARKGREY);
            screen.spr.drawFastHLine(20,434,200,TFT_DARKGREY);
            uiFooter(true);
            if(!root) screen.spr.pushImage(10,440,220,44,ui_joyselect);
            if(root) screen.spr.pushImage(10,488,220,44,ui_rootenter);
            lcd_PushColors(0,0,240,536,(uint16_t*)screen.spr.getPointer()); dirty=false;
        }
        if(keys.o.pressed() || cmd=='E') { actions[index](); navigation.reset(); dirty=true; }
        delay(10);
    }
    screen.spr.loadFont(chinese_32); screen.spr.setTextDatum(TC_DATUM); screen.spr.fillSprite(TFT_BLACK);
}
void car() { nrfControlPage(NrfControl::Car); }
void menu() {
    const uint16_t *titles[]={ui_m1,ui_m2,ui_m3,ui_blehub,ui_m4};
    const uint16_t *icons[]={image_data_1nrf,image_data_2game,image_data_4info,image_data_5ble,image_data_6set};
    void (*actions[])()={NRFControl,localGame,netInfo,bluetoothMenu,systemSet};
    selectMenu(titles,icons,actions,5,true);
}
void NRFControl() {
    const uint16_t *titles[]={ui_drone,ui_car,nrfDebugTitle,nrfGenericTitle}; const uint16_t *icons[]={image_data_1_4drone,nullptr,nullptr,nullptr};
    void (*actions[])()={drone,car,nrfDebug,nrfGeneric}; selectMenu(titles,icons,actions,4,false,ui_m1);
}
#include "NrfDebugPage.inc"
#include "NrfGenericPage.inc"
#include "NrfControlPage.inc"

void localGame() {
    const uint16_t *titles[]={ui_m7,ui_m8,ui_m9,ui_m10,ui_m11,ui_sokoban};
    const uint16_t *icons[]={image_data_2_1snake,image_data_2_2brick,image_data_2_3plane,image_data_2_4num2048,image_data_2_5tetris,ui_sokoicon};
    void (*actions[])()={snake,brick,plane,num2048,tetris,sokoban}; selectMenu(titles,icons,actions,6,false,ui_m2);
}
void netInfo() {
    const uint16_t *titles[]={ui_wifimanage};
    const uint16_t *icons[]={ui_wifiicon};
    void (*actions[])()={wifiSettings};
    selectMenu(titles,icons,actions,1,false,ui_m3);
}

void wifiSettings(){
    wifi.begin();UiRefresh refresh;screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true){keys.kvs_update();char cmd=Serial.available()?Serial.read():0;
        if(keys.x.pressed()||cmd=='Q')break;if(keys.o.pressed()||cmd=='E')wifi.toggleHotspot();
        if(keys.a.pressed()||cmd=='A')wifi.setAutoConnect(!wifi.autoConnect());
        if(cmd=='T')Serial.printf("[WIFI] hotspot=%d station=%d ap_ip=%s sta_ip=%s clients=%u auto=%d\n",wifi.hotspot(),WiFi.status(),WiFi.softAPIP().toString().c_str(),WiFi.localIP().toString().c_str(),WiFi.softAPgetStationNum(),wifi.autoConnect());
        if(cmd=='S')uiScreenshot();
        screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,8,220,44,ui_wifimanage);
        screen.spr.drawFastHLine(20,64,200,TFT_DARKGREY);
        screen.spr.drawRoundRect(6,78,228,206,12,wifi.hotspot()?TFT_CYAN:TFT_DARKGREY);
        screen.spr.pushImage(10,83,220,44,wifi.hotspot()?ui_hotspoton:ui_hotspotoff);
        screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);screen.spr.drawString("AeroPad-Setup",120,131,4);
        screen.spr.pushImage(10,162,220,44,ui_wifipassword);screen.spr.drawString("12345678",120,204,4);
        screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString("192.168.4.1",120,246,4);
        bool connected=WiFi.status()==WL_CONNECTED;
        screen.spr.drawRoundRect(6,296,228,98,12,connected?TFT_GREEN:TFT_DARKGREY);
        screen.spr.pushImage(10,300,220,44,connected?ui_routeron:ui_routeroff);
        screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);screen.spr.drawString(connected?WiFi.localIP().toString():"--",120,350,4);
        screen.spr.pushImage(10,400,220,44,wifi.autoConnect()?ui_bootwifi_on:ui_bootwifi_off);screen.spr.pushImage(10,444,220,44,ui_wifibootkeys);screen.spr.pushImage(10,488,220,44,ui_exit);
        refresh.push((uint16_t*)screen.spr.getPointer());delay(20);
    }screen.spr.loadFont(chinese_32);
}

String uiDeviceName(const char* name,int width=204) {
    String text=name;
    bool shortened=false;
    while(text.length() && screen.spr.textWidth(text,4)>width){text.remove(text.length()-1);shortened=true;}
    if(shortened){while(text.length()&&screen.spr.textWidth(text+"..",4)>width)text.remove(text.length()-1);text+="..";}
    return text;
}
void drawSelectedName(const char* name,int x,int y,int width,bool selected,const char* identity) {
    static String previous;static uint32_t started=0;
    if(selected && previous!=identity){previous=identity;started=millis();}
    int excess=screen.spr.textWidth(name,4)-width;
    if(!selected||excess<=0){screen.spr.drawString(uiDeviceName(name,width),x,y,4);return;}
    uint32_t travel=uint32_t(excess)*40,cycle=travel+2600;
    uint32_t phase=(millis()-started)%cycle;
    int offset=phase<=1300?0:min(excess,int((phase-1300)/40));
    screen.spr.setViewport(x,y,width,28,false);
    screen.spr.drawString(name,x-offset,y,4);screen.spr.resetViewport();
}
void serialSettings() {
    bleModule.begin();JoystickNavigation navigation;UiRefresh refresh;int target=0;
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true){
        keys.kvs_update();auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());char cmd=Serial.available()?Serial.read():0;
        if(cmd=='S')uiScreenshot();if(cmd=='W')uiScreenshot(true);if(keys.x.pressed()||cmd=='Q')break;
        if(keys.up.pressed()||keys.down.pressed()||cmd=='U'||cmd=='D'||move==JoystickNavigation::Up||move==JoystickNavigation::Down)target=1-target;
        // 本机 USB 回显格式独立于 BLE 遥控发送格式：关闭、HEX、文本、JSON。
        if(target==0&&(keys.left.pressed()||keys.right.pressed()||cmd=='L'||cmd=='R'||move==JoystickNavigation::Left||move==JoystickNavigation::Right)){
            bool right=keys.right.pressed()||cmd=='R'||move==JoystickNavigation::Right;bleModule.configureUsb((bleModule.usbMode()+(right?1:4))%5);
        }
        if(target==1&&(keys.o.pressed()||cmd=='E')&&bleModule.snapshot().state==BleModule::Connected){bluetoothBaudSettings();screen.spr.unloadFont();screen.spr.setTextDatum(TC_DATUM);screen.spr.resetViewport();navigation.reset();refresh.invalidate();}
        if(cmd=='T')Serial.printf("[SERIAL] usb_mode=%d remote_connected=%d\n",bleModule.usbMode(),bleModule.snapshot().state==BleModule::Connected);
        screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,8,220,44,ui_uartsettings);screen.spr.drawFastHLine(20,66,200,TFT_DARKGREY);
        screen.spr.pushImage(10,98,220,44,ui_seriallocal);screen.spr.pushImage(10,146,220,44,ui_usbmode);
        const uint16_t* modes[]={ui_usboff,ui_usbbinary,ui_usbhex,ui_usbtext,ui_usbjson};screen.spr.pushImage(10,192,220,44,modes[bleModule.usbMode()]);
        screen.spr.pushImage(10,250,220,44,ui_serialremote);screen.spr.pushImage(10,296,220,44,bleModule.snapshot().state==BleModule::Connected?ui_uartconfirm:ui_disconnected);
        screen.spr.drawRoundRect(12,target?244:88,216,target?102:150,10,TFT_CYAN);
        screen.spr.pushImage(10,360,220,44,ui_usbinfo);screen.spr.pushImage(10,402,220,44,ui_usbhelp);
        screen.spr.pushImage(10,446,220,44,ui_settingsnav);screen.spr.pushImage(10,490,220,44,ui_settingsback);
        refresh.push((uint16_t*)screen.spr.getPointer());delay(20);
    }
    screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
void bluetoothMenu() {
    const uint16_t *titles[]={ui_title,ui_module,ui_sendsettings};
    const uint16_t *icons[]={image_data_5ble,nullptr,nullptr};
    void (*actions[])()={btGamepad,bluetoothModules,bluetoothOutputSettings};
    selectMenu(titles,icons,actions,3,false,ui_blehub);
}
void bluetoothModuleSettings(){
    const uint16_t* titles[]={ui_uartsettings,ui_sendsettings};
    const uint16_t* icons[]={nullptr,nullptr};void(*actions[])()={serialSettings,bluetoothOutputSettings};
    selectMenu(titles,icons,actions,2,false,ui_module);
}
void bluetoothOutputSettings() {
    bleModule.begin();JoystickNavigation navigation;UiRefresh refresh;
    int field=0;screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true) {
        keys.kvs_update();auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());
        char cmd=Serial.available()?Serial.read():0;
        // COM11 输入的普通字节透传给已连接模块；S/W/Q/T 等仅在单字节调试命令时保留。
        bool serialPayload=cmd && Serial.available();
        if(serialPayload){uint8_t first=uint8_t(cmd),buf[96];size_t n=0;buf[n++]=first;while(Serial.available()&&n<sizeof(buf))buf[n++]=uint8_t(Serial.read());bleModule.writeUsb(buf,n);cmd=0;}
        if(cmd=='S')uiScreenshot();if(cmd=='W')uiScreenshot(true);
        if(keys.x.pressed()||cmd=='Q')break;
        bool up=keys.up.pressed()||cmd=='U'||move==JoystickNavigation::Up;
        bool down=keys.down.pressed()||cmd=='D'||move==JoystickNavigation::Down;
        bool left=keys.left.pressed()||cmd=='L'||move==JoystickNavigation::Left;
        bool right=keys.right.pressed()||cmd=='R'||move==JoystickNavigation::Right;
        if(up||down)field=1-field;
        auto status=bleModule.snapshot();
        if(left||right) {
            int format=status.format,period=status.period;
            if(field==0)format=(format+(right?1:3))%4;
            else {int i=0;while(i<8&&ControlPacket::periods[i]!=period)i++;period=ControlPacket::periods[(i+(right?1:8))%9];}
            bleModule.configure(format,period);status=bleModule.snapshot();
        }
        if(cmd=='T')Serial.printf("[OUTPUT] format=%d period=%d\n",status.format,status.period);
        screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,8,220,44,ui_sendsettings);screen.spr.drawFastHLine(20,66,200,TFT_DARKGREY);
        screen.spr.pushImage(10,116,220,44,ui_sendformat);screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);
        const uint16_t* formats[]={ui_formatbinary,ui_formatjson,ui_formathex,ui_formattext};screen.spr.pushImage(10,162,220,44,formats[status.format]);
        screen.spr.pushImage(10,246,220,44,ui_sendperiod);screen.spr.setTextColor(0x5751,TFT_BLACK);
        char text[24];snprintf(text,sizeof(text),"%d ms",status.period);screen.spr.drawString(text,120,298,4);
        for(int i=0;i<2;i++)screen.spr.drawRoundRect(12,100+i*130,216,110,10,i==field?TFT_CYAN:TFT_DARKGREY);
        screen.spr.fillRoundRect(16,114+field*130,3,82,1,TFT_CYAN);
        screen.spr.pushImage(10,366,220,44,ui_blebaud);screen.spr.pushImage(10,412,220,44,ui_uartremote);
        screen.spr.pushImage(10,450,220,44,ui_settingsnav);screen.spr.pushImage(10,492,220,44,ui_settingsback);
        refresh.push((uint16_t*)screen.spr.getPointer());delay(20);
    }
    screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
void paintBluetoothBaud(const BleModule::Snapshot& status,int mode,int choice) {
    const uint32_t rates[]={9600,19200,38400,57600,115200};
        screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,8,220,44,ui_uartsettings);screen.spr.drawFastHLine(20,66,200,TFT_DARKGREY);
        screen.spr.pushImage(10,102,220,44,status.baudSupported?ui_uartactual:ui_uartunsupported);
        screen.spr.setTextColor(0x5751,TFT_BLACK);if(status.baudSupported)screen.spr.drawNumber(status.baud,120,148,4);else screen.spr.drawString("--",120,148,4);
        screen.spr.pushImage(10,200,220,44,ui_uartauto);
        screen.spr.pushImage(10,288,220,44,ui_uartmanual);screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);screen.spr.drawNumber(rates[choice],120,338,4);
        screen.spr.drawRoundRect(12,mode?278:190,216,mode?102:64,10,TFT_CYAN);
        screen.spr.pushImage(10,390,220,44,!status.baudSupported?ui_uartneedprotocol:status.baudResult==2?ui_uartapplied:status.baudResult==-2?ui_uartapplyfailed:ui_uartnav);
        screen.spr.pushImage(10,440,220,44,ui_uartconfirm);screen.spr.pushImage(10,488,220,44,ui_exit);
}
void bluetoothBaudSettings() {
    JoystickNavigation navigation;UiRefresh refresh;int mode=0;
    const uint32_t rates[]={9600,19200,38400,57600,115200};int choice=4;
    auto initial=bleModule.snapshot();for(int i=0;i<5;i++)if(rates[i]==initial.baud)choice=i;
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true) {
        keys.kvs_update();auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());
        char cmd=Serial.available()?Serial.read():0;if(cmd=='S')uiScreenshot();if(cmd=='W')uiScreenshot(true);
        auto status=bleModule.snapshot();if(keys.x.pressed()||cmd=='Q'||status.state!=BleModule::Connected)break;
        bool up=keys.up.pressed()||cmd=='U'||move==JoystickNavigation::Up,down=keys.down.pressed()||cmd=='D'||move==JoystickNavigation::Down;
        bool left=keys.left.pressed()||cmd=='L'||move==JoystickNavigation::Left,right=keys.right.pressed()||cmd=='R'||move==JoystickNavigation::Right;
        if(up||down)mode=1-mode;
        if(mode&&(left||right))choice=(choice+(right?1:4))%5;
        if(keys.o.pressed()||cmd=='E'){if(mode&&status.baudWritable)bleModule.setBaud(rates[choice]);else if(!mode)bleModule.readBaud();}
        if(cmd=='T')Serial.printf("[UART] actual=%u readable=%d writable=%d result=%d target=%u\n",status.baud,status.baudSupported,status.baudWritable,status.baudResult,rates[choice]);
        paintBluetoothBaud(status,mode,choice);
        refresh.push((uint16_t*)screen.spr.getPointer());delay(20);
    }
    screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
void paintBluetoothModule(const BleModule::Snapshot& status,const KVS& data,bool control,bool linked,int selected,int action) {
screen.spr.fillSprite(TFT_BLACK);
            screen.spr.pushImage(10,8,220,44,ui_module);
            const uint16_t* stateText=status.state==BleModule::Scanning?ui_scanning:status.state==BleModule::Connecting?ui_connecting:status.state==BleModule::Connected?ui_connected:status.state==BleModule::Unsupported?ui_nouart:status.state==BleModule::Failed?ui_connectfailed:ui_disconnected;
            screen.spr.pushImage(10,58,220,44,stateText);screen.spr.drawFastHLine(20,108,200,TFT_DARKGREY);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);
            if(control) {
                screen.spr.setTextSize(1);screen.spr.drawString(uiDeviceName(status.peer),120,110,4);
                char text[48];snprintf(text,sizeof(text),"%s  TX %u",status.profile,status.tx);screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);screen.spr.drawString(text,120,150,4);
                snprintf(text,sizeof(text),"RX %u  ERR %u",status.rx,status.errors);screen.spr.setTextColor(0x5751,TFT_BLACK);screen.spr.drawString(text,120,184,4);
                screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);
                snprintf(text,sizeof(text),"LX %d  LY %d",data.LX,data.LY);screen.spr.drawString(text,120,226,4);
                snprintf(text,sizeof(text),"RX %d  RY %d",data.RX,data.RY);screen.spr.drawString(text,120,264,4);
                snprintf(text,sizeof(text),"L %d  R %d",data.L_knob,data.R_knob);screen.spr.drawString(text,120,302,4);
                snprintf(text,sizeof(text),"KEY %05lX",(unsigned long)ControlPacket::buttons(data));screen.spr.drawString(text,120,340,4);
                const uint16_t* echoTitles[]={ui_rxoff,ui_rxbinary,ui_rxfeedback,ui_rxtext,ui_rxjson};
                screen.spr.pushImage(10,376,220,44,status.notifications?echoTitles[bleModule.usbMode()]:ui_rxnotprovided);
                screen.spr.setTextSize(1);screen.spr.drawString(String(status.received).substring(0,12),120,420,4);screen.spr.drawString(String(status.received).substring(12,24),120,449,4);
                screen.spr.pushImage(10,486,220,44,ui_moduleexit);
            } else if(linked) {
                screen.spr.setTextSize(1);screen.spr.drawString(uiDeviceName(status.peer),120,114,4);
                const uint16_t* actions[]={ui_startcontrol,ui_uartsettings,ui_disconnectdevice};
                for(int i=0;i<3;i++){screen.spr.pushImage(10,198+i*70,220,44,actions[i]);screen.spr.drawRoundRect(12,190+i*70,216,60,10,i==action?TFT_CYAN:TFT_DARKGREY);}
                screen.spr.pushImage(10,422,220,44,ui_joyselect);screen.spr.pushImage(10,484,220,44,ui_enter);
            } else {
                if(status.count) {
                    int base=(selected/3)*3;
                    for(int i=base;i<min(base+3,status.count);i++) {
                        int y=124+(i-base)*82;auto& d=status.devices[i];
                        screen.spr.setTextDatum(TL_DATUM);screen.spr.setTextSize(1);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);
                        drawSelectedName(d.name[0]?d.name:"BLE Device",18,y+6,d.uart?128:204,i==selected,d.address);
                        if(d.uart){screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);screen.spr.setTextDatum(TR_DATUM);screen.spr.drawString("UART",220,y+6,4);}
                        String address=d.address;address.replace(":","");address.toUpperCase();
                        address=address.substring(0,4)+" "+address.substring(4,8)+" "+address.substring(8,12);
                        screen.spr.setTextDatum(TL_DATUM);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString(uiDeviceName(address.c_str()),18,y+40,4);screen.spr.setTextDatum(TC_DATUM);
                        screen.spr.drawRoundRect(12,y,216,74,8,i==selected?TFT_CYAN:TFT_DARKGREY);
                    }
                } else screen.spr.pushImage(10,218,220,44,status.state==BleModule::Scanning?ui_searchwait:ui_nodevices);
                screen.spr.pushImage(10,392,220,44,ui_joyselect);screen.spr.pushImage(10,440,220,44,ui_scancontrols);screen.spr.pushImage(10,488,220,44,ui_scanback);
            }
}
// Serial-only layout fixtures. They never connect, write UART settings, or send reports.
void bluetoothLayoutPreview() {
    UiRefresh refresh;int mode=0;
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    BleModule::Snapshot sample;
    sample.state=BleModule::Connected;sample.baud=115200;sample.baudSupported=true;sample.baudWritable=true;sample.baudResult=1;
    snprintf(sample.peer,sizeof(sample.peer),"AeroPad Preview Device");snprintf(sample.profile,sizeof(sample.profile),"NUS");
    sample.tx=99999;sample.rx=65535;sample.notifications=true;snprintf(sample.received,sizeof(sample.received),"A5 5A 01 FF 64 9C 00 00 01 00 00 FF FF 03 01 00 ");
    KVS data;data.LX=-100;data.LY=100;data.RX=-100;data.RY=100;data.L_knob=-100;data.R_knob=100;
    data.a=data.b=data.x=data.o=data.L_up=data.L_down=data.R_up=data.R_down=data.board_L=data.board_R=false;
    data.up=data.down=data.left=data.right=data.switch_L1=data.switch_L2=data.switch_R1=data.switch_R2=false;
    while(true) {
        char cmd=Serial.available()?Serial.read():0;
        if(cmd=='S')uiScreenshot();if(cmd=='W')uiScreenshot(true);if(cmd=='Q')break;
        if(cmd=='R')mode=(mode+1)%5;if(cmd=='L')mode=(mode+4)%5;
        if(mode<2)paintBluetoothModule(sample,data,mode==0,true,0,1);
        else {
            auto view=sample;if(mode==4){view.baud=0;view.baudSupported=false;view.baudWritable=false;view.baudResult=-1;}
            paintBluetoothBaud(view,mode==3?1:0,4);
        }
        screen.spr.pushImage(10,58,220,44,ui_preview);
        refresh.push((uint16_t*)screen.spr.getPointer());delay(20);
    }
    screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}
void bluetoothModules() {
    bt.begin();bt.suspendGamepad();bleModule.begin();bleModule.stream(false);bleModule.scan();
    JoystickNavigation navigation;UiRefresh refresh;int selected=0,action=0;bool control=false,linked=false;
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    uint32_t last=0;
    while(true) {
        keys.kvs_update();bleModule.update(keys.kvs);
        auto status=bleModule.snapshot();
        char cmd=Serial.available()?Serial.read():0;if(cmd=='S')uiScreenshot();if(cmd=='W')uiScreenshot(true);
        if(cmd!='S'&&cmd!='W')bleModule.flushUsb();
        if(cmd=='T') {
            Serial.printf("[MODULE] state=%d count=%d tx=%u rx=%u errors=%u streaming=%d profile=%s baud=%u supported=%d error=%d stage=%s\n",status.state,status.count,status.tx,status.rx,status.errors,status.streaming,status.profile,status.baud,status.baudSupported,status.errorCode,status.stage);
            for(int i=0;i<status.count;i++)Serial.printf("[DEVICE %d] %s %s RSSI=%d UART=%d\n",i,status.devices[i].address,status.devices[i].name,status.devices[i].rssi,status.devices[i].uart);
        }
        if(status.state==BleModule::Connected&&!linked){linked=true;action=0;navigation.reset();}
        if(status.state!=BleModule::Connected&&linked){linked=false;control=false;bleModule.stream(false);navigation.reset();}
        if(control) {
            // All individual buttons remain payload inputs. Only the two-key chord exits.
            if(cmd=='Q'||(!keys.kvs.b&&!keys.kvs.x)){bleModule.stream(false);control=false;navigation.reset();}
        } else if(linked) {
            auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());
            bool prev=keys.up.pressed()||keys.left.pressed()||cmd=='U'||cmd=='L'||move==JoystickNavigation::Up||move==JoystickNavigation::Left;
            bool next=keys.down.pressed()||keys.right.pressed()||cmd=='D'||cmd=='R'||move==JoystickNavigation::Down||move==JoystickNavigation::Right;
            if(prev)action=(action+2)%3;if(next)action=(action+1)%3;
            if(keys.x.pressed()||cmd=='Q')bleModule.disconnect();
            if(keys.o.pressed()||cmd=='E') {
                if(action==0){control=true;bleModule.stream(true);navigation.reset();}
                else if(action==1){serialSettings();screen.spr.unloadFont();screen.spr.setTextDatum(TC_DATUM);screen.spr.resetViewport();navigation.reset();refresh.invalidate();}
                else bleModule.disconnect();
            }
        } else {
            auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());
            bool prev=keys.left.pressed()||keys.up.pressed()||cmd=='L'||cmd=='U'||move==JoystickNavigation::Left||move==JoystickNavigation::Up;
            bool next=keys.right.pressed()||keys.down.pressed()||cmd=='R'||cmd=='D'||move==JoystickNavigation::Right||move==JoystickNavigation::Down;
            if(status.count){if(prev)selected=(selected+status.count-1)%status.count;if(next)selected=(selected+1)%status.count;selected=min(selected,status.count-1);}
            if(keys.x.pressed()||cmd=='Q'){bleModule.disconnect();break;}
            if(keys.a.pressed()||cmd=='A'){bleModule.scan();selected=0;navigation.reset();}
            if(keys.b.pressed()){bluetoothModuleSettings();screen.spr.unloadFont();screen.spr.setTextDatum(TC_DATUM);screen.spr.resetViewport();navigation.reset();refresh.invalidate();}
            if(cmd=='F') {bluetoothOutputSettings();screen.spr.unloadFont();screen.spr.setTextDatum(TC_DATUM);screen.spr.resetViewport();navigation.reset();refresh.invalidate();}
            if(cmd=='C'){serialSettings();screen.spr.unloadFont();screen.spr.setTextDatum(TC_DATUM);screen.spr.resetViewport();navigation.reset();refresh.invalidate();}
            if((keys.o.pressed()||cmd=='E')&&status.count&&status.state!=BleModule::Scanning&&status.state!=BleModule::Connecting)bleModule.connect(selected);
        }
        if(millis()-last>=50) {
            last=millis();paintBluetoothModule(status,keys.kvs,control,linked,selected,action);
            refresh.push((uint16_t*)screen.spr.getPointer());
        }
        delay(1);
    }
    bleModule.stream(false);screen.spr.setTextSize(1);screen.spr.setTextDatum(TC_DATUM);screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}

// 5.蓝牙手柄：BLE HID 标准手柄，连接后实时发送摇杆和按键状态。
void btGamepad()
{
    bt.begin();
    screen.spr.unloadFont();
    screen.spr.setTextDatum(TC_DATUM);
    screen.spr.setSwapBytes(true);
    uint32_t lastPaint=0;
    uint32_t frames=0, frameTime=0, renderTime=0, perfStart=millis();
    UiRefresh refresh;
    KVS previous; bool firstFrame=true,previousConnection=false;
    while (true) {
        keys.kvs_update();
        bt.update(keys.kvs);
        if (millis()-lastPaint >= 20) {
            uint32_t paintStart=micros();
            if (lastPaint) { frameTime += millis()-lastPaint; frames++; }
            lastPaint=millis();
            if(firstFrame) {
                screen.spr.fillSprite(TFT_BLACK);
                screen.spr.pushImage(10,12,220,44,ui_title);
                screen.spr.pushImage(10,434,220,44,ui_bleexit1);
                screen.spr.pushImage(10,484,220,44,ui_bleexit2);
            }
            const bool connection=bt.connected();
            if(firstFrame || connection!=previousConnection)
                screen.spr.pushImage(10,62,220,44,connection?ui_connected:ui_disconnected);
            screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);
            char line[32];
            if(firstFrame || keys.kvs.LX!=previous.LX || keys.kvs.LY!=previous.LY) {
                screen.spr.fillRect(0,122,240,30,TFT_BLACK);
            snprintf(line,sizeof(line),"LX %d   LY %d",keys.kvs.LX,keys.kvs.LY);
            screen.spr.drawString(line,120,122,4);
            }
            if(firstFrame || keys.kvs.RX!=previous.RX || keys.kvs.RY!=previous.RY) {
                screen.spr.fillRect(0,158,240,30,TFT_BLACK);
            snprintf(line,sizeof(line),"RX %d   RY %d",keys.kvs.RX,keys.kvs.RY);
            screen.spr.drawString(line,120,158,4);
            }
            if(firstFrame || keys.kvs.L_knob!=previous.L_knob || keys.kvs.R_knob!=previous.R_knob) {
                screen.spr.fillRect(0,194,240,30,TFT_BLACK);
            snprintf(line,sizeof(line),"L %d    R %d",keys.kvs.L_knob,keys.kvs.R_knob);
            screen.spr.drawString(line,120,194,4);
            }
            const char *labels[]={"A","B","X","O"};
            bool pressed[]={!keys.kvs.a,!keys.kvs.b,!keys.kvs.x,!keys.kvs.o};
            bool oldPressed[]={!previous.a,!previous.b,!previous.x,!previous.o};
            for(int i=0;i<4;i++) {
                if(!firstFrame && pressed[i]==oldPressed[i])continue;
                int x=10+i*57;
                screen.spr.fillRoundRect(x,238,49,44,8,pressed[i]?TFT_CYAN:TFT_DARKGREY);
                screen.spr.setTextColor(pressed[i]?TFT_BLACK:TFT_WHITE);
                screen.spr.drawString(labels[i],x+24,245,4);
            }
            if(firstFrame || keys.kvs.LX!=previous.LX || keys.kvs.LY!=previous.LY)
                UiLayout::joystick(screen.spr,UiLayout::joystickLeft,310,keys.kvs.LX,keys.kvs.LY);
            if(firstFrame || keys.kvs.RX!=previous.RX || keys.kvs.RY!=previous.RY)
                UiLayout::joystick(screen.spr,UiLayout::joystickRight,310,keys.kvs.RX,keys.kvs.RY);

            uint16_t *pixels=(uint16_t*)screen.spr.getPointer();
            refresh.push(pixels);
            previous=keys.kvs;previousConnection=connection;firstFrame=false;
            renderTime += micros()-paintStart;
        }
        if(Serial.available()) {
            char command=Serial.read();
            if(command=='S') uiScreenshot();
            if(command=='Q') break;
            if(command=='D')bt.suspendGamepad();
            if(command=='C')bt.begin();
            if(command=='T') Serial.printf("[UI] frames=%u elapsed=%ums average_period=%ums average_render=%uus connected=%d\n",frames,millis()-perfStart,frames?frameTime/frames:0,frames?renderTime/frames:0,bt.connected());
        }
        if(keys.kvs.b==0 && keys.kvs.x==0) break;
        delay(1);
    }
    bt.releaseAll();
    screen.spr.loadFont(chinese_32);
    screen.spr.fillSprite(TFT_BLACK);
}

// 6.系统设置   ->   1.按键测试 2.陀螺仪立方体
void joystickCalibration();
void systemSet() {
    const uint16_t *titles[]={ui_m5,ui_m6,ui_cal,ui_monitor}; const uint16_t *icons[]={image_data_6_1_keysTest,image_data_6_2_cube,ui_calicon,ui_monitoricon};
    void (*actions[])()={keysTest,cube,joystickCalibration,deviceMonitor}; selectMenu(titles,icons,actions,4,false,ui_m4);
}
void deviceMonitor(){
    JoystickNavigation navigation;UiRefresh refresh;int page=0;uint32_t last=0;bool dirty=true;
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    const uint16_t* headings[]={ui_monmemory,ui_monheapdetail,ui_monpsdetail,ui_monhardware,ui_monnetwork,ui_monble};
    const uint16_t* labels[][4]={{ui_monheap,ui_monpsram,ui_monfirmware,ui_monblockratio},{ui_montotal,ui_monfree,ui_monmin,ui_monblock},{ui_montotal,ui_monused,ui_monfree,ui_monblock},{ui_moncpu,ui_monflash,ui_montasks,ui_monuptime},{ui_monwifi,ui_monquality,ui_monclients,ui_monip},{ui_montx,ui_monrx,ui_monerrors,ui_monperiod}};
    auto percent=[](size_t used,size_t total){return total?100.0f*used/total:0.0f;};
    auto kib=[](size_t n){return String(n/1024.0f,1)+" KiB";};
    // Image size calculation walks flash: cache immutable data outside the UI loop.
    const esp_partition_t* slot=esp_ota_get_running_partition();const size_t slotSize=slot?slot->size:0;
    const size_t sketchSize=ESP.getSketchSize();uint32_t paints=0,maxPaintUs=0;
    while(true){keys.kvs_update();char c=Serial.available()?Serial.read():0;auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());
        if(keys.x.pressed()||c=='Q')break;
        if(keys.left.pressed()||c=='L'||move==JoystickNavigation::Left){page=(page+5)%6;dirty=true;}
        if(keys.right.pressed()||c=='R'||move==JoystickNavigation::Right){page=(page+1)%6;dirty=true;}
        if(c=='S')uiScreenshot();
        if(c=='T')Serial.printf("[MONITOR] page=%d paints=%u max_paint_us=%u interval_ms=100\n",page,paints,maxPaintUs);
        if(dirty||millis()-last>=100){uint32_t paintStart=micros();bool switched=dirty;last=millis();dirty=false;
            size_t total=ESP.getHeapSize(),freeHeap=ESP.getFreeHeap(),ptotal=ESP.getPsramSize(),pfree=ESP.getFreePsram();
            size_t block=heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL|MALLOC_CAP_8BIT);
            float ratios[]={percent(total-freeHeap,total),percent(ptotal-pfree,ptotal),percent(sketchSize,slotSize),percent(block,freeHeap)};
            String value[4];
            if(page==0)for(int i=0;i<4;i++)value[i]=(i==1&&!ptotal)||(i==2&&!slotSize)?"--":String(ratios[i],1)+" %";
            if(page==1){value[0]=kib(total);value[1]=kib(freeHeap);value[2]=kib(ESP.getMinFreeHeap());value[3]=kib(block);}
            if(page==2){value[0]=kib(ptotal);value[1]=kib(ptotal-pfree);value[2]=kib(pfree);value[3]=kib(heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));}
            if(page==3){value[0]=String(ESP.getCpuFreqMHz())+" MHz";value[1]=String(ESP.getFlashChipSize()/1048576.0f,1)+" MiB";value[2]=String(uxTaskGetNumberOfTasks());value[3]=String(millis()/3600000)+"h "+String(millis()/60000%60)+"m "+String(millis()/1000%60)+"s";}
            if(page==4){bool linked=WiFi.status()==WL_CONNECTED;int rssi=linked?WiFi.RSSI():0;value[0]=linked?String(rssi)+" dBm":"--";value[1]=linked?String(constrain(2*(rssi+100),0,100))+" %":"--";value[2]=String(WiFi.softAPgetStationNum());value[3]=linked?WiFi.localIP().toString():"--";}
            if(page==5){auto s=bleModule.snapshot();value[0]=String(s.tx);value[1]=String(s.rx);value[2]=String(s.errors);value[3]=String(s.period)+" ms";}
            screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,8,220,44,headings[page]);
            for(int i=0;i<4;i++){int y=74+i*87;screen.spr.pushImage(10,y,220,44,labels[page][i]);screen.spr.setTextColor(i==0?TFT_CYAN:i==1?TFT_GREEN:i==2?0xFD20:0xC55F,TFT_BLACK);screen.spr.drawString(value[i],120,y+43,4);
                if(page==0){screen.spr.drawRect(28,y+74,184,6,TFT_DARKGREY);screen.spr.fillRect(30,y+76,int(constrain(ratios[i],0.0f,100.0f)*1.8f),2,TFT_CYAN);}}
            screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString(String(page+1)+" / 6",120,437,4);screen.spr.pushImage(10,480,220,44,ui_monnav);
            refresh.push((uint16_t*)screen.spr.getPointer());
            paints++;maxPaintUs=max(maxPaintUs,uint32_t(micros()-paintStart));
            if(switched)Serial.printf("[MONITOR PAGE] %d paint_us=%u\n",page,micros()-paintStart);
        }delay(2);
    }screen.spr.loadFont(chinese_32);
}
void joystickCalibration()
{
    // Valid manual edits commit immediately; incomplete guided calibration remains a draft.
    int page=8, choice=0, device=0, axis=0, field=0, notice=0;
    JoystickNavigation navigation;
    bool autoSaved=false;
    bool rotary=false;
    int center[6], low[6], high[6], dead=keys.deadzone();
    auto resetDraft=[&]() { for(int i=0;i<6;i++) {center[i]=keys.center(i);low[i]=keys.minimum(i);high[i]=keys.maximum(i);} dead=keys.deadzone(); };
    resetDraft();
    const uint16_t* axisTitles[]={ui_caxis0,ui_caxis1,ui_caxis2,ui_caxis3,ui_caxis4,ui_caxis5};
    const uint16_t* options[]={ui_guide,ui_inspect,ui_fine};
    uint32_t last=0;
    UiRefresh refresh;
    screen.spr.unloadFont(); screen.spr.setSwapBytes(true); screen.spr.setTextDatum(TC_DATUM);
    auto label=[&](int y,const uint16_t* text) {screen.spr.pushImage(10,y,220,44,text);};
    const uint16_t minColor=0x565f, centerColor=0x5751, maxColor=0xfd4a, rawColor=0xfeeb, deadColor=0xc4bf;
    auto number=[&](int y,int value,uint16_t color=TFT_WHITE) {screen.spr.setTextColor(color,TFT_BLACK);screen.spr.drawNumber(value,120,y,4);};
    while(true) {
        keys.kvs_update();
        const int previousPage=page;
        bool enter=keys.o.pressed(), back=keys.x.pressed();
        bool left=keys.left.pressed(),right=keys.right.pressed(),up=keys.up.pressed(),down=keys.down.pressed();
        if(Serial.available()) {char cmd=Serial.read();enter|=cmd=='E'||cmd=='C';back|=cmd=='Q';left|=cmd=='L';right|=cmd=='R';up|=cmd=='U';down|=cmd=='D';if(cmd=='S') uiScreenshot();}
        if(page==8 || page==0 || page==3 || page==6 || page==5) {
            auto move=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());
            left|=move==JoystickNavigation::Left; right|=move==JoystickNavigation::Right;
            up|=move==JoystickNavigation::Up; down|=move==JoystickNavigation::Down;
        } else navigation.reset();
        if(back) {if(page==8) break;page=page==0?8:0;notice=0;autoSaved=false;resetDraft();}
        else if(page==8) {
            if(left||right||up||down)device=1-device;
            if(enter){rotary=device==1;page=0;choice=0;}
        } else if(page==0) {
            if(left||up) choice=(choice+2)%3;
            if(right||down) choice=(choice+1)%3;
            if(enter) {resetDraft();axis=0;field=0;notice=0;autoSaved=false;page=choice==0?(rotary?2:1):choice==1?6:5;
                if(rotary){axis=4;if(page==2)for(int i=4;i<6;i++)low[i]=high[i]=keys.raw(i);}}
        } else if(page==1 && enter) {
            if(keys.sampleCenter(center)) {for(int i=0;i<4;i++)low[i]=high[i]=center[i];page=2;notice=0;}
            else notice=1;
        } else if(page==2) {
            // Four samples suppress a single ADC spike without delaying the sweep.
            for(int i=rotary?4:0;i<(rotary?6:4);i++){int v=0;for(int n=0;n<4;n++)v+=keys.raw(i);v/=4;low[i]=min(low[i],v);high[i]=max(high[i],v);}
            if(left)axis=rotary?4+(axis-4+1)%2:(axis+3)%4;if(right)axis=rotary?4+(axis-4+1)%2:(axis+1)%4;
            if(enter) {bool valid=true;for(int i=rotary?4:0;i<(rotary?6:4);i++)valid&=rotary?high[i]-low[i]>=1000:center[i]-low[i]>=500&&high[i]-center[i]>=500;
                if(valid){page=3;axis=rotary?4:0;if(rotary)for(int i=4;i<6;i++)center[i]=(low[i]+high[i])/2;notice=0;}else notice=2;}
        } else if(page==3 || page==6) {
            int count=rotary?2:4;int base=rotary?4:0;
            if(left||up)axis=base+(axis-base+count-1)%count;if(right||down)axis=base+(axis-base+1)%count;
            if(page==3&&enter){notice=(rotary?keys.saveKnobCalibration(low+4,high+4):keys.saveCalibration(center,low,high,dead))?3:4;page=7;}
        } else if(page==5) {
            int fields=rotary?3:5;
            if(up)field=(field+fields-1)%fields;if(down)field=(field+1)%fields;
            int step=right?10:left?-10:0;
            if(field==0 && step)axis=rotary?4+(axis-4+1)%2:(axis+(step>0?1:3))%4;
            if(field==1)low[axis]=constrain(low[axis]+step,0,4095);
            if(field==2 && !rotary)center[axis]=constrain(center[axis]+step,0,4095);
            if(field==(rotary?2:3))high[axis]=constrain(high[axis]+step,0,4095);
            if(field==4)dead=constrain(dead+step,30,250);
            if(field && step) {
                bool ok=rotary?keys.saveKnobCalibration(low+4,high+4):keys.saveCalibration(center,low,high,dead);
                autoSaved=ok;notice=ok?0:4;
                if(!ok)resetDraft();
            }
            if(enter){notice=(rotary?keys.saveKnobCalibration(low+4,high+4):keys.saveCalibration(center,low,high,dead))?3:4;page=7;}
        } else if(page==7&&enter){page=0;notice=0;resetDraft();}
        if(page!=previousPage)navigation.reset();
        if(millis()-last>=20) {
            last=millis();screen.spr.fillSprite(TFT_BLACK);
            label(8,page==8?ui_cal:rotary?ui_kcal:ui_jcal);
            screen.spr.drawFastHLine(20,62,200,TFT_DARKGREY);
            if(page==8){
                screen.spr.drawRoundRect(12,106+device*140,216,94,10,TFT_CYAN);
                label(128,ui_jcal);label(268,ui_kcal);
                label(392,device==0?ui_jdesc:ui_kdesc);label(444,ui_joyselect);label(488,ui_exit);
            }else if(page==0){
                for(int i=0;i<3;i++){if(i==choice)screen.spr.drawRoundRect(12,100+i*82,216,62,10,TFT_CYAN);label(110+i*82,options[i]);}
                label(374,rotary?ui_kdesc:ui_jdesc);label(428,ui_joyselect);label(484,ui_enter);
            }else if(page==1){
                label(76,ui_s1);label(170,ui_cstart);label(234,ui_ccenter);
                label(340,ui_jdesc);if(notice)label(414,ui_failed);label(484,ui_exit);
            }else if(page==2){
                label(70,rotary?ui_k1:ui_s2);label(122,rotary?ui_ckturn:ui_cextreme);
                label(182,axisTitles[axis]);
                screen.spr.drawRoundRect(16,234,208,130,8,TFT_DARKGREY);
                label(234,ui_cmin);number(275,low[axis],minColor);label(298,ui_cmax);number(339,high[axis],maxColor);
                label(380,notice==2?ui_crange:rotary?ui_kswitch:ui_cnext);label(428,ui_cfinish);label(484,ui_exit);
            }else if(page==3||page==6){
                label(70,page==3?(rotary?ui_k2:ui_s3):axisTitles[axis]);
                if(page==3)label(118,axisTitles[axis]);
                int base=page==3?164:118;
                label(base,ui_cmin);number(base+42,low[axis],minColor);label(base+78,ui_cmid);number(base+120,center[axis],centerColor);label(base+156,ui_cmax);number(base+198,high[axis],maxColor);
                if(page==6){label(352,rotary?ui_craw:ui_sampling);number(394,keys.raw(axis),rawColor);label(438,ui_joyselect);label(484,ui_exit);}
                else{label(404,ui_creview);label(448,ui_joyselect);label(492,ui_exit);}
            }else if(page==5){
                label(76,ui_fine);label(142,axisTitles[axis]);
                const uint16_t* names[]={ui_axisselect,ui_cmin,rotary?ui_cmax:ui_cmid,ui_cmax,ui_cdead};
                screen.spr.drawRoundRect(16,208,208,112,8,TFT_CYAN);label(216,names[field]);
                if(field)number(269,field==1?low[axis]:field==2?(rotary?high[axis]:center[axis]):field==3?high[axis]:dead,field==1?minColor:field==2?(rotary?maxColor:centerColor):field==3?maxColor:deadColor);
                else label(266,rotary?ui_kswitch:ui_cnext);
                label(340,notice==4?ui_cconfirm:autoSaved?ui_autosaved:ui_autohelp);label(394,ui_joyfield);label(436,ui_joyadjust);label(484,ui_autoback);
            }else if(page==7){label(174,notice==3?ui_saved:ui_cconfirm);label(310,ui_cdone);label(484,ui_exit);}
            // Draw borders after opaque text bitmaps, so text cannot erase card edges.
            if(page==8)screen.spr.drawRoundRect(12,106+device*140,216,94,10,TFT_CYAN);
            if(page==0)screen.spr.drawRoundRect(12,100+choice*82,216,62,10,TFT_CYAN);
            if(page==2)screen.spr.drawRoundRect(16,234,208,130,8,TFT_DARKGREY);
            if(page==5)screen.spr.drawRoundRect(16,208,208,112,8,field==0?TFT_CYAN:field==1?minColor:field==2?(rotary?maxColor:centerColor):field==3?maxColor:deadColor);
            refresh.push((uint16_t*)screen.spr.getPointer());
        }
        delay(1);
    }
    screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);
    screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}

//--------------------------------------------- 二级菜单 -------------------------------------------------------
//--------------------------------------------- 1.NRF遥控 -------------------------------------------------------
// 1.1 皮卡	


// 1.2 货车


// 1.3 坦克


// 1.4 无人机
void drone() { nrfControlPage(NrfControl::Drone); }

// 1.5 挖掘机


// 1.6 舰船



//----------------------------------------------2.本机游戏-----------------------------------------------------
// 2.1 贪吃蛇
void snake() {
    LocalGames::Snake game;game.reset();UiRefresh refresh;bool playing=false,paused=false;uint32_t tick=millis(),paint=0;
    Preferences prefs;prefs.begin("localgames",false);int best=prefs.getInt("snake",0);
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true){keys.kvs_update();char c=Serial.available()?Serial.read():0;if(keys.x.pressed()||c=='Q')break;if(c=='S')uiScreenshot();
        if(keys.o.pressed()||c=='E'){if(game.over){game.reset();paused=false;}playing=true;tick=millis();}
        if((keys.a.pressed()||c=='A')&&playing&&!game.over){paused=!paused;tick=millis();}
        if(keys.up.pressed()||c=='U'||keys.kvs.LY<-55)game.turn(0);else if(keys.right.pressed()||c=='R'||keys.kvs.LX>55)game.turn(1);else if(keys.down.pressed()||c=='D'||keys.kvs.LY>55)game.turn(2);else if(keys.left.pressed()||c=='L'||keys.kvs.LX<-55)game.turn(3);
        if(playing&&!paused&&!game.over&&millis()-tick>=uint32_t(max(70,220-game.score/5))){tick=millis();game.step(esp_random());if(game.score>best){best=game.score;if(game.over)prefs.putInt("snake",best);}}
        if(c=='T')Serial.printf("[SNAKE] score=%d length=%d over=%d paused=%d playing=%d\n",game.score,game.length,game.over,paused,playing);
        if(millis()-paint>=20){paint=millis();screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,4,220,44,ui_m7);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString("S "+String(game.score)+"  BEST "+String(best),120,57,4);
            screen.spr.drawRect(10,98,220,340,TFT_DARKGREY);screen.spr.fillRoundRect(12+game.food.x*12,100+game.food.y*12,10,10,3,0xFD20);
            for(int i=game.length-1;i>=0;i--)screen.spr.fillRoundRect(12+game.body[i].x*12,100+game.body[i].y*12,10,10,2,i==0?TFT_CYAN:TFT_GREEN);
            if(!playing||paused||game.over){screen.spr.fillRect(10,244,220,44,TFT_BLACK);screen.spr.pushImage(10,244,220,44,game.over?(game.won?ui_gamewin:ui_gameover):paused?ui_gamepause:ui_gamestart);}
            screen.spr.pushImage(10,442,220,44,ui_snakekeys);screen.spr.pushImage(10,488,220,44,ui_gameexit);refresh.push((uint16_t*)screen.spr.getPointer());}
        delay(2);
    }prefs.putInt("snake",best);prefs.end();screen.spr.loadFont(chinese_32);
}

// 2.2 打砖块
void brick() {
    LocalGames::Breakout game;game.reset();UiRefresh refresh;bool paused=false;uint32_t tick=millis(),paint=0;Preferences prefs;prefs.begin("localgames",false);int best=prefs.getInt("brick",0);
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true){keys.kvs_update();char c=Serial.available()?Serial.read():0;if(keys.x.pressed()||c=='Q')break;if(c=='S')uiScreenshot();
        uint32_t now=millis();float dt=min(uint32_t(now-tick),uint32_t(40))*.001f;tick=now;
        if(keys.o.pressed()||c=='E'){if(game.over){game.reset();paused=false;}game.launched=true;}
        if((keys.a.pressed()||c=='A')&&!game.over)paused=!paused;
        if(!paused&&!game.over){float axis=keys.kvs.LX/100.0f;if(!keys.kvs.left)axis=-1;if(!keys.kvs.right)axis=1;if(c=='L')game.move(game.paddle-20);if(c=='R')game.move(game.paddle+20);game.move(game.paddle+axis*260*dt);int steps=max(1,int(ceilf(dt/.008f)));for(int i=0;i<steps;i++)game.step(dt/steps);best=max(best,game.score);}
        if(c=='T')Serial.printf("[BRICK] score=%d lives=%d level=%d over=%d paused=%d launched=%d\n",game.score,game.lives,game.level,game.over,paused,game.launched);
        if(now-paint>=20){paint=now;screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,4,220,44,ui_m8);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString("S "+String(game.score)+" L "+String(game.level)+" HP "+String(game.lives),120,57,4);
            screen.spr.drawRect(10,98,220,340,TFT_DARKGREY);for(int i=0;i<40;i++)if(game.bricks[i])screen.spr.fillRoundRect(16+(i%8)*26,118+(i/8)*20,23,15,2,game.bricks[i]>1?TFT_ORANGE:i/8%2?TFT_GREEN:TFT_CYAN);
            screen.spr.fillRoundRect(int(game.paddle)-26,410,52,7,3,TFT_WHITE);screen.spr.fillCircle(lroundf(game.x),lroundf(game.y),4,TFT_CYAN);
            if(paused||game.over||!game.launched){screen.spr.fillRect(10,260,220,44,TFT_BLACK);screen.spr.pushImage(10,260,220,44,game.over?(game.won?ui_gamewin:ui_gameover):paused?ui_gamepause:ui_gamestart);}
            screen.spr.pushImage(10,442,220,44,ui_brickkeys);screen.spr.pushImage(10,488,220,44,ui_gameexit);refresh.push((uint16_t*)screen.spr.getPointer());}delay(2);
    }prefs.putInt("brick",best);prefs.end();screen.spr.loadFont(chinese_32);
}

// 2.3 飞机大战
void plane() {
    LocalGames::Plane game;UiRefresh refresh;bool playing=false,paused=false;uint32_t tick=millis(),paint=0;Preferences prefs;prefs.begin("localgames",false);int best=prefs.getInt("plane",0);
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true){keys.kvs_update();char c=Serial.available()?Serial.read():0;if(keys.x.pressed()||c=='Q')break;if(c=='S')uiScreenshot();
        uint32_t now=millis();float dt=min(uint32_t(now-tick),uint32_t(40))*.001f;tick=now;
        if(keys.o.pressed()||c=='E'){if(game.over){game.reset();paused=false;}playing=true;}
        if((keys.a.pressed()||c=='A')&&playing&&!game.over)paused=!paused;
        if((keys.b.pressed()||c=='B')&&playing&&!paused)game.bomb();
        if(playing&&!paused&&!game.over){float ax=keys.kvs.LX/100.0f,ay=keys.kvs.LY/100.0f;if(!keys.kvs.left||c=='L')ax=-1;if(!keys.kvs.right||c=='R')ax=1;if(!keys.kvs.up||c=='U')ay=-1;if(!keys.kvs.down||c=='D')ay=1;int n=max(1,int(ceilf(dt/.008f)));for(int i=0;i<n;i++)game.step(dt/n,ax,ay,esp_random());best=max(best,game.score);}
        if(c=='T')Serial.printf("[PLANE] score=%d lives=%d bombs=%d level=%d over=%d paused=%d playing=%d\n",game.score,game.lives,game.bombs,game.level,game.over,paused,playing);
        if(now-paint>=20){paint=now;screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,4,220,44,ui_m9);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString("S "+String(game.score)+" HP "+String(game.lives)+" B "+String(game.bombs),120,57,4);
            for(int i=0;i<20;i++)screen.spr.drawPixel(16+(i*47)%208,103+(i*79+int(now/35))%330,TFT_DARKGREY);
            for(auto& b:game.shots)if(b.active)screen.spr.fillRect(int(b.x)-1,int(b.y)-4,3,8,TFT_CYAN);
            for(auto& b:game.hostile)if(b.active)screen.spr.fillCircle(int(b.x),int(b.y),3,TFT_ORANGE);
            for(auto& e:game.enemies)if(e.hp){int x=int(e.x),y=int(e.y);uint16_t col=e.hp>1?TFT_ORANGE:TFT_RED;screen.spr.fillTriangle(x,y+12,x-12,y-6,x+12,y-6,col);screen.spr.fillRect(x-3,y-12,6,20,col);}
            if(game.invulnerable<=0||(now/100)%2){int x=int(game.x),y=int(game.y);screen.spr.fillTriangle(x,y-14,x-13,y+10,x+13,y+10,TFT_CYAN);screen.spr.fillRect(x-3,y-8,6,22,TFT_WHITE);}
            if(!playing||paused||game.over){screen.spr.fillRect(10,244,220,44,TFT_BLACK);screen.spr.pushImage(10,244,220,44,game.over?ui_gameover:paused?ui_gamepause:ui_gamestart);screen.spr.setTextColor(TFT_GREEN,TFT_BLACK);screen.spr.drawString("BEST "+String(best),120,300,4);}
            screen.spr.drawRect(10,98,220,340,TFT_DARKGREY);screen.spr.pushImage(10,442,220,44,ui_planekeys);screen.spr.pushImage(10,488,220,44,ui_gameexit);refresh.push((uint16_t*)screen.spr.getPointer());}delay(2);
    }prefs.putInt("plane",best);prefs.end();screen.spr.loadFont(chinese_32);
}

// 2.4 2048
void num2048() {
    LocalGames::Merge2048 game;game.reset(esp_random());UiRefresh refresh;JoystickNavigation navigation;bool dirty=true;Preferences prefs;prefs.begin("localgames",false);int best=prefs.getInt("2048",0);
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true){keys.kvs_update();char c=Serial.available()?Serial.read():0;if(keys.x.pressed()||c=='Q')break;if(c=='S')uiScreenshot();
        auto nav=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());int d=-1;
        if(keys.up.pressed()||c=='U'||nav==JoystickNavigation::Up)d=0;if(keys.right.pressed()||c=='R'||nav==JoystickNavigation::Right)d=1;if(keys.down.pressed()||c=='D'||nav==JoystickNavigation::Down)d=2;if(keys.left.pressed()||c=='L'||nav==JoystickNavigation::Left)d=3;
        if(d>=0){game.move(d,esp_random());dirty=true;}if(keys.a.pressed()||c=='A'){game.undo();dirty=true;}if(keys.o.pressed()||c=='E'){game.reset(esp_random());dirty=true;navigation.reset();}best=max(best,game.score);
        if(c=='T')Serial.printf("[2048] score=%d over=%d undo=%d\n",game.score,game.over,game.undoReady);
        if(dirty){dirty=false;screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,4,220,44,ui_m10);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString("S "+String(game.score),120,62,4);screen.spr.drawString("BEST "+String(best),120,99,4);
            for(int i=0;i<16;i++){int x=14+(i%4)*53,y=150+(i/4)*53;uint32_t v=game.cells[i];uint16_t col=v>=2048?0xB560:v>=128?0x8208:v>=16?0x3208:v?0x1928:0x1082;screen.spr.fillRoundRect(x,y,49,49,5,col);if(v){screen.spr.setTextColor(v>=2048?TFT_YELLOW:TFT_WHITE,col);screen.spr.drawString(String(v),x+24,y+13,v>=1024?2:4);}}
            if(game.over)screen.spr.pushImage(10,374,220,44,ui_gameover);else screen.spr.pushImage(10,374,220,44,ui_snakekeys);
            screen.spr.pushImage(10,434,220,44,ui_mergehelp);screen.spr.pushImage(10,488,220,44,ui_exit);refresh.push((uint16_t*)screen.spr.getPointer());}delay(2);
    }prefs.putInt("2048",best);prefs.end();screen.spr.loadFont(chinese_32);
}

// 2.5 俄罗斯方块
void sokoban() {
    LocalGames::Sokoban game;LocalGames::SokobanCelebration celebration;Preferences prefs;prefs.begin("sokoban",false);
    // The original five tutorial maps do not represent progress in this new pack.
    if(prefs.getUChar("version",0)!=2){prefs.putUChar("level",0);prefs.putUChar("selected",0);prefs.putUChar("version",2);}
    int unlocked=min(LocalGames::Sokoban::COUNT-1,(int)prefs.getUChar("level",0));game.reset(min(unlocked,(int)prefs.getUChar("selected",0)));
    UiRefresh refresh;JoystickNavigation navigation;bool dirty=true,selecting=true;uint32_t paint=0;
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    while(true){keys.kvs_update();char c=Serial.available()?Serial.read():0;if(keys.x.pressed()||c=='Q')break;if(c=='S')uiScreenshot();
        auto nav=navigation.update(keys.kvs.LX,keys.kvs.LY,millis());int d=-1;
        if(keys.up.pressed()||c=='U'||nav==JoystickNavigation::Up)d=0;if(keys.right.pressed()||c=='R'||nav==JoystickNavigation::Right)d=1;if(keys.down.pressed()||c=='D'||nav==JoystickNavigation::Down)d=2;if(keys.left.pressed()||c=='L'||nav==JoystickNavigation::Left)d=3;
        if(selecting&&d>=0){game.reset((game.level+(d==0||d==3?-1:1)+unlocked+1)%(unlocked+1));dirty=true;}
        if(d>=0&&!selecting&&!celebration.active&&game.move(d)){dirty=true;if(game.won()){celebration.begin(millis());if(game.level+1<game.COUNT&&game.level+1>unlocked){unlocked=game.level+1;prefs.putUChar("level",unlocked);}}}
        if((keys.a.pressed()||c=='A')&&!selecting){game.undo();celebration.cancel();dirty=true;}
        if(keys.o.pressed()||c=='E'){game.reset(game.won()&&game.level==game.COUNT-1?0:game.level);selecting=false;prefs.putUChar("selected",game.level);celebration.cancel();navigation.reset();dirty=true;}
        if(keys.b.pressed()||c=='B'){game.reset(game.level);selecting=true;celebration.cancel();navigation.reset();dirty=true;}
        if(celebration.due(millis())&&game.level+1<game.COUNT){game.reset(game.level+1);prefs.putUChar("selected",game.level);celebration.cancel();navigation.reset();dirty=true;}
        if(c=='T')Serial.printf("[SOKO] level=%d steps=%d pushes=%d placed=%d won=%d celebrating=%d history=%d selecting=%d unlocked=%d\n",game.level+1,game.steps,game.pushes,game.placed(),game.won(),celebration.active,game.historyCount,selecting,unlocked+1);
        if(celebration.active&&millis()-paint>=20){dirty=true;paint=millis();}
        if(dirty){dirty=false;screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,4,220,44,ui_sokoban);screen.spr.pushImage(10,55,220,44,ui_sokostats);
            screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);screen.spr.drawString(String(game.level+1)+"/10",45,101,4);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString(String(game.steps),120,101,4);screen.spr.setTextColor(TFT_GREEN,TFT_BLACK);screen.spr.drawString(String(game.placed())+"/"+String(game.total()),195,101,4);
            for(int i=0;i<game.COUNT;i++)screen.spr.fillCircle(48+16*i,138,i==game.level?4:2,i==game.level?TFT_WHITE:i<=unlocked?TFT_CYAN:TFT_DARKGREY);
            if(!celebration.active){for(int i=0;i<game.N;i++){int x=16+(i%game.W)*26,y=151+(i/game.W)*26;screen.spr.fillRoundRect(x,y,24,24,3,game.terrain[i]=='#'?0x29AB:0x1082);if(game.terrain[i]=='#'){screen.spr.drawFastHLine(x+3,y+6,18,0x426F);continue;}
                if(game.terrain[i]=='.'){screen.spr.drawCircle(x+12,y+12,7,TFT_GREEN);screen.spr.fillCircle(x+12,y+12,2,TFT_GREEN);}
                if(game.boxes[i]){uint16_t color=game.terrain[i]=='.'?TFT_GREEN:TFT_ORANGE;screen.spr.fillRoundRect(x+2,y+2,20,20,3,color);screen.spr.drawRect(x+4,y+4,16,16,0x6204);screen.spr.drawLine(x+5,y+5,x+18,y+18,0x6204);screen.spr.drawLine(x+18,y+5,x+5,y+18,0x6204);if(game.corner(i))screen.spr.drawRoundRect(x+1,y+1,22,22,3,TFT_RED);}
                if(i==game.player){screen.spr.fillCircle(x+12,y+12,9,TFT_CYAN);screen.spr.fillCircle(x+9,y+9,2,TFT_BLACK);screen.spr.fillCircle(x+15,y+9,2,TFT_BLACK);screen.spr.drawFastHLine(x+8,y+16,8,TFT_BLACK);}}
                screen.spr.pushImage(10,375,220,44,selecting?ui_switch:game.stuck()?ui_sokostuck:ui_snakekeys);
            }else{
                uint32_t elapsed=millis()-celebration.started;const uint16_t colors[]={TFT_CYAN,TFT_YELLOW,TFT_GREEN,TFT_MAGENTA,TFT_ORANGE};
                // Non-blocking 50Hz confetti, a pulsing halo and a vector trophy.
                for(int i=0;i<32;i++){int x=14+(i*53+elapsed/(9+i%5))%210,y=145+(i*37+elapsed/(5+i%7))%220;screen.spr.fillRect(x,y,3+(i%3),5,colors[i%5]);}
                screen.spr.drawCircle(120,213,52+(elapsed/100)%8,0x2945);screen.spr.drawCircle(120,213,46,TFT_YELLOW);
                screen.spr.fillRoundRect(95,179,50,48,9,TFT_YELLOW);screen.spr.drawRoundRect(80,183,80,32,10,TFT_YELLOW);screen.spr.fillRect(116,223,8,23,TFT_YELLOW);screen.spr.fillRoundRect(98,245,44,8,3,TFT_YELLOW);
                screen.spr.pushImage(10,275,220,44,game.level+1==game.COUNT?ui_gamewin:ui_sokowin);
                if(game.level+1<game.COUNT){screen.spr.pushImage(10,326,220,44,ui_sokonext);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString(String((LocalGames::SokobanCelebration::Duration-elapsed+999)/1000),120,382,4);}
            }
            screen.spr.pushImage(10,434,220,44,selecting?ui_enter:ui_mergehelp);
            if(selecting){screen.spr.setTextColor(TFT_CYAN,TFT_BLACK);screen.spr.drawString("PUSH "+String(LocalGames::sokobanMinimumPushes[game.level])+"+",120,496,4);}else screen.spr.pushImage(10,488,220,44,ui_sokokeys);
            refresh.push((uint16_t*)screen.spr.getPointer());}delay(2);
    }prefs.end();screen.spr.loadFont(chinese_32);
}

void tetris() {
    LocalGames::Tetris game;game.reset(esp_random());UiRefresh refresh;JoystickNavigation navigation;bool playing=false,paused=false;uint32_t lastFall=millis(),paint=0;Preferences prefs;prefs.begin("localgames",false);int best=prefs.getInt("tetris",0);
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);const uint16_t colors[]={TFT_CYAN,TFT_YELLOW,TFT_VIOLET,TFT_GREEN,TFT_RED,TFT_BLUE,TFT_ORANGE};
    while(true){keys.kvs_update();char c=Serial.available()?Serial.read():0;if(keys.x.pressed()||c=='Q')break;if(c=='S')uiScreenshot();
        if(keys.o.pressed()||c=='E'){if(!playing||game.over){if(game.over)game.reset(esp_random());playing=true;paused=false;lastFall=millis();}else if(!paused)game.rotate();}
        if((keys.a.pressed()||c=='A')&&playing&&!game.over){paused=!paused;lastFall=millis();}
        int nx=!keys.kvs.left?-100:!keys.kvs.right?100:keys.kvs.LX;auto nav=navigation.update(nx,keys.kvs.LY,millis());
        if(playing&&!paused&&!game.over){if(keys.left.pressed()||c=='L'||nav==JoystickNavigation::Left)game.move(-1);if(keys.right.pressed()||c=='R'||nav==JoystickNavigation::Right)game.move(1);if(keys.up.pressed()||c=='U'||nav==JoystickNavigation::Up)game.rotate();if(keys.b.pressed()||c=='B')game.drop(esp_random());
            uint32_t interval=(!keys.kvs.down||keys.kvs.LY>55)?45:max(90,650-game.lines/10*50);if(c=='D'||millis()-lastFall>=interval){lastFall=millis();game.down(esp_random());}best=max(best,game.score);}
        if(c=='T')Serial.printf("[TETRIS] score=%d lines=%d over=%d paused=%d playing=%d\n",game.score,game.lines,game.over,paused,playing);
        if(millis()-paint>=20){paint=millis();screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,4,220,44,ui_m11);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString("S "+String(game.score)+" ROW "+String(game.lines),120,58,4);
            auto cell=[&](int x,int y,int type,bool outline){if(y<0||y>=20)return;int px=12+x*16,py=100+y*16;if(outline)screen.spr.drawRect(px+1,py+1,14,14,TFT_DARKGREY);else screen.spr.fillRoundRect(px+1,py+1,14,14,2,colors[type]);};
            for(int y=0;y<20;y++)for(int x=0;x<10;x++)if(game.board[y][x])cell(x,y,game.board[y][x]-1,false);
            int ghost=game.ghost();for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(game.cell(game.piece,game.rotation,x,y)){cell(game.x+x,ghost+y,game.piece,true);cell(game.x+x,game.y+y,game.piece,false);}
            screen.spr.drawRect(10,98,164,324,TFT_DARKGREY);screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.drawString("NEXT",204,104,2);for(int y=0;y<4;y++)for(int x=0;x<4;x++)if(game.cell(game.next,0,x,y))screen.spr.fillRect(181+x*12,137+y*12,10,10,colors[game.next]);
            if(!playing||paused||game.over){screen.spr.fillRect(10,244,220,44,TFT_BLACK);screen.spr.pushImage(10,244,220,44,game.over?ui_gameover:paused?ui_gamepause:ui_gamestart);}
            screen.spr.pushImage(10,434,220,44,ui_tetrishelp);screen.spr.pushImage(10,488,220,44,ui_gameexit);refresh.push((uint16_t*)screen.spr.getPointer());}delay(2);
    }prefs.putInt("tetris",best);prefs.end();screen.spr.loadFont(chinese_32);
}


//---------------------------------------------------------------------------------------------------
// 3.1 弹球



//---------------------------------------------4.网络信息------------------------------------------------------




//---------------------------------------------5.蓝牙手柄------------------------------------------------------




//---------------------------------------------6.系统设置------------------------------------------------------
// 6.1 按键测试
void keysTest()
{
    UiRefresh refresh;
    uint32_t perfStart=millis(), frames=0, renderTime=0;
    screen.spr.unloadFont();
    screen.spr.setSwapBytes(true);
    screen.spr.setTextDatum(TC_DATUM);
	// 按键测试的固定UI
	{
		// 清屏并绘制框架
		screen.spr.fillSprite(TFT_BLACK);
		screen.spr.pushImage(10,12,220,44,ui_m5);

		// 4个前端按键
		screen.spr.drawSmoothCircle(35, 75, 15, TFT_GREEN, TFT_BLACK);  // 抗锯齿的圆形
		screen.spr.drawSmoothCircle(80, 70, 10, TFT_GREEN, TFT_BLACK);
		screen.spr.drawSmoothCircle(150, 70, 10, TFT_GREEN, TFT_BLACK);
		screen.spr.drawSmoothCircle(205, 75, 15, TFT_GREEN, TFT_BLACK);

		// 电位器旋钮
		screen.spr.drawRect(35, 110, 75, 20, TFT_CYAN);
		screen.spr.drawRect(130, 110, 75, 20, TFT_CYAN);

		// 4个拨杆开关
		screen.spr.drawRect(20, 150, 20, 30, TFT_PINK);
		screen.spr.drawRect(60, 150, 20, 30, TFT_PINK);
		screen.spr.drawRect(160, 150, 20, 30, TFT_PINK);
		screen.spr.drawRect(200, 150, 20, 30, TFT_PINK);

        UiLayout::joystick(screen.spr,UiLayout::joystickLeft,200,0,0);
        UiLayout::joystick(screen.spr,UiLayout::joystickRight,200,0,0);

		// 板载按键
		screen.spr.drawRect(80, 320, 20, 10, TFT_WHITE);
		screen.spr.drawRect(140, 320, 20, 10, TFT_WHITE);

		// 功能键
		screen.spr.drawRect(40, 360, 20, 20, TFT_SILVER);
		screen.spr.drawRect(40, 400, 20, 20, TFT_SILVER);
		screen.spr.drawRect(10, 380, 20, 20, TFT_SILVER);
		screen.spr.drawRect(70, 380, 20, 20, TFT_SILVER);
		screen.spr.drawRect(180, 360, 20, 20, TFT_SILVER);
		screen.spr.drawRect(180, 400, 20, 20, TFT_SILVER);
		screen.spr.drawRect(150, 380, 20, 20, TFT_SILVER);
		screen.spr.drawRect(210, 380, 20, 20, TFT_SILVER);

		// 陀螺仪
		screen.spr.drawRect(90, 431, 60, 60, TFT_VIOLET);
        screen.spr.pushImage(10,492,220,44,ui_keysexit);

		// 把内容推到屏幕显示。 改方向后这里长款也要同步改	
		refresh.push((uint16_t*)screen.spr.getPointer());
	}

	// 实时显示按键状态
	while (true)
	{
		char cmd=Serial.available()?Serial.read():0; if(cmd=='S') uiScreenshot(); if(cmd=='Q') break; if(cmd=='T')Serial.printf("[KEYUI] frames=%u elapsed=%ums average_render=%uus\n",frames,millis()-perfStart,frames?renderTime/frames:0);
        uint32_t paintStart=micros();
        // 按键状态更新
		keys.kvs_update();
		static uint32_t lastAxisLog = 0;
		if (millis() - lastAxisLog >= 500) {
			lastAxisLog = millis();
			Serial.printf("[JOY] RXraw=%d RYraw=%d RX=%d RY=%d\n", analogRead(PIN_RX), analogRead(PIN_RY), keys.kvs.RX, keys.kvs.RY);
		}

		// 前端4个按键
		keys.kvs.L_up == 0 ? screen.spr.fillSmoothCircle(35, 75, 12, TFT_GREEN, TFT_BLACK) : screen.spr.fillSmoothCircle(35, 75, 12, TFT_BLACK, TFT_BLACK);
		keys.kvs.L_down == 0 ? screen.spr.fillSmoothCircle(80, 70, 7, TFT_GREEN, TFT_BLACK) : screen.spr.fillSmoothCircle(80, 70, 7, TFT_BLACK, TFT_BLACK);
		keys.kvs.R_down == 0 ? screen.spr.fillSmoothCircle(150, 70, 7, TFT_GREEN, TFT_BLACK) : screen.spr.fillSmoothCircle(150, 70, 7, TFT_BLACK, TFT_BLACK);
		keys.kvs.R_up == 0 ? screen.spr.fillSmoothCircle(205, 75, 12, TFT_GREEN, TFT_BLACK) : screen.spr.fillSmoothCircle(205, 75, 12, TFT_BLACK, TFT_BLACK);

		// 电位旋钮
		screen.spr.fillRect(36, 111, 73, 18, TFT_BLACK);
		screen.spr.fillRect(131, 111, 73, 18, TFT_BLACK);
		screen.spr.fillRect(map(keys.kvs.L_knob, -100, 100, 38, 107) - 2, 111, 4, 18, TFT_CYAN);
		screen.spr.fillRect(map(keys.kvs.R_knob, -100, 100, 133, 202) - 2, 111, 4, 18, TFT_CYAN);

		// 4个拨杆开关
		if (keys.kvs.switch_L1 == 1)
		{
			screen.spr.fillRect(21, 151, 18, 13, TFT_PINK);
			screen.spr.fillRect(21, 166, 18, 13, TFT_BLACK);
			buzzer.on();
			led.on();
		}
		else
		{
			screen.spr.fillRect(21, 151, 18, 13, TFT_BLACK);
			screen.spr.fillRect(21, 166, 18, 13, TFT_PINK);
			buzzer.off();
			led.off();
		}

		if (keys.kvs.switch_L2 == 1)
		{
			screen.spr.fillRect(61, 151, 18, 13, TFT_PINK);
			screen.spr.fillRect(61, 166, 18, 13, TFT_BLACK);
		}
		else {
			screen.spr.fillRect(61, 151, 18, 13, TFT_BLACK);
			screen.spr.fillRect(61, 166, 18, 13, TFT_PINK);
		}

		if (keys.kvs.switch_R1 == 1) {
			screen.spr.fillRect(161, 151, 18, 13, TFT_PINK);
			screen.spr.fillRect(161, 166, 18, 13, TFT_BLACK);
		}
		else {
			screen.spr.fillRect(161, 151, 18, 13, TFT_BLACK);
			screen.spr.fillRect(161, 166, 18, 13, TFT_PINK);
		}

		if (keys.kvs.switch_R2 == 1) {
			screen.spr.fillRect(201, 151, 18, 13, TFT_PINK);
			screen.spr.fillRect(201, 166, 18, 13, TFT_BLACK);
		}
		else {
			screen.spr.fillRect(201, 151, 18, 13, TFT_BLACK);
			screen.spr.fillRect(201, 166, 18, 13, TFT_PINK);
		}

        UiLayout::joystick(screen.spr,UiLayout::joystickLeft,200,keys.kvs.LX,keys.kvs.LY);
        UiLayout::joystick(screen.spr,UiLayout::joystickRight,200,keys.kvs.RX,keys.kvs.RY);

		// 板载按键
		keys.kvs.board_L == 0 ? screen.spr.fillRect(81, 321, 18, 8, TFT_WHITE) : screen.spr.fillRect(81, 321, 18, 8, TFT_BLACK);
		keys.kvs.board_R == 0 ? screen.spr.fillRect(141, 321, 18, 8, TFT_WHITE) : screen.spr.fillRect(141, 321, 18, 8, TFT_BLACK);

		// 功能按键
		keys.kvs.up == 0 ? screen.spr.fillRect(41, 361, 18, 18, TFT_SILVER) : screen.spr.fillRect(41, 361, 18, 18, TFT_BLACK);
		keys.kvs.down == 0 ? screen.spr.fillRect(41, 401, 18, 18, TFT_SILVER) : screen.spr.fillRect(41, 401, 18, 18, TFT_BLACK);
		keys.kvs.left == 0 ? screen.spr.fillRect(11, 381, 18, 18, TFT_SILVER) : screen.spr.fillRect(11, 381, 18, 18, TFT_BLACK);
		keys.kvs.right == 0 ? screen.spr.fillRect(71, 381, 18, 18, TFT_SILVER) : screen.spr.fillRect(71, 381, 18, 18, TFT_BLACK);
		keys.kvs.x == 0 ? screen.spr.fillRect(181, 361, 18, 18, TFT_SILVER) : screen.spr.fillRect(181, 361, 18, 18, TFT_BLACK);
		keys.kvs.a == 0 ? screen.spr.fillRect(181, 401, 18, 18, TFT_SILVER) : screen.spr.fillRect(181, 401, 18, 18, TFT_BLACK);
		keys.kvs.o == 0 ? screen.spr.fillRect(151, 381, 18, 18, TFT_SILVER) : screen.spr.fillRect(151, 381, 18, 18, TFT_BLACK);
		keys.kvs.b == 0 ? screen.spr.fillRect(211, 381, 18, 18, TFT_SILVER) : screen.spr.fillRect(211, 381, 18, 18, TFT_BLACK);

		// 陀螺仪
		int angleX = map(constrain(keys.kvs.angleX,-100,100), -100, 100, 437, 484);
		int angleY = map(constrain(keys.kvs.angleY,-100,100), -100, 100, 96, 143);
		screen.spr.fillRect(91, 432, 58, 58, TFT_BLACK);
		screen.spr.fillRect(angleY - 5, angleX - 5, 10, 10, TFT_VIOLET);

		// 把内容推到屏幕显示。 改方向后这里长款也要同步改
		refresh.push((uint16_t*)screen.spr.getPointer());


		// 使用同时按住的电平状态，两个按键不必在同一帧产生 pressed 事件。
		if (keys.kvs.b == 0 && keys.kvs.x == 0)
		{
			screen.spr.fillSprite(TFT_BLACK);									// 清屏
			refresh.push((uint16_t*)screen.spr.getPointer());  // 把内容推到屏幕显示。 改方向后这里长款也要同步改
			break;
		}

        frames++;renderTime+=micros()-paintStart;
		delay(1);
	}
    buzzer.off(); led.off();
    screen.spr.loadFont(chinese_32);
}


// 6.2 陀螺仪立方体
void cube() {
    UiRefresh refresh;
    screen.spr.unloadFont();screen.spr.setSwapBytes(true);screen.spr.setTextDatum(TC_DATUM);
    auto calibrateGyro=[&]() {
        screen.spr.fillSprite(TFT_BLACK);screen.spr.pushImage(10,8,220,44,ui_m6);
        screen.spr.pushImage(10,200,220,44,ui_cubestill);screen.spr.pushImage(10,260,220,44,ui_cubecal);
        refresh.push((uint16_t*)screen.spr.getPointer());
        float sum[3]={},square[3]={};bool valid=true;
        for(int n=0;n<160;n++) {
            keys.mpu6050.update();
            float v[]={keys.mpu6050.getGyroX(),keys.mpu6050.getGyroY(),keys.mpu6050.getGyroZ()};
            float ax=keys.mpu6050.getAccX(),ay=keys.mpu6050.getAccY(),az=keys.mpu6050.getAccZ();
            float gravity=ax*ax+ay*ay+az*az;
            valid&=std::isfinite(gravity)&&gravity>0.64f&&gravity<1.44f;
            for(int i=0;i<3;i++){valid&=std::isfinite(v[i]);sum[i]+=v[i];square[i]+=v[i]*v[i];}
            delay(5);
        }
        for(int i=0;i<3;i++){sum[i]/=160;valid&=square[i]/160-sum[i]*sum[i]<0.16f&&fabsf(sum[i])<10;}
        if(valid)keys.mpu6050.setGyroOffsets(keys.mpu6050.getGyroXoffset()+sum[0],keys.mpu6050.getGyroYoffset()+sum[1],keys.mpu6050.getGyroZoffset()+sum[2]);
        Serial.printf("[CUBE CAL] stable=%d bias=%.3f,%.3f,%.3f\n",valid,sum[0],sum[1],sum[2]);
        return valid;
    };
    bool calibrated=calibrateGyro();
    keys.kvs_update();
    float origin[3]={keys.mpu6050.getAngleX(),keys.mpu6050.getAngleY(),keys.mpu6050.getAngleZ()};
    float display[3]={0,0,0},painted[3]={999,999,999};bool paused=false,firstFrame=true,previousPaused=false;
    int previousNumbers[3]={999,999,999};
    uint32_t last=0,frames=0,renderTime=0,started=millis();
    const CubeGeometry::Point vertices[]={{-1,-1,-1},{1,-1,-1},{1,1,-1},{-1,1,-1},{-1,-1,1},{1,-1,1},{1,1,1},{-1,1,1}};
    const int faces[6][4]={{0,1,2,3},{4,5,6,7},{0,4,7,3},{1,5,6,2},{0,1,5,4},{3,2,6,7}};
    const uint16_t colors[]={0x1928,0x1928,0x2206,0x2206,0x2184,0x2184};
    while(true) {
        keys.kvs_update();
        char cmd=Serial.available()?Serial.read():0;
        if(cmd=='S')uiScreenshot();if(cmd=='W')uiScreenshot(true);
        if(keys.x.pressed() || cmd=='Q')break;
        if(keys.a.pressed() || cmd=='F')paused=!paused;
        float measured[3]={keys.mpu6050.getAngleX(),keys.mpu6050.getAngleY(),keys.mpu6050.getAngleZ()};
        bool rebase=keys.o.pressed() || cmd=='Z';
        if(keys.b.pressed() || cmd=='G'){calibrated=calibrateGyro();keys.kvs_update();measured[0]=keys.mpu6050.getAngleX();measured[1]=keys.mpu6050.getAngleY();measured[2]=keys.mpu6050.getAngleZ();rebase=true;firstFrame=true;}
        if(rebase) {
            for(int i=0;i<3;i++){origin[i]=measured[i];display[i]=0;}
            paused=false;
        }
        if(cmd=='T')Serial.printf("[CUBE] frames=%u elapsed=%ums render=%uus angles=%.1f,%.1f,%.1f paused=%d\n",frames,millis()-started,frames?renderTime/frames:0,display[0],display[1],display[2],paused);
        uint32_t now=millis();
        if(now-last>=20) {
            float dt=last?min(uint32_t(now-last),uint32_t(100))*0.001f:0.02f;
            last=now;uint32_t paintStart=micros();
            if(!paused)for(int i=0;i<3;i++) {
                float target=CubeGeometry::wrap(measured[i]-origin[i]);
                if(std::isfinite(target))display[i]=CubeGeometry::wrap(display[i]+CubeGeometry::wrap(target-display[i])*(1.0f-expf(-dt/0.06f)));
            }
            if(firstFrame) {
                screen.spr.fillSprite(TFT_BLACK);
                screen.spr.pushImage(10,8,220,44,ui_m6);
                screen.spr.drawFastHLine(20,106,200,TFT_DARKGREY);
                screen.spr.pushImage(0,326,80,36,ui_cube_roll);screen.spr.pushImage(80,326,80,36,ui_cube_pitch);screen.spr.pushImage(160,326,80,36,ui_cube_yaw);
                screen.spr.pushImage(10,408,220,44,ui_cubeyaw);
                screen.spr.pushImage(10,448,220,44,ui_cubecontrols);
                screen.spr.pushImage(10,492,220,44,ui_cubeback);
            }
            if(firstFrame || paused!=previousPaused)screen.spr.pushImage(10,56,220,44,!calibrated?ui_cuberetry:paused?ui_cubepaused:ui_cubelive);
            bool changed=firstFrame;for(int i=0;i<3;i++)changed|=fabsf(CubeGeometry::wrap(display[i]-painted[i]))>0.08f;
            if(changed) {
            screen.spr.fillRect(0,110,240,210,TFT_BLACK);
            CubeGeometry::Point view[8],projected[8];
            for(int i=0;i<8;i++) {
                view[i]=CubeGeometry::rotate(vertices[i],display[0],display[1],display[2]);
                // Fixed camera angle keeps three faces visible at the zero pose.
                view[i]=CubeGeometry::rotate(view[i],22,-28,0);
                projected[i]=CubeGeometry::project(view[i]);
            }
            int order[6]={0,1,2,3,4,5};float depth[6];
            for(int f=0;f<6;f++){depth[f]=0;for(int j=0;j<4;j++)depth[f]+=view[faces[f][j]].z;}
            for(int i=0;i<5;i++)for(int j=i+1;j<6;j++)if(depth[order[i]]<depth[order[j]]){int v=order[i];order[i]=order[j];order[j]=v;}
            for(int f:order) {
                auto a=projected[faces[f][0]],b=projected[faces[f][1]],c=projected[faces[f][2]],d=projected[faces[f][3]];
                screen.spr.fillTriangle(a.x,a.y,b.x,b.y,c.x,c.y,colors[f]);
                screen.spr.fillTriangle(a.x,a.y,c.x,c.y,d.x,d.y,colors[f]);
                const uint16_t edge=f<2?TFT_CYAN:f<4?0x5751:0xfd4a;
                for(int j=0;j<4;j++){auto p=projected[faces[f][j]],q=projected[faces[f][(j+1)%4]];screen.spr.drawLine(p.x,p.y,q.x,q.y,edge);}
            }
            for(int i=0;i<3;i++)painted[i]=display[i];
            }
            const uint16_t angleColors[]={TFT_CYAN,0x5751,0xfd4a};
            for(int i=0;i<3;i++) {
                int value=lroundf(display[i]);
                if(firstFrame || value!=previousNumbers[i]) {
                    screen.spr.fillRect(i*80,374,80,30,TFT_BLACK);
                    screen.spr.setTextColor(angleColors[i],TFT_BLACK);screen.spr.drawNumber(value,40+i*80,374,4);
                    previousNumbers[i]=value;
                }
            }
            firstFrame=false;previousPaused=paused;
            refresh.push((uint16_t*)screen.spr.getPointer());frames++;renderTime+=micros()-paintStart;
        }
        delay(1);
    }
    screen.spr.setTextColor(TFT_WHITE,TFT_BLACK);screen.spr.loadFont(chinese_32);screen.spr.fillSprite(TFT_BLACK);
}






//--------------------------------------------- 工具方法 -------------------------------------------------------
// 获取电压ADC值
int getVolADC()
{
	return analogRead(PIN_VOL);
}

// 获取实际电压值（mV）
int getVol()
{
	return analogReadMilliVolts(PIN_VOL);
}

// 设置周期性获取电压值（时间ms），返回定时器的ID
//int setGetVolTimer(int time);		

// 关闭周期性获取电压（定时器ID）
//void closeGetVolTimer(int timerID);
