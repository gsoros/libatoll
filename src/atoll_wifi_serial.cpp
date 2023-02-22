#include "atoll_wifi_serial.h"

using namespace Atoll;

void WifiSerial::setup(
    const char *hostName,
    uint16_t port,
    uint8_t maxClients,
    float taskFreq,
    uint32_t taskStack) {
    if (nullptr != hostName) _hostName = hostName;
    if (0 < port) _port = port;
    if (0 < maxClients) _maxClients = maxClients;
    if (0.0 < taskFreq) taskSetFreq(taskFreq);
    if (0 < taskStack) _taskStack = taskStack;
    _server = WiFiServer(_port, _maxClients);
    // log_d("setup: %s:%d, max %d client%s",
    //       WiFi.localIP().toString().c_str(),
    //       _port,
    //       _maxClients,
    //       1 < _maxClients ? "s" : "");
    _rxBuf.clear();
    _txBuf.clear();
}

void WifiSerial::loop() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        log_i("Wifi is disabled, task should be stopped");
        return;
    }
    if (!WiFi.isConnected() && !WiFi.softAPgetStationNum()) return;
    if (!_connected) {
        if (!_server.hasClient()) return;
        _client = _server.available();
        if (!_client) return;
        _connected = true;
        // board.led.blink(5);
        _client.printf("%s %s %s\n", _hostName, __DATE__, __TIME__);
        // board.status.printHeader();
    } else if (!_client.connected()) {
        disconnect();
        return;
    }
    while (0 < _client.available()) _rxBuf.push(_client.read());
    while (0 < _txBuf.size()) _client.write(_txBuf.shift());
    /*
    static char buf[WIFISERIAL_RINGBUF_TX_SIZE];
    strncpy(buf, "", sizeof(buf));
    while (0 < _txBuf.size() && strlen(buf) < sizeof(buf) - 1) {
        const char c = _txBuf.shift();
        strcat(buf, &c);
    }
    if (0 < strlen(buf))
        _client.write((const uint8_t *)buf, strlen(buf));
    */

    if (_disconnect) {
        flush();
        _disconnect = false;
        disconnect();
    }
}

void WifiSerial::start() {
    _server.begin(_port);
}

void WifiSerial::stop() {
    log_i("Shutting down");
    disconnect();
    taskStop();
    _server.end();
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
        _txBuf.push(*buf++);
        size--;
    }
    return written;
}

int WifiSerial::available() {
    return _rxBuf.size();
}

int WifiSerial::read() {
    if (available()) {
        char c = _rxBuf.shift();
        switch (c) {
            case 4:
                print("Ctrl-D received, bye.\n");
                _disconnect = true;
                // return -1;
                break;
                // case 10:
                //     print("[LF]");
                //     break;
                // case 13:
                //     print("[CR]");
                //     break;
        }
        return c;
    }
    return -1;
}

int WifiSerial::peek() {
    return available() ? _rxBuf.first() : 0;
}

void WifiSerial::flush() {
    _client.flush();
}
