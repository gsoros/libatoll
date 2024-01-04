#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED) && defined(FEATURE_BLE_CLIENT)

#include "atoll_peer_characteristic_jkbms.h"

using namespace Atoll;

PeerCharacteristicJkBms::PeerCharacteristicJkBms(const char* label,
                                                 BLEUUID serviceUuid,
                                                 BLEUUID charUuid) {
    snprintf(this->label, sizeof(this->label), "%s", label);
    this->serviceUuid = serviceUuid;
    this->charUuid = charUuid;
}

String PeerCharacteristicJkBms::decode(const uint8_t* data, const size_t length) {
    log_e("not implemented");
    return lastValue;
}

bool PeerCharacteristicJkBms::encode(const String value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

void PeerCharacteristicJkBms::onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) {
    log_d("onNotify");
}

void PeerCharacteristicJkBms::notify() {
}

bool PeerCharacteristicJkBms::subscribeOnConnect() {
    return true;
}

bool PeerCharacteristicJkBms::readOnSubscribe() {
    return false;
}

#endif