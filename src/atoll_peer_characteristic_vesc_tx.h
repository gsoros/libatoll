#ifndef __atoll_peer_characteristic_vesc_tx_h
#define __atoll_peer_characteristic_vesc_tx_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicVescTX : public PeerCharacteristicTemplate<String> {
   public:
    PeerCharacteristicVescTX(const char* label = "VescTX",
                             BLEUUID serviceUuid = BLEUUID(VESC_SERVICE_UUID),
                             BLEUUID charUuid = BLEUUID(VESC_TX_CHAR_UUID));
    virtual String decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const String value, uint8_t* data, size_t length) override;
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) override;
    virtual void notify() override;
    virtual bool readOnSubscribe() override;
};

}  // namespace Atoll

#endif