#include "atoll_peer_characteristic_battery.h"

using namespace Atoll;

PeerCharacteristicBattery::PeerCharacteristicBattery() {
    snprintf(label, sizeof(label), "%s", "Battery");
    serviceUuid = BLEUUID(BATTERY_SERVICE_UUID);
    charUuid = BLEUUID(BATTERY_LEVEL_CHAR_UUID);
}

uint8_t PeerCharacteristicBattery::decode(const uint8_t* data, const size_t length) {
    /// https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.battery_level.xml
    ///
    /// Format: little-endian (but NimBLE converts it to big-endian?)
    /// Bytes:
    /// [Level: 1] The current charge level of a battery. 100 represents fully charged while
    /// 0 represents fully discharged.
    ///
    uint8_t level = data[0];
    // log_i("decoded %d%", level);
    if (100 < level) log_e("level too high: %d", level);
    return level;
}

bool PeerCharacteristicBattery::encode(const uint8_t value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}