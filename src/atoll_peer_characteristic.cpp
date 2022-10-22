#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic.h"
#include "atoll_peer.h"
#include "atoll_ble_client.h"

using namespace Atoll;

PeerCharacteristic::~PeerCharacteristic() {
    log_i("destructing %s", label);
}

BLERemoteService* PeerCharacteristic::getRemoteService(BLEClient* client) {
    if (nullptr == client) {
        client = getClient();
    }
    if (nullptr == client) {
        log_e("%s no client", label);
        return nullptr;
    }
    if (!client->isConnected()) {
        log_e("%s client not connected", label);
        return nullptr;
    }
    BLERemoteService* rs = client->getService(serviceUuid);
    if (nullptr == rs)
        log_e("%s could not get remote service", label);
    return rs;
}

BLERemoteCharacteristic* PeerCharacteristic::getRemoteChar(BLEClient* client) {
    if (nullptr == client) {
        client = getClient();
    }
    BLERemoteService* rs = getRemoteService(client);
    if (nullptr == rs) {
        log_e("%s could not get remote service", label);
        return nullptr;
    }
    BLEClient* rsc = rs->getClient();
    if (nullptr == rsc) {
        log_e("%s remote service has no client", label);
        return nullptr;
    }
    if (!rsc->isConnected()) {
        log_e("%s remote service client not connected", label);
        return nullptr;
    }
    BLERemoteCharacteristic* rc = rs->getCharacteristic(charUuid);
    if (nullptr == rc) {
        log_e("%s could not get remote char", label);
        return rc;
    }
    // log_i("%s got remote char", label);
    return rc;
}

bool PeerCharacteristic::subscribe(BLEClient* client) {
    BLERemoteCharacteristic* rc = getRemoteChar(client);
    if (nullptr == rc) return false;
    if (!rc->canNotify() && !rc->canIndicate()) {
        log_e("%s cannot notify or indicate", label);
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
            })) {
        log_e("%s could not subscribe", label);
        return false;
    }
    log_i("%s subscribed", label);
    return true;
}

bool PeerCharacteristic::unsubscribe(BLEClient* client) {
    BLERemoteCharacteristic* rc = getRemoteChar(client);
    if (nullptr == rc) {
        log_e("%s no char", label);
        return false;
    }
    if (!rc->unsubscribe()) {
        log_e("%s unsubscribe failed", label);
        return false;
    }
    log_i("%s unsubscribed", label);
    return true;
}

bool PeerCharacteristic::readOnSubscribe() { return true; }

bool PeerCharacteristic::subscribeOnConnect() { return true; }

BLEClient* PeerCharacteristic::getClient() {
    if (nullptr == peer) {
        log_e("%s peer is null", label);
        return nullptr;
    }
    return peer->getClient();
}

#endif