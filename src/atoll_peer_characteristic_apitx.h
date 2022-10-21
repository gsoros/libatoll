#ifndef __atoll_peer_characteristic_apitx_h
#define __atoll_peer_characteristic_apitx_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicApiTX : public PeerCharacteristicTemplate<String> {
   public:
    PeerCharacteristicApiTX(const char* label = "ApiTX",
                            BLEUUID serviceUuid = BLEUUID(ESPM_API_SERVICE_UUID),
                            BLEUUID charUuid = BLEUUID(API_TX_CHAR_UUID));
    virtual String decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const String value, uint8_t* data, size_t length) override;
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) override;
    virtual void notify() override;
    virtual bool readOnSubscribe() override;
};

}  // namespace Atoll

#endif