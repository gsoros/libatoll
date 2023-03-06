#if !defined(__atoll_peer_characteristic_power_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_power_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicPower : public PeerCharacteristicTemplate<uint16_t> {
   public:
    // lastValue is power
    uint16_t revolutions = 0;
    uint16_t lastCrankEvent = 0;
    ulong lastCrankEventTime = 0;
    uint16_t lastCadence = 0;

    PeerCharacteristicPower();
    // virtual ~PeerCharacteristicPower();

    virtual uint16_t decode(const uint8_t* data, const size_t length);
    virtual bool encode(const uint16_t value, uint8_t* data, size_t length);
};

}  // namespace Atoll

#endif