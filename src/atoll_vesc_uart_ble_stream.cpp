#include "atoll_peer.h"

using namespace Atoll;

VescUartBleStream::VescUartBleStream() {
    rxBuf.clear();
}

int VescUartBleStream::available() {
    return rxBuf.size();
}

int VescUartBleStream::read() {
    if (rxBuf.size()) return (int)rxBuf.shift();
    return 0;
}

int VescUartBleStream::peek() {
    if (rxBuf.size()) return (int)rxBuf.first();
    return 0;
}

size_t VescUartBleStream::write(uint8_t value) {
    return write(&value, 1);
}

size_t VescUartBleStream::write(const uint8_t* buffer, size_t size) {
    if (nullptr == vesc) {
        log_e("vesc is null");
        return 0;
    }
    PeerCharacteristicVescRX* vescRX = (PeerCharacteristicVescRX*)vesc->getChar("VescRX");
    if (nullptr == vescRX) {
        log_e("VescRX char not found");
        return 0;
    }
    if (!vesc->hasClient()) {
        log_e("no client");
        return 0;
    }
    if (!vescRX->write(vesc->getClient(), buffer, size)) {
        log_e("%s could not write RX char", vesc->saved.name);
        return 0;
    }
    // log_d("%d bytes sent to %s RX", size, vesc->saved.name);
    rxBuf.clear();
    // log_d("%s RX buffer cleared", vesc->saved.name);
    return size;
}