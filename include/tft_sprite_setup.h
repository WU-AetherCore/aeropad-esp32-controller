#pragma once
// TFT_eSPI provides RAM sprites and fonts only. rm67162.cpp drives the AMOLED.
#define ST7789_DRIVER
#define DISABLE_ALL_LIBRARY_WARNINGS
#define TFT_WIDTH 240
#define TFT_HEIGHT 536
#define TFT_MISO -1
#define TFT_MOSI 18
#define TFT_SCLK 47
#define TFT_CS 6
#define TFT_DC 7
#define TFT_RST 17
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT
#define SPI_FREQUENCY 75000000
#define TFT_SPI_MODE SPI_MODE0
