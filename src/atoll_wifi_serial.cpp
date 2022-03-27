#include "atoll_wifi_serial.h"

using namespace Atoll;

void WifiSerial::setup() { setup(WIFISERIAL_PORT, WIFISERIAL_MAXCLIENTS); }
void WifiSerial::setup(uint16_t port, uint8_t maxClients) {
    _server = WiFiServer(port, maxClients);
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        log_i("Wifi is disabled, not starting");
        return;
    }
    log_i("starting on %s:%d, max %d client%s",
          WiFi.localIP().toString().c_str(),
          port,
          maxClients,
          1 < maxClients ? "s" : "");
    _server.begin();
}

void WifiSerial::loop() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        log_i("Wifi is disabled, task should be stopped");
        return;
    }
    if (!WiFi.isConnected() && WiFi.softAPgetStationNum() < 1) return;
    if (!_connected) {
        if (!_server.hasClient()) return;
        _client = _server.available();
        if (!_client) return;
        _connected = true;
        log_i("Client connected");
        // board.led.blink(5);
        //_client.printf("Welcome to %s.\n", board.hostName);
        // board.status.printHeader();
    } else if (!_client.connected()) {
        disconnect();
        return;
    }
    while (0 < _client.available()) {
        _rx_buf.push(_client.read());
    }
    while (0 < _tx_buf.size()) {
        _client.write(_tx_buf.shift());
    }
    if (_disconnect) {
        flush();
        _disconnect = false;
        disconnect();
    }
}

void WifiSerial::off() {
    log_i("Shutting down");
    disconnect();
    taskStop();
}

void WifiSerial::disconnect() {
    if (_client.connected()) {
        _client.write(7);  // BEL
        _client.write(4);  // EOT
        _client.write('\n');
    }
    _client.stop();
    _connected = false;
    log_i("Client disconnected");
    // board.led.blink(5);
}

size_t WifiSerial::write(uint8_t c) {
    return write(&c, 1);
}

size_t WifiSerial::write(const uint8_t *buf, size_t size) {
    // TODO mutex
    size_t written = size;
    while (size) {
        _tx_buf.push(*buf++);
        size--;
    }
    return written;
}

int WifiSerial::available() {
    return _rx_buf.size();
}

int WifiSerial::read() {
    if (available()) {
        char c = _rx_buf.shift();
        switch (c) {
            case 4:
                print("Ctrl-D received, bye.\n");
                _disconnect = true;
                // return -1;
        }
        return c;
    }
    return -1;
}

int WifiSerial::peek() {
    return available() ? _rx_buf.first() : 0;
}

void WifiSerial::flush() {
    _client.flush();
}