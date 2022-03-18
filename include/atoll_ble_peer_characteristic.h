#ifndef __atoll_ble_peer_characteristic_h
#define __atoll_ble_peer_characteristic_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_ble_constants.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

namespace Atoll {

class BlePeerCharacteristic {
   public:
    char name[SETTINGS_STR_LENGTH] = "unnamed characteristic";
    BLEUUID serviceUuid = BLEUUID((uint16_t)0);
    BLEUUID charUuid = BLEUUID((uint16_t)0);
    BLERemoteService* service = nullptr;
    BLERemoteCharacteristic* characteristic = nullptr;

    BlePeerCharacteristic() {
        log_i("BlePeerCharacteristic construct");
    }
    virtual ~BlePeerCharacteristic();

    virtual bool subscribe(BLEClient* client);
    virtual bool unSubscribe();
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify);
};

template <class T>
class BlePeerCharacteristicTemplate : public BlePeerCharacteristic {
   public:
    T lastValue = (T)NULL;

    BlePeerCharacteristicTemplate() : BlePeerCharacteristic() {
        log_i("BlePeerCharacteristicTemplate construct");
    }
    virtual ~BlePeerCharacteristicTemplate();

    virtual T decode(const uint8_t* data, const size_t length);
    virtual bool encode(const T value, uint8_t* data, size_t length);

    virtual T read();
    virtual bool write(T value, size_t length);

    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify);
};

class PowerPeerCharacteristic : public BlePeerCharacteristicTemplate<uint16_t> {
   public:
    PowerPeerCharacteristic() : BlePeerCharacteristicTemplate<uint16_t>() {
        strncpy(name, "Power", sizeof(name));
        serviceUuid = BLEUUID(CYCLING_POWER_SERVICE_UUID);
        charUuid = BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID);
        log_i("PowerPeerCharacteristic construct, name: %s, char: %s", name, charUuid.toString().c_str());
    }
    virtual ~PowerPeerCharacteristic();

    virtual uint16_t decode(const uint8_t* data, const size_t length) {
        log_i("not implemented");
        return 0;
    }

    virtual bool encode(const uint16_t value, uint8_t* data, size_t length) {
        log_i("not implemented");
        return true;
    };
};

}  // namespace Atoll

#endif