#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED) && defined(FEATURE_BLE_CLIENT)

#include "atoll_peer_characteristic_temperature.h"

using namespace Atoll;

PeerCharacteristicTemperature::PeerCharacteristicTemperature() {
    snprintf(label, sizeof(label), "%s", "Temperature");
    serviceUuid = BLEUUID(ENVIRONMENTAL_SENSING_SERVICE_UUID);
    charUuid = BLEUUID(TEMPERATURE_CHAR_UUID);
}

float PeerCharacteristicTemperature::decode(const uint8_t* data, const size_t length) {
    if (length < 2) {
        log_e("length < 2");
        return 0.0f;
    }

    /// https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.temperature.xml
    ///
    /// Format: sint16 little-endian
    /// Bytes:
    /// [Temperature: 2] ˚C * 100
    return (float)(((int16_t)data[0] | ((int16_t)data[1] << 8))) / 100;
}

bool PeerCharacteristicTemperature::encode(const float value, uint8_t* data, size_t length) {
    log_i("not implemented");
    return true;
}

#endif