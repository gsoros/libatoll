#if !defined(__atoll_peer_characteristic_vesc_rx_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_vesc_rx_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicVescRX : public PeerCharacteristicTemplate<uint8_t> {
   public:
    PeerCharacteristicVescRX(const char* label = "VescRX",
                             BLEUUID serviceUuid = BLEUUID(VESC_SERVICE_UUID),
                             BLEUUID charUuid = BLEUUID(VESC_RX_CHAR_UUID));

    virtual uint8_t decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const uint8_t value, uint8_t* data, size_t length) override;
    virtual bool subscribeOnConnect() override;

    virtual bool write(BLEClient* client, const uint8_t* value, size_t length);
};

}  // namespace Atoll

#endif