#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic_vesc_tx.h"

using namespace Atoll;

PeerCharacteristicVescTX::PeerCharacteristicVescTX(const char* label,
                                                   BLEUUID serviceUuid,
                                                   BLEUUID charUuid) {
    snprintf(this->label, sizeof(this->label), "%s", label);
    this->serviceUuid = serviceUuid;
    this->charUuid = charUuid;
}

uint8_t PeerCharacteristicVescTX::decode(const uint8_t* data, const size_t length) {
    log_d("%s decoding %dB", label, length);
    if (nullptr == stream) {
        log_e("%s stream is null", label);
        return 0;
    }
    for (size_t i = 0; i < length; i++) {
        log_d("%s pushing %d: %d into RX buffer", label, i, data[i]);
        stream->rxBuf.push(data[i]);
    }
    return 0;
}

bool PeerCharacteristicVescTX::encode(const uint8_t value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

void PeerCharacteristicVescTX::onNotify(BLERemoteCharacteristic* rc, uint8_t* data, size_t length, bool isNotify) {
    log_d("%s received %dB", label, length);
    decode(data, length);
}

void PeerCharacteristicVescTX::notify() {
    log_e("not implemented");
}

bool PeerCharacteristicVescTX::readOnSubscribe() {
    return false;
}

#endif