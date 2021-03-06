#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic.h"
#include "atoll_ble_client.h"

using namespace Atoll;

PeerCharacteristic::~PeerCharacteristic() {
    log_i("destructing %s", label);
}

bool PeerCharacteristic::subscribe(BLEClient* client) {
    if (!characteristic) {
        if (!service)
            service = client->getService(serviceUuid);
        if (!service) {
            log_e("could not find service for '%s'", label);
            return false;
        }
    }
    if (!characteristic)
        characteristic = service->getCharacteristic(charUuid);
    if (!characteristic) {
        log_e("could not find '%s' characteristic", label);
        return false;
    }
    if (!characteristic->canNotify() && !characteristic->canIndicate()) {
        log_e("'%s' cannot notify or indicate", label);
        return false;
    }
    if (!characteristic->subscribe(
            characteristic->canNotify(),
            [this](
                BLERemoteCharacteristic* c,
                uint8_t* data,
                size_t length,
                bool isNotify) {
                onNotify(c, data, length, isNotify);
            },
            false)) {
        log_e("could not subscribe to '%s' characteristic", label);
        return false;
    }
    log_i("subscribed %s", label);
    return true;
}

bool PeerCharacteristic::unsubscribe() {
    log_i("unsub: %schar", characteristic ? "" : "no ");
    if (!characteristic) return false;
    // characteristic->unsubscribe(false);
    // log_i("unsub: done");
    // return true;
    bool res = characteristic->unsubscribe();
    log_i("unsub: %s", res ? "succ" : "fail");
    return res;
}

#endif