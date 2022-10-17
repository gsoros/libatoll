#ifndef __atoll_peer_characteristic_apirx_h
#define __atoll_peer_characteristic_apirx_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicApiRX : public PeerCharacteristicTemplate<String> {
   public:
    PeerCharacteristicApiRX(const char* label = "ApiRX",
                            BLEUUID serviceUuid = BLEUUID(ESPM_API_SERVICE_UUID),
                            BLEUUID charUuid = BLEUUID(API_RX_CHAR_UUID));
    virtual String decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const String value, uint8_t* data, size_t length) override;
    virtual bool subscribeOnConnect() override;
};

}  // namespace Atoll

#endif