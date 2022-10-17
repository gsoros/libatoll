#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic.h"
#include "atoll_ble_client.h"

using namespace Atoll;

PeerCharacteristic::~PeerCharacteristic() {
    log_i("destructing %s", label);
}

BLERemoteService* PeerCharacteristic::getRemoteService(BLEClient* client) {
    if (nullptr != remoteService) return remoteService;
    if (nullptr == client) {
        log_e("%s no client", label);
        return nullptr;
    }
    if (!client->isConnected()) {
        log_e("%s client not connected", label);
        return nullptr;
    }
    remoteService = client->getService(serviceUuid);
    if (nullptr == remoteService)
        log_e("%s could not get remote service", label);
    else
        log_i("%s got remote service", label);
    return remoteService;
}

void PeerCharacteristic::unsetRemoteService() {
    remoteService = nullptr;
}

BLERemoteCharacteristic* PeerCharacteristic::getRemoteChar(BLEClient* client) {
    if (nullptr != remoteChar) return remoteChar;
    if (nullptr == remoteService) {
        if (nullptr == client)
            log_e("%s no client", label);
        else if (!client->isConnected())
            log_e("%s client not connected", label);
        else
            remoteService = client->getService(serviceUuid);
    }
    if (nullptr == remoteService) {
        log_e("%s could not get remote service", label);
        return nullptr;
    }
    remoteChar = remoteService->getCharacteristic(charUuid);
    if (nullptr == remoteChar)
        log_e("%s could not get remote char", label);
    else
        log_i("%s got remote char", label);
    return remoteChar;
}

void PeerCharacteristic::unsetRemoteChar() {
    remoteChar = nullptr;
}

bool PeerCharacteristic::subscribe(BLEClient* client) {
    if (!remoteChar) getRemoteChar(client);
    if (!remoteChar) return false;
    if (!remoteChar->canNotify() && !remoteChar->canIndicate()) {
        log_e("'%s' cannot notify or indicate", label);
        return false;
    }
    if (!remoteChar->subscribe(
            remoteChar->canNotify(),
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

bool PeerCharacteristic::unsubscribe() {
    log_i("%s unsub: %schar", label, remoteChar ? "" : "no ");
    if (!remoteChar) return false;
    // remoteChar->unsubscribe(false);
    // log_i("unsub: done");
    // return true;
    bool res = remoteChar->unsubscribe();
    log_i("%s unsub: %s", label, res ? "succ" : "fail");
    return res;
}

bool PeerCharacteristic::readOnSubscribe() { return true; }

bool PeerCharacteristic::subscribeOnConnect() { return true; }

#endif