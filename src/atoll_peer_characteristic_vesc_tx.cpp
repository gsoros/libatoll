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

String PeerCharacteristicVescTX::decode(const uint8_t* data, const size_t length) {
    log_d("%s decoding %dB", label, length);
    char value[length + 1];
    strncpy(value, (char*)data, length);
    value[length] = 0;
    lastValue = String(value);
    log_d("%s length=%d '%s'", label, length, lastValue.c_str());
    return lastValue;
}

bool PeerCharacteristicVescTX::encode(const String value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

void PeerCharacteristicVescTX::onNotify(BLERemoteCharacteristic* rc, uint8_t* data, size_t length, bool isNotify) {
    log_d("%s received %dB", label, length);
    lastValue = decode(data, length);
    notify();
}

void PeerCharacteristicVescTX::notify() {
    log_d("%s received %dB", label, lastValue.length());
    if (nullptr == stream) return;
    // stream->write((uint8_t*)lastValue.c_str(), (size_t)lastValue.length());
    for (unsigned int i = 0; i < lastValue.length(); i++)
        stream->rxBuf.push(lastValue.charAt(i));
}

bool PeerCharacteristicVescTX::readOnSubscribe() {
    return false;
}

#endif