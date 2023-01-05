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
    lastValue = decode(data, length);
    notify();
}

void PeerCharacteristicVescTX::notify() {
    log_d("%s received '%s'", label, lastValue.c_str());
}

bool PeerCharacteristicVescTX::readOnSubscribe() {
    return false;
}

#endif