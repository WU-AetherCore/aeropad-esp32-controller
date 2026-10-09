#pragma once
#include <Arduino.h>
#include <TFT_eSPI.h>
// Display labels only: never changes radio addresses or control packets.
namespace UiNames {
void begin();
String catalog();
bool save(const String &key, const String &name, int width,
          const String &bitmap, String &error);
bool reset(const String &key, String &error);
bool draw(TFT_eSprite &sprite, const char *key, int x, int y, int width,
          bool selected, uint16_t color = TFT_WHITE);
bool custom(const char *key);
uint32_t revision();
} // namespace UiNames
