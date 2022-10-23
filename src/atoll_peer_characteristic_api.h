#ifndef __atoll_peer_characteristic_api_h
#define __atoll_peer_characteristic_api_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicApi : public PeerCharacteristicTemplate<String> {
   public:
    virtual void remoteOpStart(BLEClient* client) override;
    virtual void remoteOpEnd(BLEClient* client) override;

   protected:
    virtual bool needsSecurity(BLEClient* client);
    uint8_t remoteOpIOCap = BLE_HS_IO_KEYBOARD_ONLY;
};

}  // namespace Atoll

#endif