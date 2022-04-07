#include "atoll_peer_characteristic_heartrate.h"

using namespace Atoll;

PeerCharacteristicHeartrate::PeerCharacteristicHeartrate() {
    snprintf(label, sizeof(label), "%s", "Heartrate");
    serviceUuid = BLEUUID(HEART_RATE_SERVICE_UUID);
    charUuid = BLEUUID(HEART_RATE_MEASUREMENT_CHAR_UUID);
    // log_i("PeerCharacteristicHeartrate construct, label: %s, char: %s", label, charUuid.toString().c_str());
}

uint16_t PeerCharacteristicHeartrate::decode(const uint8_t* data, const size_t length) {
    if (length < 2) {
        log_e("reading length < 2");
        return 0;
    }

    /// https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.heart_rate_measurement.xml
    ///
    /// Format: little-endian (but NimBLE converts it to big-endian?)
    /// Bytes:
    /// [Flags: 1]
    /// [Heart rate: 1 or 2, depending on bit 0 of the Flags field]
    /// [Energy Expended: 2, presence dependent upon bit 3 of the Flags field]
    /// [RR-Interval: 2, presence dependent upon bit 4 of the Flags field]
    ///
    /// Flags: 0b00000001  // 0: Heart Rate Value Format is set to UINT8, 1: HRVF is UINT16
    /// Flags: 0b00000010  // Sensor Contact Status bit 1
    /// Flags: 0b00000100  // Sensor Contact Status bit 2
    /// Flags: 0b00001000 // Energy Expended Status bit
    /// Flags: 0b00010000 // RR-Interval bit
    /// Flags: 0b00100000 // ReservedForFutureUse
    /// Flags: 0b01000000 // ReservedForFutureUse
    /// Flags: 0b10000000 // ReservedForFutureUse

    uint8_t flags = data[0];
    bool format16bit = 0b00000001 & flags;
    uint16_t heartrate;
    if (format16bit)
        heartrate = data[1] | (data[2] << 8);
    else
        heartrate = data[1] | (0b00000000 << 8);

    return heartrate;
}

bool PeerCharacteristicHeartrate::encode(const uint16_t value, uint8_t* data, size_t length) {
    log_i("not implemented");
    return true;
}