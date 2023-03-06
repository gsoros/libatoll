#if !defined(__atoll_peer_characteristic_weightscale_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_weightscale_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicWeightscale : public PeerCharacteristicTemplate<double> {
   public:
    PeerCharacteristicWeightscale();

    virtual double decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const double value, uint8_t* data, size_t length) override;
    // virtual bool readOnSubscribe() override { return false; }
};

}  // namespace Atoll

#endif