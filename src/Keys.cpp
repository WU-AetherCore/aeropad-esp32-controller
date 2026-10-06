#include "Keys.h"
#include "NetworkPortal.h"
#include <Preferences.h>


Adafruit_MCP23X17 mcp;
// mcp初始化配置
void mcp_init()
{
	Serial.printf("[I2C] MCP23017 0x27: %s\n", mcp.begin_I2C(0x27) ? "connected" : "not detected");

	// 配置4个拨杆开关和8个功能按键
	mcp.pinMode(MCP_PIN_L1, INPUT);      // 配置扩展板的IO为输入模式（不能用INPUT_PULL!）
	mcp.pinMode(MCP_PIN_L2, INPUT);		 // 也可以是输出模式 mcp.pinMode(1, OUTPUT);
	mcp.pinMode(MCP_PIN_R1, INPUT);		 // 左1 --- 左2 --- 右1 --- 右2
	mcp.pinMode(MCP_PIN_R2, INPUT);
	mcp.pinMode(MCP_PIN_UP, INPUT);
	mcp.pinMode(MCP_PIN_DOWN, INPUT);
	mcp.pinMode(MCP_PIN_LEFT, INPUT);
	mcp.pinMode(MCP_PIN_RIGHT, INPUT);
	mcp.pinMode(MCP_PIN_O, INPUT);
	mcp.pinMode(MCP_PIN_X, INPUT);
	mcp.pinMode(MCP_PIN_A, INPUT);
	mcp.pinMode(MCP_PIN_B, INPUT);

	mcp.pinMode(MCP_PIN_L1, INPUT);       // 上/下拉需要单独使用pullUp函数（内部上/下拉电阻）
	mcp.pinMode(MCP_PIN_L2, INPUT);
	mcp.pinMode(MCP_PIN_R1, INPUT);
	mcp.pinMode(MCP_PIN_R2, INPUT);
	mcp.pinMode(MCP_PIN_UP, INPUT_PULLUP);
	mcp.pinMode(MCP_PIN_DOWN, INPUT_PULLUP);
	mcp.pinMode(MCP_PIN_LEFT, INPUT_PULLUP);
	mcp.pinMode(MCP_PIN_RIGHT, INPUT_PULLUP);
	mcp.pinMode(MCP_PIN_O, INPUT_PULLUP);
	mcp.pinMode(MCP_PIN_X, INPUT_PULLUP);
	mcp.pinMode(MCP_PIN_A, INPUT_PULLUP);
	mcp.pinMode(MCP_PIN_B, INPUT_PULLUP);
}


// 构造函数，传入IO口和默认状态
MCP_bounce::MCP_bounce(uint8_t pin, bool default_state)
{
	_pin = pin;
	_default_state = default_state;
}

// 单击
bool MCP_bounce::pressed()
{
	// 单次按键消抖
	if (mcp.digitalRead(_pin) != _default_state && !_isPressed_flag) {  // 按钮刚被按下, 且按下标记为False, 防止瞬间多次执行
		delay(20);
		if (mcp.digitalRead(_pin) != _default_state) {                  // 确认按钮仍然被按下
			_isPressed_flag = true;
			return true;
		}
	}

	else if (mcp.digitalRead(_pin) == _default_state) {                 // 按钮释放时重置按钮状态
		_isPressed_flag = false;                                        // 更新按钮状态为未按下
	}

	return false;
}

// 读取按键状态
bool MCP_bounce::read()
{
	return mcp.digitalRead(_pin);
}

// 打开 返回read() != _default_state;，用于拨杆开关，on()和off()更直观
bool MCP_bounce::on()
{
	return this->read() != _default_state;
}

// 关闭 返回read() != _default_state;，用于拨杆开关，on()和off()更直观
bool MCP_bounce::off()
{
	return this->read() == _default_state;
}







// 构造函数中初始化 MPU6050 对象,	和MCP23017按键对象	
Keys::Keys() : mpu6050(Wire), up(MCP_PIN_UP, HIGH), down(MCP_PIN_DOWN, HIGH), left(MCP_PIN_LEFT, HIGH), right(MCP_PIN_RIGHT, HIGH), o(MCP_PIN_O, HIGH), x(MCP_PIN_X, HIGH), a(MCP_PIN_A, HIGH), b(MCP_PIN_B, HIGH), L1(MCP_PIN_L1, LOW), L2(MCP_PIN_L2, LOW), R1(MCP_PIN_R1, LOW), R2(MCP_PIN_R2, LOW)
{
}

// 初始化配置
void Keys::init(int conID)
{
	_conID = conID;
    Preferences prefs;
    prefs.begin("joycal", true);
    for(int i=0;i<4;i++) { char key[8]; snprintf(key,sizeof(key),"c%d",i); _centers[i]=prefs.getInt(key,_centers[i]); }
    _deadzone=constrain(prefs.getInt("dead",90),30,250);
    prefs.end();
    prefs.begin("joycal",true);
    int saved[14];
    if(prefs.getBytesLength("full") == sizeof(saved) && prefs.getBytes("full",saved,sizeof(saved))==sizeof(saved) && saved[0]==1) {
        bool valid=saved[13]>=30 && saved[13]<=250;
        for(int i=0;i<4;i++) valid=valid && saved[5+i]>=0 && saved[9+i]<=4095 && saved[1+i]-saved[5+i]>=500 && saved[9+i]-saved[1+i]>=500;
        if(valid) { for(int i=0;i<4;i++) { _centers[i]=saved[1+i]; _minimum[i]=saved[5+i]; _maximum[i]=saved[9+i]; } _deadzone=saved[13]; }
    }
    prefs.end();


    prefs.begin("joycal",true);int knobs[4];
    if(prefs.getBytesLength("knobs")==sizeof(knobs)&&prefs.getBytes("knobs",knobs,sizeof(knobs))==sizeof(knobs)){
        bool valid=true;for(int i=0;i<2;i++)valid&=knobs[i]>=0&&knobs[i+2]<=4095&&knobs[i+2]-knobs[i]>=1000;
        if(valid)for(int i=0;i<2;i++){_knobMinimum[i]=knobs[i];_knobMaximum[i]=knobs[i+2];}
    }prefs.end();
    printCalibration();

	Wire.setPins(41, 40);				  // 设置I2C引脚（sda, scl）。 一共只有两个I2C资源，另一个【Wire1】
	Wire.begin();						  // 初始化I2C
	Wire.beginTransmission(0x68);
	Serial.printf("[I2C] MPU6050 0x68: %s\n", Wire.endTransmission() == 0 ? "connected" : "not detected");
											
	mcp_init();							  // 初始化MCP23017
	mpu6050.begin();					  // 自动与默认地址通信
	
	L_up.attach(PIN_L_UP, INPUT_PULLUP);  // 给按键对象绑定引脚，并设置模式 INPUT, INPUT_PULLUP or OUTPUT
	L_up.interval(25);                    // 设置消抖时间为25ms
	L_up.setPressedState(LOW);            // 设置按键按下时的电平，默认为HIGH

	L_down.attach(PIN_L_DOWN, INPUT_PULLUP);
	L_down.interval(25);
	L_down.setPressedState(LOW);

	R_up.attach(PIN_R_UP, INPUT_PULLUP);
	R_up.interval(25);
	R_up.setPressedState(LOW);

	R_down.attach(PIN_R_DOWN, INPUT_PULLUP);
	R_down.interval(25);
	R_down.setPressedState(LOW);

	board_L.attach(PIN_BOARD_L, INPUT_PULLUP);
	board_L.interval(25);
	board_L.setPressedState(LOW);

	board_R.attach(PIN_BOARD_R, INPUT_PULLUP);
	board_R.interval(25);
	board_R.setPressedState(LOW);

}

// 按键更新
void Keys::kvs_update()
{
	// 按键消抖更新状态
	bounce_update();

	// 前端4个按键
	kvs.L_up = L_up.read();
	kvs.L_down = L_down.read();
	kvs.R_up = R_up.read();
	kvs.R_down = R_down.read();
	// 板载按键
	kvs.board_L = board_L.read();
	kvs.board_R = board_R.read();


	// 4个拨杆开关    // 左1 --- 左2 --- 右1 --- 右2
	kvs.switch_L1 = L1.read();
	kvs.switch_L2 = L2.read();
	kvs.switch_R1 = R1.read();
	kvs.switch_R2 = R2.read();
	// 功能按键
	kvs.up = up.read();
	kvs.down = down.read();
	kvs.left = left.read();
	kvs.right = right.read();
	kvs.o = o.read();
	kvs.x = x.read();
	kvs.a = a.read();
	kvs.b = b.read();


	// 电位旋钮
	kvs.L_knob = (int8_t)constrain(map(raw(4), _knobMinimum[0], _knobMaximum[0], 100, -100),-100,100);
	kvs.R_knob = (int8_t)constrain(map(raw(5), _knobMinimum[1], _knobMaximum[1], 100, -100),-100,100);
    const int pins[4]={PIN_LX,PIN_LY,PIN_RX,PIN_RY};
    int8_t values[4];
    for(int i=0;i<4;i++) {
        int raw=analogRead(pins[i]), delta=raw-_centers[i];
        int value=0;
        if(delta > _deadzone) value=(delta-_deadzone)*100/(_maximum[i]-_centers[i]-_deadzone);
        else if(delta < -_deadzone) value=(delta+_deadzone)*100/(_centers[i]-_minimum[i]-_deadzone);
        values[i]=(int8_t)constrain(i>=2?-value:value,-100,100);
    }
    kvs.LX=values[0]; kvs.LY=values[1]; kvs.RX=values[2]; kvs.RY=values[3];

	// 陀螺仪
	mpu6050.update();
	if (mpu6050.getAngleX() < -45) { kvs.angleX = -100; }
	else if (mpu6050.getAngleX() > 45) { kvs.angleX = 100; }
	else { kvs.angleX = (int8_t)map(mpu6050.getAngleX(), -45, 45, -100, 100); }

	if (mpu6050.getAngleY() < -45) { kvs.angleY = -100; }
	else if (mpu6050.getAngleY() > 45) { kvs.angleY = 100; }
	else { kvs.angleY = (int8_t)map(mpu6050.getAngleY(), -45, 45, -100, 100); }
    WIFI::publish(kvs);
}

// 串口打印按键数据
void Keys::ShowInSerial()
{
	//Serial.print("JOY:");
	Serial.print("LX:");
	Serial.print(kvs.LX);
	Serial.print("\tLY:");
	Serial.print(kvs.LY);
	Serial.print("\tRX:");
	Serial.print(kvs.RX);
	Serial.print("\tRY:");
	Serial.print(kvs.RY);

	Serial.print("\tKNOB:");
	Serial.print("L_knob:");
	Serial.print(kvs.L_knob);
	Serial.print(" R_knob:");
	Serial.print(kvs.R_knob);

	Serial.print(" L_UP:");
	Serial.print(kvs.L_up);
	Serial.print(" L_DOWN:");
	Serial.print(kvs.L_down);
	Serial.print(" R_UP:");
	Serial.print(kvs.R_up);
	Serial.print(" R_DOWN:");
	Serial.print(kvs.R_down);

	Serial.print("\t4_switch:");
	Serial.print(" L1:");
	Serial.print(kvs.switch_L1);
	Serial.print(" L2:");
	Serial.print(kvs.switch_L2);
	Serial.print(" R1:");
	Serial.print(kvs.switch_R1);
	Serial.print(" R2:");
	Serial.print(kvs.switch_R2);

	Serial.print("\tFUNC_KEYS:");
	Serial.print(" UP:");
	Serial.print(kvs.up);
	Serial.print(" DOWN:");
	Serial.print(kvs.down);
	Serial.print(" LEFT:");
	Serial.print(kvs.left);
	Serial.print(" RIGHT:");
	Serial.print(kvs.right);
	Serial.print(" O:");
	Serial.print(kvs.o);
	Serial.print(" X:");
	Serial.print(kvs.x);
	Serial.print(" A:");
	Serial.print(kvs.a);
	Serial.print(" B:");
	Serial.print(kvs.b);

	Serial.print("\tMPU6050 : ");
	Serial.print(" angleX: ");
	Serial.print(kvs.angleX);
	Serial.print(" angleY : ");
	Serial.println(kvs.angleY);
}

// 按键测试，检测消抖效果，单击、按下、释放
void Keys::dounnceTest()
{
	while (true)
	{
		bounce_update();		 //按键消抖更新，前端4个按键+板载2个按键

		if(up.pressed()) { Serial.println("up pressed"); }
		if (down.pressed()) { Serial.println("down pressed"); }
		if (left.pressed()) { Serial.println("left pressed"); }
		if (right.pressed()) { Serial.println("right pressed"); }

		// on()和off(), read()可用于判断长按
		if (a.on()) { Serial.println("a on"); }
		if (b.on()) { Serial.println("b on"); }
		if (!o.read()) { Serial.println("o on"); }  // o.read() == 0
		if (!x.read()) { Serial.println("x on"); }  // x.read() == 0

		if (L1.on()) { Serial.println("L1 on"); }
		if (L2.on()) { Serial.println("L2 on"); }
		if (R1.on()) { Serial.println("R1 on"); }
		if (R2.on()) { Serial.println("R2 on"); }

		if(board_L.pressed()) {Serial.println("board_L pressed");}
		if(board_R.pressed()) {Serial.println("board_R pressed");}
		if(L_up.pressed()) {Serial.println("L_up pressed");}
		if(L_down.isPressed()) {Serial.println("L_down isPressed");}	  // 长按
		if(R_up.pressed()) {Serial.println("R_up pressed");}
		if(R_down.isPressed()) {Serial.println("R_down isPressed");}      // 长按

		delay(20);

		// 同时按下左键和B键，退出本功能
		if (left.read() == 0 && b.read() == 0) { break; }
	}
	
}

// 按键消抖更新，前端4个按键+板载2个按键
void Keys::bounce_update()
{
	// 按键消抖更新状态
	L_up.update();
	L_down.update();
	R_up.update();
	R_down.update();
	board_L.update();
	board_R.update();
}


int Keys::raw(int axis) const { const int pins[6]={PIN_LX,PIN_LY,PIN_RX,PIN_RY,PIN_L_KNOB,PIN_R_KNOB}; return analogRead(pins[axis]); }
bool Keys::sampleCenter(int *centers) {
    int sums[4]={0}, lows[4]={4095,4095,4095,4095}, highs[4]={0};
    for(int n=0;n<100;n++) {
        for(int i=0;i<4;i++) { int v=raw(i); sums[i]+=v; lows[i]=min(lows[i],v); highs[i]=max(highs[i],v); }
        delay(5);
    }
    for(int i=0;i<4;i++) if(highs[i]-lows[i]>120 || sums[i]/100<800 || sums[i]/100>3300) {
        Serial.printf("[CAL] rejected centre axis=%d min=%d max=%d\n",i,lows[i],highs[i]); return false;
    }
    for(int i=0;i<4;i++) centers[i]=sums[i]/100;
    return true;
}
bool Keys::saveCalibration(const int *centers, const int *minimum, const int *maximum, int dead) {
    if(dead<30 || dead>250) return false;
    int blob[14]={1}; blob[13]=dead;
    for(int i=0;i<4;i++) {
        if(minimum[i]<0 || maximum[i]>4095 || centers[i]-minimum[i]<500 || maximum[i]-centers[i]<500 || centers[i]-minimum[i]<=dead || maximum[i]-centers[i]<=dead) {
            Serial.printf("[CAL] incomplete axis=%d min=%d centre=%d max=%d\n",i,minimum[i],centers[i],maximum[i]); return false;
        }
        blob[1+i]=centers[i]; blob[5+i]=minimum[i]; blob[9+i]=maximum[i];
    }
    bool unchanged=dead==_deadzone;for(int i=0;i<4;i++)unchanged&=centers[i]==_centers[i]&&minimum[i]==_minimum[i]&&maximum[i]==_maximum[i];
    if(unchanged)return true;
    Preferences prefs; if(!prefs.begin("joycal",false)) return false;
    bool ok=prefs.putBytes("full",blob,sizeof(blob))==sizeof(blob); prefs.end();
    if(!ok) return false;
    for(int i=0;i<4;i++) { _centers[i]=centers[i]; _minimum[i]=minimum[i]; _maximum[i]=maximum[i]; }
    _deadzone=dead;
    Serial.println("[CAL] full calibration saved"); return true;
}
bool Keys::calibrateCenter() {
    int centers[4]; return sampleCenter(centers) && saveCalibration(centers,_minimum,_maximum,_deadzone);
}
void Keys::adjustDeadzone(int change) {
    saveCalibration(_centers,_minimum,_maximum,constrain(_deadzone+change,30,250));
}

bool Keys::saveKnobCalibration(const int *minimum,const int *maximum) {
    int blob[4];for(int i=0;i<2;i++){if(minimum[i]<0||maximum[i]>4095||maximum[i]-minimum[i]<1000)return false;blob[i]=minimum[i];blob[i+2]=maximum[i];}
    bool unchanged=true;for(int i=0;i<2;i++)unchanged&=minimum[i]==_knobMinimum[i]&&maximum[i]==_knobMaximum[i];
    if(unchanged)return true;
    Preferences prefs;if(!prefs.begin("joycal",false))return false;
    bool ok=prefs.putBytes("knobs",blob,sizeof(blob))==sizeof(blob);prefs.end();
    if(ok)for(int i=0;i<2;i++){_knobMinimum[i]=minimum[i];_knobMaximum[i]=maximum[i];}
    return ok;
}

void Keys::printCalibration() const {
    Serial.printf("[CALSTATE] dead=%d",_deadzone);
    for(int i=0;i<6;i++)Serial.printf(" axis%d=%d,%d,%d",i,minimum(i),center(i),maximum(i));
    Serial.println();
}
