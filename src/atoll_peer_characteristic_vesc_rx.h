#ifndef __atoll_peer_characteristic_vesc_rx_h
#define __atoll_peer_characteristic_vesc_rx_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicVescRX : public PeerCharacteristicTemplate<String> {
   public:
    PeerCharacteristicVescRX(const char* label = "VescRX",
                             BLEUUID serviceUuid = BLEUUID(VESC_SERVICE_UUID),
                             BLEUUID charUuid = BLEUUID(VESC_RX_CHAR_UUID));
    virtual String decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const String value, uint8_t* data, size_t length) override;
    // virtual bool subscribeOnConnect() override;
};

}  // namespace Atoll

#endif