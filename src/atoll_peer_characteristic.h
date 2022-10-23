#ifndef __atoll_peer_characteristic_h
#define __atoll_peer_characteristic_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_ble_constants.h"
#include "atoll_log.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

namespace Atoll {

class Peer;

class PeerCharacteristic {
   public:
    char label[SETTINGS_STR_LENGTH] = "unnamed characteristic";
    BLEUUID serviceUuid = BLEUUID((uint16_t)0);
    BLEUUID charUuid = BLEUUID((uint16_t)0);
    Peer* peer = nullptr;

    virtual ~PeerCharacteristic();  // virt dtor so we can safely delete

    virtual BLERemoteService* getRemoteService(BLEClient* client = nullptr);
    virtual BLERemoteCharacteristic* getRemoteChar(BLEClient* client = nullptr);

    virtual bool subscribe(BLEClient* client);
    virtual bool unsubscribe(BLEClient* client);
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) = 0;
    virtual bool readOnSubscribe();
    virtual bool subscribeOnConnect();

    virtual BLEClient* getClient();

    virtual void remoteOpStart(BLEClient* client);
    virtual void remoteOpEnd(BLEClient* client);
};

}  // namespace Atoll

#endif