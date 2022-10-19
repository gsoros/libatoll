#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic_apitx.h"

using namespace Atoll;

PeerCharacteristicApiTX::PeerCharacteristicApiTX(const char* label,
                                                 BLEUUID serviceUuid,
                                                 BLEUUID charUuid) {
    snprintf(this->label, sizeof(this->label), "%s", label);
    this->serviceUuid = serviceUuid;
    this->charUuid = charUuid;
}

String PeerCharacteristicApiTX::decode(const uint8_t* data, const size_t length) {
    char value[length + 1];
    strncpy(value, (char*)data, length);
    value[length] = 0;
    lastValue = String(value);
    log_i("received length=%d '%s'", length, lastValue.c_str());
    return lastValue;
}

bool PeerCharacteristicApiTX::encode(const String value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

#endif