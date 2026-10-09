#include "../src/UiNameRecord.h"
#include <cassert>
#include <cstring>
int main() {
  using namespace UiNameRecord;
  static_assert(sizeof(Header) == 16, "Record layout changed");
  assert(validUtf8(u8"无人机预设一", strlen(u8"无人机预设一")));
  assert(validUtf8(u8"A🚁中文", strlen(u8"A🚁中文")));
  assert(!validUtf8("", 0));
  assert(!validUtf8("\xc0\xaf", 2));
  assert(!validUtf8("\xed\xa0\x80", 3));
  assert(!validUtf8("\xf4\x90\x80\x80", 4));
  assert(!validUtf8("\xe4\xb8", 2));
  assert(!validUtf8("a\nb", 3));
  char longName[34];
  memset(longName, 'a', 33);
  assert(validUtf8(longName, 32));
  assert(!validUtf8(longName, 33));
  uint8_t data[] = {0xe4, 0xb8, 0xad, 0x80, 0x00};
  Header h{0x314d414e, 5, 0, 8, 3};
  auto original = checksum(h, data, sizeof(data));
  data[4] ^= 1;
  assert(checksum(h, data, sizeof(data)) != original);
  data[4] ^= 1;
  h.generation++;
  assert(checksum(h, data, sizeof(data)) != original);
  h.generation--;
  h.width++;
  assert(checksum(h, data, sizeof(data)) != original);
  h.width--;
  h.length--;
  assert(checksum(h, data, sizeof(data)) != original);
  Header reset{0x314d414e, 6, 0, 0, 0};
  assert(checksum(reset, nullptr, 0) != original);
}
