#ifndef __atoll_peer_characteristic_heartrate_h
#define __atoll_peer_characteristic_heartrate_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicHeartrate : public PeerCharacteristicTemplate<uint16_t> {
   public:
    PeerCharacteristicHeartrate();

    virtual uint16_t decode(const uint8_t* data, const size_t length);
    virtual bool encode(const uint16_t value, uint8_t* data, size_t length);
};

}  // namespace Atoll

#endif