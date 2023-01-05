#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic_vesc_rx.h"

using namespace Atoll;

PeerCharacteristicVescRX::PeerCharacteristicVescRX(const char* label,
                                                   BLEUUID serviceUuid,
                                                   BLEUUID charUuid) {
    snprintf(this->label, sizeof(this->label), "%s", label);
    this->serviceUuid = serviceUuid;
    this->charUuid = charUuid;
}

String PeerCharacteristicVescRX::decode(const uint8_t* data, const size_t length) {
    log_e("not implemented");
    lastValue = String("not implemented");
    return lastValue;
}

bool PeerCharacteristicVescRX::encode(const String value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

// bool PeerCharacteristicVescRX::subscribeOnConnect() {
//     return false;
// }

#endif