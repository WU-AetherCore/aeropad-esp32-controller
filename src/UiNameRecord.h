#pragma once
#include <cstddef>
#include <cstdint>
namespace UiNameRecord {
struct Header {
  uint32_t magic, generation, crc;
  uint16_t width, length;
};
inline uint32_t crc(const uint8_t *b, size_t n, uint32_t c = 0xffffffff) {
  while (n--) {
    c ^= *b++;
    for (int i = 0; i < 8; i++)
      c = (c >> 1) ^ ((c & 1) ? 0xedb88320 : 0);
  }
  return c;
}
inline uint32_t checksum(const Header &h, const uint8_t *b, size_t n) {
  uint32_t c = crc((const uint8_t *)&h.generation, 4);
  c = crc((const uint8_t *)&h.width, 4, c);
  return ~crc(b, n, c);
}
inline bool validUtf8(const char *s, size_t length) {
  unsigned count = 0;
  for (unsigned i = 0; i < length;) {
    uint8_t c = s[i++];
    uint32_t cp;
    int n;
    if (c < 128) {
      if (c < 32 || c == 127)
        return false;
      cp = c;
      n = 0;
    } else if (c >= 0xc2 && c <= 0xdf) {
      cp = c & 31;
      n = 1;
    } else if (c >= 0xe0 && c <= 0xef) {
      cp = c & 15;
      n = 2;
    } else if (c >= 0xf0 && c <= 0xf4) {
      cp = c & 7;
      n = 3;
    } else
      return false;
    int continuations = n;
    while (n--) {
      if (i >= length || (uint8_t(s[i]) & 0xc0) != 0x80)
        return false;
      cp = (cp << 6) | (uint8_t(s[i++]) & 63);
    }
    if ((continuations == 2 && cp < 0x800) ||
        (continuations == 3 && cp < 0x10000) || cp > 0x10ffff ||
        (cp >= 0xd800 && cp <= 0xdfff))
      return false;
    if (++count > 32)
      return false;
  }
  return count > 0 && length <= 128;
}
} // namespace UiNameRecord
