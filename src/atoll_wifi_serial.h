#ifndef __atoll_wifi_serial_h
#define __atoll_wifi_serial_h

#include <Arduino.h>
#include <Stream.h>
#include <WiFi.h>
#include <CircularBuffer.h>

#include "atoll_task.h"

#ifndef WIFISERIAL_RINGBUF_RX_SIZE
#define WIFISERIAL_RINGBUF_RX_SIZE 64
#endif
#ifndef WIFISERIAL_RINGBUF_TX_SIZE
#define WIFISERIAL_RINGBUF_TX_SIZE 256
#endif
#ifndef WIFISERIAL_PORT
#define WIFISERIAL_PORT 23
#endif
#ifndef WIFISERIAL_MAXCLIENTS
#define WIFISERIAL_MAXCLIENTS 1  // max 1 client as not async
#endif

namespace Atoll {

class WifiSerial : public Task, public Stream {
   public:
    const char *taskName() { return "WifiSerial"; }
    void setup(const char *hostName = nullptr,
               uint16_t port = 0,
               uint8_t maxClients = 0,
               float taskFreq = 0.0,
               uint32_t taskStack = 0);
    void loop();
    void start();
    void stop();
    void disconnect();
    size_t write(uint8_t c);
    size_t write(const uint8_t *buf, size_t size);

    int available();
    int read();
    int peek();
    void flush();

   private:
    WiFiServer _server;
    WiFiClient _client;
    CircularBuffer<char, WIFISERIAL_RINGBUF_RX_SIZE> _rxBuf;
    CircularBuffer<char, WIFISERIAL_RINGBUF_TX_SIZE> _txBuf;  // TODO mutex
    bool _connected = false;
    bool _disconnect = false;
    const char *_hostName = "libAtoll_unamed";
    uint16_t _port = WIFISERIAL_PORT;
    uint8_t _maxClients = WIFISERIAL_MAXCLIENTS;
};

}  // namespace Atoll

#endif