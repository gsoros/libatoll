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

uint8_t PeerCharacteristicVescRX::decode(const uint8_t* data, const size_t length) {
    log_e("not implemented");
    return lastValue;
}

bool PeerCharacteristicVescRX::encode(const uint8_t value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

bool PeerCharacteristicVescRX::subscribeOnConnect() {
    return false;
}

bool PeerCharacteristicVescRX::write(BLEClient* client, const uint8_t* value, size_t length) {
    BLERemoteCharacteristic* rc = getRemoteChar(client);
    if (!rc) return false;
    if (!rc->canWrite()) {
        log_e("%s not writable", label);
        return false;
    }
    remoteOpStart(client);
    bool res = rc->writeValue(value, length);
    remoteOpEnd(client);
    return res;
}

#endif