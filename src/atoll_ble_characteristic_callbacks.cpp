#include "atoll_ble_characteristic_callbacks.h"
#include "atoll_ble_constants.h"
#include "atoll_log.h"

using namespace Atoll;

void BleCharacteristicCallbacks::onRead(BLECharacteristic *c, BLEConnInfo &connInfo) {
    // log_d("%s: value: %s", c->getUUID().toString().c_str(), c->getValue().c_str());
}

void BleCharacteristicCallbacks::onWrite(BLECharacteristic *c, BLEConnInfo &connInfo) {
    // log_d("%s: value: %s", c->getUUID().toString().c_str(), c->getValue().c_str());
}

void BleCharacteristicCallbacks::onNotify(BLECharacteristic *c) {
    // log_d("%d", c->getValue<int>());
}

void BleCharacteristicCallbacks::onStatus(BLECharacteristic *c, int code) {
    // log_d("char: %s, code: %d", c->getUUID().toString().c_str(), code);
}

void BleCharacteristicCallbacks::onSubscribe(BLECharacteristic *c, BLEConnInfo &info, uint16_t subValue) {
    char remote[64] = "";
    snprintf(remote, sizeof(remote), "client ID: %d Address: %s (%s)",
             info.getConnHandle(),
             info.getAddress().toString().c_str(), info.getIdAddress().toString().c_str());

    char uuid[40] = "";
    strncpy(uuid, c->getUUID().toString().c_str(), sizeof(uuid));
    switch (subValue) {
        case 0:
            log_d("%s unsubscribed from %s", remote, uuid);
            break;
        case 1:
            log_d("%s subscribed to notfications for %s", remote, uuid);
            break;
        case 2:
            log_d("%s subscribed to indications for %s", remote, uuid);
            break;
        case 3:
            log_d("%s subscribed to notifications and indications for %s", remote, uuid);
            break;
        default:
            log_d("%s did something to %s", remote, uuid);
    }
}