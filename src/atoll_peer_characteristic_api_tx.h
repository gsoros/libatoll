#if !defined(__atoll_peer_characteristic_api_tx_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_api_tx_h

#include "atoll_peer_characteristic_api.h"

namespace Atoll {

class PeerCharacteristicApiTX : public PeerCharacteristicApi {
   public:
    bool lastValueDirty = false;

    PeerCharacteristicApiTX(const char* label = "ApiTX",
                            BLEUUID serviceUuid = BLEUUID(ESPM_API_SERVICE_UUID),
                            BLEUUID charUuid = BLEUUID(API_TX_CHAR_UUID));
    virtual String decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const String value, uint8_t* data, size_t length) override;
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) override;
    virtual void notify() override;
    virtual bool readOnSubscribe() override;

    virtual void loop();
};

}  // namespace Atoll

#endif