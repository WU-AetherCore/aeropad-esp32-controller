#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
namespace PortalPolicy {
constexpr size_t MaxHeader = 4096, MaxBody = 24576,
                 Capacity = MaxHeader + MaxBody;
enum Result { More, Complete, Invalid };
inline bool equal(const uint8_t *p, size_t n, const char *name) {
  size_t m = strlen(name);
  if (n != m)
    return false;
  for (size_t i = 0; i < n; i++) {
    uint8_t c = p[i];
    if (c >= 'A' && c <= 'Z')
      c += 32;
    if (c != uint8_t(name[i]))
      return false;
  }
  return true;
}
// The Arduino parser is called only after a complete, bounded request is
// buffered.
inline Result header(const uint8_t *p, size_t n, size_t &head, size_t &body) {
  head = body = 0;
  for (size_t i = 3; i < n && i < MaxHeader; i++)
    if (p[i - 3] == '\r' && p[i - 2] == '\n' && p[i - 1] == '\r' &&
        p[i] == '\n') {
      head = i + 1;
      break;
    }
  if (!head)
    return n >= MaxHeader ? Invalid : More;
  size_t line = 0;
  while (line + 1 < head && !(p[line] == '\r' && p[line + 1] == '\n'))
    line++;
  if (line < 14 || (memcmp(p, "GET ", 4) != 0 && memcmp(p, "POST ", 5) != 0))
    return Invalid;
  bool length = false;
  size_t at = line + 2;
  while (at + 2 < head) {
    size_t end = at;
    while (end + 1 < head && !(p[end] == '\r' && p[end + 1] == '\n'))
      end++;
    size_t colon = at;
    while (colon < end && p[colon] != ':')
      colon++;
    if (colon == end)
      return Invalid;
    size_t value = colon + 1;
    while (value < end && (p[value] == ' ' || p[value] == '\t'))
      value++;
    if (equal(p + at, colon - at, "transfer-encoding"))
      return Invalid;
    if (equal(p + at, colon - at, "content-length")) {
      if (length || value == end)
        return Invalid;
      length = true;
      size_t result = 0;
      for (size_t i = value; i < end; i++) {
        if (p[i] < '0' || p[i] > '9')
          return Invalid;
        result = result * 10 + p[i] - '0';
        if (result > MaxBody)
          return Invalid;
      }
      body = result;
    }
    if (equal(p + at, colon - at, "content-type") && end - value >= 9 &&
        !memcmp(p + value, "multipart", 9))
      return Invalid;
    at = end + 2;
  }
  return Complete;
}
inline uint16_t be16(const uint8_t *p) { return uint16_t(p[0]) << 8 | p[1]; }
// Captive DNS: support one uncompressed question and EDNS clients without
// unchecked copies.
inline size_t dns(const uint8_t *p, size_t n, uint8_t *out, size_t capacity,
                  const uint8_t ip[4]) {
  if (n < 12 || n > 512 || capacity < 12 || (p[2] & 0xf8) || be16(p + 4) != 1 ||
      be16(p + 6) || be16(p + 8))
    return 0;
  size_t at = 12;
  bool ended = false;
  while (at < n && at - 12 < 255) {
    uint8_t length = p[at++];
    if (!length) {
      ended = true;
      break;
    }
    if (length > 63 || at + length > n || at + length - 12 >= 255)
      return 0;
    at += length;
  }
  if (!ended || at + 4 > n)
    return 0;
  uint16_t type = be16(p + at), cl = be16(p + at + 2);
  size_t question = at + 4;
  bool a = type == 1 && cl == 1;
  if (question + (a ? 16 : 0) > capacity)
    return 0;
  memcpy(out, p, question);
  out[2] = 0x80 | (p[2] & 1);
  out[3] = 0x80;
  out[6] = 0;
  out[7] = a ? 1 : 0;
  out[8] = out[9] = out[10] = out[11] = 0;
  if (a) {
    uint8_t answer[] = {0xc0, 0x0c, 0, 1, 0,     1,     0,     0,
                        0,    30,   0, 4, ip[0], ip[1], ip[2], ip[3]};
    memcpy(out + question, answer, sizeof(answer));
    question += sizeof(answer);
  }
  return question;
}
} // namespace PortalPolicy
