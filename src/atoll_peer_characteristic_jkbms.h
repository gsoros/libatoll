#if !defined(__atoll_peer_characteristic_jkbms_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_jkbms_h

#include "atoll_peer_characteristic_template.h"

namespace Atoll {

class PeerCharacteristicJkBms : public PeerCharacteristicTemplate<String> {
   public:
    PeerCharacteristicJkBms(const char* label = "JkBms",
                            BLEUUID serviceUuid = BLEUUID(SERVICE_UUID),
                            BLEUUID charUuid = BLEUUID(CHAR_UUID));
    virtual String decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const String value, uint8_t* data, size_t length) override;
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) override;
    virtual void notify() override;
    virtual bool subscribeOnConnect() override;
    virtual bool readOnSubscribe() override;

    // https://github.com/syssi/esphome-jk-bms/blob/main/components/jk_bms_ble/jk_bms_ble.cpp
    static const uint16_t SERVICE_UUID = 0xffe0;
    static const uint16_t CHAR_UUID = 0xffe1;
};

}  // namespace Atoll

#endif