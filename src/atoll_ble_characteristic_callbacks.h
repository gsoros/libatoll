#if !defined(__atoll_ble_characteristic_callbacks_h) && defined(FEATURE_BLE_SERVER)
#define __atoll_ble_characteristic_callbacks_h

#include <Arduino.h>
#include <NimBLEDevice.h>

namespace Atoll {

class BleCharacteristicCallbacks : public BLECharacteristicCallbacks {
   public:
    virtual void onRead(BLECharacteristic *c, BLEConnInfo &connInfo) override;
    virtual void onWrite(BLECharacteristic *c, BLEConnInfo &connInfo) override;
    virtual void onNotify(BLECharacteristic *c) override;
    virtual void onStatus(BLECharacteristic *c, int code) override;
    virtual void onSubscribe(BLECharacteristic *c, BLEConnInfo &connInfo, uint16_t subValue) override;
};

}  // namespace Atoll

#endif