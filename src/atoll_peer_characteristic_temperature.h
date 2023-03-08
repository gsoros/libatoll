#if !defined(__atoll_peer_characteristic_temperature_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_temperature_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicTemperature : public PeerCharacteristicTemplate<float> {
   public:
    PeerCharacteristicTemperature();

    virtual float decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const float value, uint8_t* data, size_t length) override;
};

}  // namespace Atoll

#endif