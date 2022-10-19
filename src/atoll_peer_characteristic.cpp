#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic.h"
#include "atoll_ble_client.h"

using namespace Atoll;

PeerCharacteristic::~PeerCharacteristic() {
    log_i("destructing %s", label);
}

BLERemoteService* PeerCharacteristic::getRemoteService(BLEClient* client) {
    if (nullptr == client) {
        log_e("%s no client", label);
        return nullptr;
    }
    if (!client->isConnected()) {
        log_e("%s client not connected", label);
        return nullptr;
    }
    log_i("%s getting remote service for uuid %s", label, serviceUuid.toString().c_str());
    BLERemoteService* rs = client->getService(serviceUuid);
    if (nullptr == rs)
        log_e("%s could not get remote service", label);
    else {
        log_i("%s got remote service", label);
        // log_i("%s discovering chars", label);
        // rs->getCharacteristics();
    }
    return rs;
}

BLERemoteCharacteristic* PeerCharacteristic::getRemoteChar(BLEClient* client) {
    BLERemoteService* rs = getRemoteService(client);
    if (nullptr == rs) {
        log_e("%s could not get remote service", label);
        return nullptr;
    }
    BLEClient* c = rs->getClient();
    if (nullptr == c) {
        log_e("%s remote service has no client", label);
        return nullptr;
    }
    if (!c->isConnected()) {
        log_e("%s remote service client not connected", label);
        return nullptr;
    }
    log_i("%s getting remote char for uuid %s", label, charUuid.toString().c_str());
    BLERemoteCharacteristic* rc = rs->getCharacteristic(charUuid);
    if (nullptr == rc)
        log_e("%s could not get remote char", label);
    else
        log_i("%s got remote char", label);
    return rc;
}

bool PeerCharacteristic::subscribe(BLEClient* client) {
    BLERemoteCharacteristic* rc = getRemoteChar(client);
    if (nullptr == rc) return false;
    if (!rc->canNotify() && !rc->canIndicate()) {
        log_e("'%s' cannot notify or indicate", label);
        return false;
    }
    if (!rc->subscribe(
            rc->canNotify(),
            [this](
                BLERemoteCharacteristic* c,
                uint8_t* data,
                size_t length,
                bool isNotify) {
                onNotify(c, data, length, isNotify);
            },
            false)) {
        log_e("could not subscribe to '%s' remoteChar", label);
        return false;
    }
    log_i("subscribed %s", label);
    return true;
}

bool PeerCharacteristic::unsubscribe(BLEClient* client) {
    BLERemoteCharacteristic* rc = getRemoteChar(client);
    log_i("%s unsub: %schar", label, rc ? "" : "no ");
    if (nullptr == rc) return false;
    bool res = rc->unsubscribe();
    log_i("%s unsub: %s", label, res ? "succ" : "fail");
    return res;
}

bool PeerCharacteristic::readOnSubscribe() { return true; }

bool PeerCharacteristic::subscribeOnConnect() { return true; }

#endif