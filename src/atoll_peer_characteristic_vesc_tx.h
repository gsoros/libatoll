#if !defined(__atoll_peer_characteristic_vesc_tx_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_vesc_tx_h

#include "atoll_peer_characteristic_template.h"
#include "atoll_vesc_uart_ble_stream.h"

namespace Atoll {

class VescUartBleStream;

class PeerCharacteristicVescTX : public PeerCharacteristicTemplate<uint8_t> {
   public:
    PeerCharacteristicVescTX(const char* label = "VescTX",
                             BLEUUID serviceUuid = BLEUUID(VESC_SERVICE_UUID),
                             BLEUUID charUuid = BLEUUID(VESC_TX_CHAR_UUID));
    virtual uint8_t decode(const uint8_t* data, const size_t length) override;
    virtual bool encode(const uint8_t value, uint8_t* data, size_t length) override;
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) override;
    virtual void notify() override;
    virtual bool readOnSubscribe() override;

    VescUartBleStream* stream = nullptr;
};

}  // namespace Atoll

#endif