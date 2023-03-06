#if !defined(__atoll_peer_characteristic_heartrate_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_heartrate_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicHeartrate : public PeerCharacteristicTemplate<uint16_t> {
   public:
    PeerCharacteristicHeartrate();

    virtual uint16_t decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const uint16_t value, uint8_t* data, size_t length) override;
    virtual bool readOnSubscribe() override { return false; }  // my HRM does not allow read
};

}  // namespace Atoll

#endif