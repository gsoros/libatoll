#ifndef __atoll_peer_characteristic_h
#define __atoll_peer_characteristic_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_ble_constants.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

namespace Atoll {

class PeerCharacteristic {
   public:
    char label[SETTINGS_STR_LENGTH] = "unnamed characteristic";
    BLEUUID serviceUuid = BLEUUID((uint16_t)0);
    BLEUUID charUuid = BLEUUID((uint16_t)0);
    BLERemoteService* service = nullptr;
    BLERemoteCharacteristic* characteristic = nullptr;

    virtual ~PeerCharacteristic();  // virt dtor so we can safely delete

    virtual bool subscribe(BLEClient* client);
    virtual bool unsubscribe();
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) = 0;
    virtual bool readOnSubscribe() { return true; }
};

}  // namespace Atoll

#endif