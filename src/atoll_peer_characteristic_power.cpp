#include "atoll_peer_characteristic_power.h"

using namespace Atoll;

PeerCharacteristicPower::PeerCharacteristicPower() {
    snprintf(label, sizeof(label), "%s", "Power");
    serviceUuid = BLEUUID(CYCLING_POWER_SERVICE_UUID);
    charUuid = BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID);
    // log_i("PeerCharacteristicPower construct, label: %s, char: %s", label, charUuid.toString().c_str());
}

uint16_t PeerCharacteristicPower::decode(const uint8_t* data, const size_t length) {
    if (length < 8) {
        log_e("power reading length < 8");
        return 0;
    }
    /// https://github.com/sputnikdev/bluetooth-gatt-parser/blob/master/src/main/resources/gatt/characteristic/org.bluetooth.characteristic.cycling_power_measurement.xml
    ///
    /// Format: little-endian (but NimBLE converts it to big-endian?)
    /// Bytes: [flags: 2][power: 2(int16)]...[revolutions: 2(uint16)][last crank event: 2(uint16)]
    /// Flags: 0b00000000 00000001;  // Pedal Power Balance Present
    /// Flags: 0b00000000 00000010;  // Pedal Power Balance Reference
    /// Flags: 0b00000000 00000100;  // Accumulated Torque Present
    /// Flags: 0b00000000 00001000;  // Accumulated Torque Source
    /// Flags: 0b00000000 00010000;  // Wheel Revolution Data Present
    /// Flags: 0b00000000 00100000;  // *** Crank Revolution Data Present
    /// Flags: 0b00000000 01000000;  // Extreme Force Magnitudes Present
    /// Flags: 0b00000000 10000000;  // Extreme Torque Magnitudes Present
    /// Flags: 0b00000001 00000000;  // Extreme Angles Present
    /// Flags: 0b00000010 00000000;  // Top Dead Spot Angle Present
    /// Flags: 0b00000100 00000000;  // Bottom Dead Spot Angle Present
    /// Flags: 0b00001000 00000000;  // Accumulated Energy Present
    /// Flags: 0b00010000 00000000;  // Offset Compensation Indicator
    /// Flags: 0b00100000 00000000;  // ReservedForFutureUse
    /// Flags: 0b01000000 00000000;  // ReservedForFutureUse
    /// Flags: 0b10000000 00000000;  // ReservedForFutureUse
    /// crank event time unit: 1/1024s, rolls over
    ///

    uint16_t flags = data[0] | (data[1] << 8);
    // TODO power should be int16_t
    uint16_t power = data[2] | (data[3] << 8);
    // log_i("power: %d", power);

    bool crankRevDataPresent = 0b00100000 & flags;
    // log_i("flags: 0x%02X%s", flags, crankRevDataPresent ? " (crank rev data present)" : "");
    if (!crankRevDataPresent) return power;

    uint8_t offset = 4;
    if (0b00000001 & flags) offset += 2;
    if (0b00000100 & flags) offset += 2;
    if (0b0001000 & flags) offset += 2;
    if (length < offset + 4) {
        log_e("power reading is missing crank rev data");
        return power;
    }
    uint16_t revsIn = data[offset] | (data[offset + 1] << 8);
    // log_i("revsIn: %d", revsIn);
    uint16_t lceIn = data[offset + 2] | (data[offset + 3] << 8);
    // log_i("last crank event: %d", lceIn);

    double dTime = 0.0;
    if (lastCrankEvent > 0) {
        dTime = (lceIn - ((lceIn < lastCrankEvent) ? (lastCrankEvent - UINT16_MAX) : lastCrankEvent)) / 1.024 / 60000;  // 1 minute
    }

    uint16_t cadence = 0;
    uint16_t dRev = 0;
    if (revolutions > 0 && dTime > 0) {
        dRev = revsIn - ((revsIn < revolutions) ? (revolutions - UINT16_MAX) : revolutions);
        cadence = dRev / dTime;
    }

    ulong t = millis();
    if (lastCrankEventTime < t - 2000) cadence = 0;
    if (lastCrankEvent != lceIn) {
        lastCrankEvent = lceIn;
        lastCrankEventTime = t;
    }

    if ((lastCadence != cadence && cadence > 0) || (lastCrankEventTime < t - 2000)) {
        // log_i("TODO notify new cadence: %d", cadence);
    }
    lastCadence = cadence;
    revolutions = revsIn;

    return power;
}

bool PeerCharacteristicPower::encode(const uint16_t value, uint8_t* data, size_t length) {
    log_i("not implemented");
    return true;
}