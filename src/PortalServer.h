#pragma once
#include "PortalPolicy.h"
#include <WebServer.h>
#include <WiFiUdp.h>
#include <cerrno>
#include <esp_heap_caps.h>
#include <lwip/sockets.h>
// A slow or disconnected phone must never spin Stream::timedRead on CPU0.
class PortalServer : public WebServer {
  class BufferedClient : public WiFiClient {
    const uint8_t *data;
    size_t length, position = 0;

  public:
    BufferedClient(const WiFiClient &peer, const uint8_t *p, size_t n)
        : WiFiClient(peer), data(p), length(n) {
      Stream::setTimeout(0);
    }
    int available() override { return length - position; }
    int read() override { return position < length ? data[position++] : -1; }
    int read(uint8_t *dst, size_t n) override {
      n = min(n, length - position);
      memcpy(dst, data + position, n);
      position += n;
      return n;
    }
    int peek() override { return position < length ? data[position] : -1; }
    uint8_t connected() override { return position < length; }
    void flush() override {}
  };
  uint8_t *buffer = nullptr;
  size_t used = 0, expected = 0;
  uint32_t accepted = 0, progress = 0, sendDeadline = 0;
  void discard() {
    _currentClient.stop();
    _currentClient = WiFiClient();
    _currentStatus = HC_NONE;
    _currentUpload.reset();
    used = expected = 0;
  }

public:
  uint32_t requests = 0, timeouts = 0, rejected = 0, maxHandlerMs = 0;
  explicit PortalServer(int port) : WebServer(port) {}
  void begin() override {
    WebServer::begin();
    buffer = (uint8_t *)heap_caps_malloc(PortalPolicy::Capacity,
                                         MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (!buffer) {
      WebServer::close();
      Serial.println("[PORTAL] request buffer allocation failed");
    }
  }
  void close() override {
    discard();
    WebServer::close();
    free(buffer);
    buffer = nullptr;
  }
  void handleClient() override {
    if (!buffer)
      return;
    if (!_currentClient) {
      _currentClient = _server.available();
      if (!_currentClient)
        return;
      used = expected = 0;
      accepted = progress = millis();
      _currentClient.setNoDelay(true);
    }
    uint32_t now = millis();
    if (now - accepted > 3000 || now - progress > 1000) {
      timeouts++;
      discard();
      return;
    }
    int budget = 1536;
    while (budget > 0 && _currentClient.available() > 0) {
      int n = min(budget, _currentClient.available());
      n = min(n, int(PortalPolicy::Capacity - used));
      if (n <= 0) {
        rejected++;
        discard();
        return;
      }
      int received = _currentClient.read(buffer + used, n);
      if (received <= 0)
        break;
      used += received;
      budget -= received;
      progress = now;
      if (!expected) {
        size_t head, body;
        auto result = PortalPolicy::header(buffer, used, head, body);
        if (result == PortalPolicy::Invalid) {
          rejected++;
          discard();
          return;
        }
        if (result == PortalPolicy::Complete)
          expected = head + body;
      }
      if (expected && used >= expected)
        break;
    }
    if (expected && used >= expected) {
      BufferedClient input(_currentClient, buffer, expected);
      uint32_t start = millis();
      if (_parseRequest(input)) {
        sendDeadline = millis() + 1000;
        _contentLength = CONTENT_LENGTH_NOT_SET;
        requests++;
        _handleRequest();
      } else
        rejected++;
      maxHandlerMs = max(maxHandlerMs, uint32_t(millis() - start));
      discard();
    } else if (!_currentClient.connected()) {
      discard();
    }
  }

protected:
  size_t _currentClientWrite(const char *p, size_t n) override {
    size_t sent = 0;
    while (sent < n && int32_t(millis() - sendDeadline) < 0) {
      int fd = _currentClient.fd();
      if (fd < 0)
        break;
      int count =
          ::send(fd, p + sent, min(n - sent, size_t(1460)), MSG_DONTWAIT);
      if (count > 0)
        sent += count;
      else if (count == 0 || (errno != EAGAIN && errno != EWOULDBLOCK))
        break;
      if (sent < n)
        vTaskDelay(max(TickType_t(1), pdMS_TO_TICKS(2)));
    }
    if (sent < n)
      _currentClient.stop();
    return sent;
  }
  size_t _currentClientWrite_P(PGM_P p, size_t n) override {
    return _currentClientWrite(p, n);
  }
};
class PortalDns {
  WiFiUDP udp;
  uint8_t address[4] = {};
  uint8_t input[512], output[512];

public:
  uint32_t rejected = 0;
  bool start(uint16_t port, const char *, const IPAddress &ip) {
    for (int i = 0; i < 4; i++)
      address[i] = ip[i];
    return udp.begin(port);
  }
  void stop() { udp.stop(); }
  void processNextRequest() {
    int n = udp.parsePacket();
    if (n <= 0)
      return;
    if (n > 512) {
      udp.flush();
      rejected++;
      return;
    }
    int got = udp.read(input, n);
    size_t size =
        got > 0 ? PortalPolicy::dns(input, got, output, sizeof(output), address)
                : 0;
    if (!size) {
      rejected++;
      return;
    }
    udp.beginPacket(udp.remoteIP(), udp.remotePort());
    udp.write(output, size);
    udp.endPacket();
  }
};
