#include "../src/PortalPolicy.h"
#include <cassert>
#include <string>
#include <vector>
int main() {
  using namespace PortalPolicy;
  size_t h, b;
  std::string get = "GET /api/live HTTP/1.1\r\nHost: device\r\n\r\n";
  assert(header((uint8_t *)get.data(), get.size(), h, b) == Complete &&
         b == 0 && h == get.size());
  for (size_t n = 0; n < get.size(); n++)
    assert(header((uint8_t *)get.data(), n, h, b) == More);
  std::string post = "POST /api/names HTTP/1.1\r\nContent-Length: 200\r\n\r\n";
  assert(header((uint8_t *)post.data(), post.size(), h, b) == Complete &&
         b == 200);
  for (const char *bad :
       {"Content-Length: 99999999", "Content-Length: -1",
        "Content-Length: 2\r\nContent-Length: 2", "Transfer-Encoding: chunked",
        "Content-Type: multipart/form-data"}) {
    std::string s = "POST /api/names HTTP/1.1\r\n";
    s += bad;
    s += "\r\n\r\n";
    assert(header((uint8_t *)s.data(), s.size(), h, b) == Invalid);
  }
  std::vector<uint8_t> huge(MaxHeader, 'x');
  assert(header(huge.data(), huge.size(), h, b) == Invalid);
  uint8_t q[] = {0x12, 0x34, 1,   0, 0,   1,   0,   0,   0, 0, 0, 0, 3,
                 'w',  'w',  'w', 4, 't', 'e', 's', 't', 0, 0, 1, 0, 1},
          ip[] = {192, 168, 4, 1}, out[512];
  size_t n = dns(q, sizeof(q), out, sizeof(out), ip);
  assert(n == sizeof(q) + 16 && out[7] == 1 && !memcmp(out + n - 4, ip, 4));
  for (size_t i = 0; i < sizeof(q); i++)
    assert(dns(q, i, out, sizeof(out), ip) == 0);
  q[23] = 28;
  assert(dns(q, sizeof(q), out, sizeof(out), ip) == sizeof(q) && out[7] == 0);
  q[23] = 1;
  q[11] = 1;
  assert(dns(q, sizeof(q), out, sizeof(out), ip) == sizeof(q) + 16);
  q[12] = 0xc0;
  assert(!dns(q, sizeof(q), out, sizeof(out), ip));
  q[12] = 63;
  assert(!dns(q, sizeof(q), out, sizeof(out), ip));
  for (unsigned seed = 0; seed < 2000; seed++) {
    std::vector<uint8_t> bytes(seed % 513);
    uint32_t x = seed;
    for (auto &v : bytes) {
      x = x * 1664525u + 1013904223u;
      v = x >> 24;
    }
    assert(dns(bytes.data(), bytes.size(), out, sizeof(out), ip) <=
           sizeof(out));
  }
}
