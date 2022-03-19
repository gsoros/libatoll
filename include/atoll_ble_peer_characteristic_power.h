#ifndef __atoll_ble_peer_characteristic_power_h
#define __atoll_ble_peer_characteristic_power_h

#include "atoll_ble_peer_characteristic_template.h"

namespace Atoll {

class BlePeerCharacteristicPower : public BlePeerCharacteristicTemplate<uint16_t> {
   public:
    // lastValue is power
    uint16_t revolutions = 0;
    uint16_t lastCrankEvent = 0;
    ulong lastCrankEventTime = 0;
    uint16_t lastCadence = 0;

    BlePeerCharacteristicPower();
    // virtual ~BlePeerCharacteristicPower();

    virtual uint16_t decode(const uint8_t* data, const size_t length);
    virtual bool encode(const uint16_t value, uint8_t* data, size_t length);
};

}  // namespace Atoll

#endif