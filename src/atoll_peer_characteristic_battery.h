#ifndef __atoll_peer_characteristic_battery_h
#define __atoll_peer_characteristic_battery_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicBattery : public PeerCharacteristicTemplate<uint8_t> {
   public:
    PeerCharacteristicBattery();

    virtual uint8_t decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const uint8_t value, uint8_t* data, size_t length) override;
};

}  // namespace Atoll

#endif