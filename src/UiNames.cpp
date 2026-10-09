#include "UiNames.h"
#include "UiNameRecord.h"
#include <SPIFFS.h>
#include <atomic>
#include <esp_heap_caps.h>
#include <esp_partition.h>
#include <mbedtls/base64.h>
namespace UiNames {
struct Item {
  Item(const char *k, const char *g, const char *f)
      : key(k), group(g), factory(f) {}
  const char *key;
  const char *group;
  const char *factory;
  char name[129] = {};
  uint8_t *bits = nullptr;
  uint16_t width = 0;
  uint32_t generation = 0, start = 0, last = 0;
  uint8_t bank = 0;
};
#define ITEM(k, g, n) {k, g, n}
static Item items[] = {ITEM("main", "主菜单", "主菜单"),
                       ITEM("nrf", "主菜单", "NRF 遥控"),
                       ITEM("games", "主菜单", "本机游戏"),
                       ITEM("network", "主菜单", "网络信息"),
                       ITEM("bluetooth", "主菜单", "蓝牙中心"),
                       ITEM("system", "主菜单", "系统设置"),
                       ITEM("drone", "NRF 遥控", "无人机"),
                       ITEM("car", "NRF 遥控", "四驱车"),
                       ITEM("nrfdebug", "NRF 遥控", "NRF 调试"),
                       ITEM("nrfsettings", "NRF 遥控", "NRF 设置"),
                       ITEM("snake", "本机游戏", "贪吃蛇"),
                       ITEM("brick", "本机游戏", "打砖块"),
                       ITEM("plane", "本机游戏", "飞机大战"),
                       ITEM("2048", "本机游戏", "2048 游戏"),
                       ITEM("tetris", "本机游戏", "俄罗斯方块"),
                       ITEM("sokoban", "本机游戏", "推箱子"),
                       ITEM("wifi", "网络信息", "WiFi 管理"),
                       ITEM("bleprotocol", "蓝牙中心", "自定义遥控"),
                       ITEM("gamepad", "蓝牙中心", "蓝牙游戏手柄"),
                       ITEM("module", "蓝牙中心", "蓝牙模块控制"),
                       ITEM("send", "蓝牙中心", "发送设置"),
                       ITEM("serial", "蓝牙中心", "串口设置"),
                       ITEM("keytest", "系统设置", "按键测试"),
                       ITEM("cube", "系统设置", "陀螺仪立方体"),
                       ITEM("calibration", "系统设置", "控件校准"),
                       ITEM("monitor", "系统设置", "设备监测"),
                       ITEM("drone1", "无人机预设", "预设 1"),
                       ITEM("drone2", "无人机预设", "预设 2"),
                       ITEM("drone3", "无人机预设", "预设 3"),
                       ITEM("drone4", "无人机预设", "预设 4"),
                       ITEM("drone5", "无人机预设", "预设 5"),
                       ITEM("car1", "四驱车预设", "预设 1"),
                       ITEM("car2", "四驱车预设", "预设 2"),
                       ITEM("car3", "四驱车预设", "预设 3"),
                       ITEM("car4", "四驱车预设", "预设 4"),
                       ITEM("car5", "四驱车预设", "预设 5"),
                       ITEM("ble1", "蓝牙遥控预设", "预设 1"),
                       ITEM("ble2", "蓝牙遥控预设", "预设 2"),
                       ITEM("ble3", "蓝牙遥控预设", "预设 3"),
                       ITEM("ble4", "蓝牙遥控预设", "预设 4"),
                       ITEM("ble5", "蓝牙遥控预设", "预设 5")};
#undef ITEM
// Two independently checksummed banks retain the previous label through a
// partial write.
using UiNameRecord::Header;
static SemaphoreHandle_t mutex = nullptr;
static std::atomic<bool> ready{false}, loading{true};
static std::atomic<uint32_t> version{0};
using UiNameRecord::checksum;
static Item *find(const char *k) {
  for (auto &i : items)
    if (k && strcmp(k, i.key) == 0)
      return &i;
  return nullptr;
}
static String path(const Item &i, int bank) {
  return String("/names_") + i.key + (bank ? ".b" : ".a");
}
static bool utf8(const String &s) {
  return UiNameRecord::validUtf8(s.c_str(), s.length());
}
static uint8_t *read(const Item &i, int bank, Header &h) {
  fs::File f = SPIFFS.open(path(i, bank), "r");
  if (!f || f.read((uint8_t *)&h, sizeof(h)) != sizeof(h) ||
      h.magic != 0x314d414e || h.width > 1024 || h.length > 128 ||
      (h.width == 0) != (h.length == 0)) {
    return nullptr;
  }
  size_t size = h.length + ((h.width + 7) / 8) * 36;
  if (f.size() != sizeof(h) + size)
    return nullptr;
  uint8_t *data = (uint8_t *)heap_caps_malloc(size + 1, MALLOC_CAP_SPIRAM |
                                                            MALLOC_CAP_8BIT);
  if (!data)
    return nullptr;
  if (f.read(data, size) != size || checksum(h, data, size) != h.crc) {
    free(data);
    return nullptr;
  }
  data[size] = 0;
  if (h.length) {
    String name;
    name.reserve(h.length);
    for (int k = 0; k < h.length; k++)
      name += char(data[k]);
    if (!utf8(name)) {
      free(data);
      return nullptr;
    }
  }
  return data;
}
static void loadStorage(void *) {
  Serial.println("[NAMES] mounting label storage");
  bool mounted = SPIFFS.begin(false);
  Serial.printf("[NAMES] mount=%d\n", int(mounted));
  if (!mounted) {
    const esp_partition_t *p = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA, ESP_PARTITION_SUBTYPE_DATA_SPIFFS, nullptr);
    uint8_t bytes[256];
    bool blank = p != nullptr;
    for (size_t at = 0; blank && at < p->size; at += sizeof(bytes)) {
      blank = esp_partition_read(p, at, bytes, sizeof(bytes)) == ESP_OK;
      for (auto b : bytes)
        if (b != 255)
          blank = false;
    }
    if (blank) {
      Serial.println("[NAMES] initializing empty label partition");
      mounted = SPIFFS.begin(true);
    }
  }
  if (!mounted) {
    loading = false;
    Serial.println("[NAMES] storage unavailable; existing partition preserved");
    vTaskDelete(nullptr);
    return;
  }
  for (auto &i : items) {
    Header a{}, b{};
    uint8_t *da = read(i, 0, a);
    uint8_t *db = read(i, 1, b);
    bool pick = db && (!da || int32_t(b.generation - a.generation) > 0);
    uint8_t *data = pick ? db : da;
    Header &h = pick ? b : a;
    free(pick ? da : db);
    if (data) {
      xSemaphoreTake(mutex, portMAX_DELAY);
      i.bank = pick;
      i.generation = h.generation;
      i.width = h.width;
      if (h.length) {
        memcpy(i.name, data, h.length);
        size_t n = ((h.width + 7) / 8) * 36;
        memmove(data, data + h.length, n);
        i.bits = data;
      } else
        free(data);
      xSemaphoreGive(mutex);
    }
  }
  ready = true;
  loading = false;
  version++;
  Serial.printf("[NAMES] ready: %u labels, PSRAM glyph cache\n",
                unsigned(sizeof(items) / sizeof(items[0])));
  vTaskDelete(nullptr);
}
void begin() {
  if (mutex)
    return;
  mutex = xSemaphoreCreateMutex();
  if (mutex)
    if (xTaskCreatePinnedToCore(loadStorage, "name_storage", 4096, nullptr, 1,
                                nullptr, 0) != pdPASS) {
      loading = false;
      Serial.println("[NAMES] storage task allocation failed");
    }
}
static String quote(const char *s) {
  String r = "\"";
  while (*s) {
    if (*s == '"' || *s == '\\')
      r += '\\';
    r += *s++;
  }
  return r + '"';
}
bool custom(const char *key) {
  if (!mutex || !key)
    return false;
  Item *i = find(key);
  if (!i)
    return false;
  xSemaphoreTake(mutex, portMAX_DELAY);
  bool exists = i->bits != nullptr;
  xSemaphoreGive(mutex);
  return exists;
}
String catalog() {
  if (!mutex)
    return "{\"ready\":false,\"items\":[]}";
  String j = "{\"ready\":" + String(ready ? "true" : "false") +
             ",\"loading\":" + String(loading ? "true" : "false") +
             ",\"items\":[";
  xSemaphoreTake(mutex, portMAX_DELAY);
  bool first = true;
  for (auto &i : items) {
    if (!first)
      j += ',';
    first = false;
    j += "{\"key\":" + quote(i.key) + ",\"group\":" + quote(i.group) +
         ",\"default\":" + quote(i.factory) + ",\"name\":" + quote(i.name) +
         "}";
  }
  xSemaphoreGive(mutex);
  return j + "]}";
}
bool save(const String &key, const String &name, int width,
          const String &bitmap, String &error) {
  Item *i = find(key.c_str());
  if (!ready || !i) {
    error = "名称存储不可用或项目不存在";
    return false;
  }
  bool reset = name.isEmpty() && width == 0 && bitmap.isEmpty();
  if (!reset && (!utf8(name) || width < 1 || width > 1024)) {
    error = "名称需为1至32个字符，且显示宽度不超过1024像素";
    return false;
  }
  size_t n = ((width + 7) / 8) * 36, size = name.length() + n;
  uint8_t *data = (uint8_t *)heap_caps_malloc(size + 1, MALLOC_CAP_SPIRAM |
                                                            MALLOC_CAP_8BIT);
  if (!data) {
    error = "缓存内存不足";
    return false;
  }
  memcpy(data, name.c_str(), name.length());
  size_t out = 0;
  if (!reset && (bitmap.length() > 6144 ||
                 mbedtls_base64_decode(data + name.length(), n, &out,
                                       (const uint8_t *)bitmap.c_str(),
                                       bitmap.length()) != 0 ||
                 out != n)) {
    free(data);
    error = "字体数据不完整，请重新保存";
    return false;
  }
  Header h{0x314d414e, i->generation + 1, 0, uint16_t(width),
           uint16_t(name.length())};
  h.crc = checksum(h, data, size);
  int bank = 1 - i->bank;
  fs::File f = SPIFFS.open(path(*i, bank), "w");
  bool ok = f && f.write((uint8_t *)&h, sizeof(h)) == sizeof(h) &&
            f.write(data, size) == size;
  f.close();
  Header verify{};
  uint8_t *check = ok ? read(*i, bank, verify) : nullptr;
  ok = check && verify.generation == h.generation;
  free(check);
  if (!ok) {
    free(data);
    error = "保存校验失败，原名称仍保留";
    return false;
  }
  xSemaphoreTake(mutex, portMAX_DELAY);
  free(i->bits);
  memset(i->name, 0, sizeof(i->name));
  memcpy(i->name, name.c_str(), name.length());
  i->width = width;
  i->generation = h.generation;
  i->bank = bank;
  i->last = 0;
  if (reset) {
    free(data);
    i->bits = nullptr;
  } else {
    memmove(data, data + name.length(), n);
    i->bits = data;
  }
  version++;
  xSemaphoreGive(mutex);
  return true;
}
bool reset(const String &key, String &error) {
  return save(key, "", 0, "", error);
}
uint32_t revision() { return version.load(); }
bool draw(TFT_eSprite &s, const char *key, int x, int y, int width,
          bool selected, uint16_t color) {
  if (!mutex || !key)
    return false;
  Item *i = find(key);
  if (!i)
    return false;
  xSemaphoreTake(mutex, portMAX_DELAY);
  if (!i->bits) {
    xSemaphoreGive(mutex);
    return false;
  }
  uint32_t now = millis();
  if (!selected || now - i->last > 250)
    i->start = now;
  i->last = now;
  int excess = max(0, int(i->width) - width), offset = 0;
  if (selected && excess) {
    uint32_t travel = excess * 45 + 800,
             phase = (now - i->start) % (travel + 1800);
    if (phase > 900) {
      float t = min(1.0f, float(phase - 900) / travel);
      float easing = t * t * (3 - 2 * t);
      offset = int(excess * easing);
    }
  }
  int origin = i->width <= width ? (width - i->width) / 2 : 0;
  int stride = (i->width + 7) / 8;
  auto *pixels = (uint16_t *)s.getPointer();
  uint16_t ink = (color << 8) | (color >> 8);
  int sw = s.width(), sh = s.height();
  for (int row = 0; row < 36; row++) {
    if (y + row < 0 || y + row >= sh)
      continue;
    for (int col = 0; col < width; col++) {
      int source = col - origin + offset;
      if (x + col >= 0 && x + col < sw && source >= 0 && source < i->width &&
          (i->bits[row * stride + source / 8] & (0x80 >> (source % 8))))
        pixels[(y + row) * sw + x + col] = ink;
    }
  }
  if (!selected && excess)
    s.fillRect(x + width - 10, y + 29, 8, 3, color);
  xSemaphoreGive(mutex);
  return true;
}
} // namespace UiNames
