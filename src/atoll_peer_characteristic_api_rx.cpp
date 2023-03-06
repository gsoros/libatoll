#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED) && defined(FEATURE_BLE_CLIENT)

#include "atoll_peer_characteristic_api_rx.h"

using namespace Atoll;

PeerCharacteristicApiRX::PeerCharacteristicApiRX(const char* label,
                                                 BLEUUID serviceUuid,
                                                 BLEUUID charUuid) {
    snprintf(this->label, sizeof(this->label), "%s", label);
    this->serviceUuid = serviceUuid;
    this->charUuid = charUuid;
}

String PeerCharacteristicApiRX::decode(const uint8_t* data, const size_t length) {
    log_e("not implemented");
    lastValue = String("not implemented");
    return lastValue;
}

bool PeerCharacteristicApiRX::encode(const String value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

bool PeerCharacteristicApiRX::subscribeOnConnect() {
    return false;
}

#endif