#include "atoll_peer_characteristic_weightscale.h"

using namespace Atoll;

PeerCharacteristicWeightscale::PeerCharacteristicWeightscale() {
    snprintf(label, sizeof(label), "%s", "Scale");
    serviceUuid = BLEUUID(WEIGHT_SCALE_SERVICE_UUID);
    charUuid = BLEUUID(WEIGHT_MEASUREMENT_CHAR_UUID);
}

double PeerCharacteristicWeightscale::decode(const uint8_t* data, const size_t length) {
    /// https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.weight_measurement.xml
    ///
    /// Bytes: [Flags: 1][Weight SI: 2][[Weight Imp.: 2]...]
    /// Flags: 0b0000000X;  // Measurement Units 0: SI; 1: Imp.
    /// Weight SI: DecimalExponent: -3, Multiplier: 5
    ///
    if (nullptr == data) {
        log_e("data is null");
        return 0.0;
    }
    uint8_t flags = data[0];
    if (flags & 0b00000001) {
        log_e("not SI");
        return 0.0;
    }
    if (length < 3) {
        log_e("wrong length: %d", length);
        return 0.0;
    }
    int16_t si005 = data[1] | (data[2] << 8);
    lastValue = (double)si005 / 200.0;
    // log_i("%.2f", lastValue);
    return lastValue;
}

bool PeerCharacteristicWeightscale::encode(const double value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}