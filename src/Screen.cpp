#include "Screen.h"


// 初始化配置
void Screen::init()
{
	rm67162_init();						// 屏幕初始化
	lcd_setRotation(0);					// 0-3  1是横向
	spr.setColorDepth(16);
	if (!spr.createSprite(240, 536)) {
		Serial.println("[Screen] Sprite allocation failed; check OPI PSRAM");
		while (true) { delay(1000); }
	}
	spr.setSwapBytes(1);
	spr.fillSprite(TFT_BLACK);			// 清屏
}


