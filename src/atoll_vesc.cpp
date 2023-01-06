#include "atoll_vesc.h"

using namespace Atoll;

Vesc::Vesc(Saved saved,
           PeerCharacteristicVescRX* customVescRX,
           PeerCharacteristicVescTX* customVescTX)
    : Peer(saved) {
    addChar(nullptr != customVescRX
                ? customVescRX
                : new PeerCharacteristicVescRX());
    PeerCharacteristicVescTX* vescTX = nullptr != customVescTX
                                           ? customVescTX
                                           : new PeerCharacteristicVescTX();
    vescTX->stream = &bleStream;
    addChar(vescTX);
    bleStream.vesc = this;
}

int VescUartBleStream::available() {
    log_e("not implemented");
    return 0;
}

int VescUartBleStream::read() {
    log_e("not implemented");
    return 0;
}

int VescUartBleStream::peek() {
    log_e("not implemented");
    return 0;
}

size_t VescUartBleStream::write(uint8_t) {
    log_e("not implemented");
    return 0;
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
    log_d("%d bytes sent to %s RX", size, vesc->saved.name);
    return size;
}