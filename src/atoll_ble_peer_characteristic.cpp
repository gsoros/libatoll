#include "atoll_ble_peer_characteristic.h"

using namespace Atoll;

BlePeerCharacteristic::~BlePeerCharacteristic() {}

bool BlePeerCharacteristic::subscribe(BLEClient* client) {
    if (!characteristic) {
        if (!service)
            service = client->getService(serviceUuid);
        if (!service) {
            log_e("could not find service for '%s'", name);
            return false;
        }
    }
    if (!characteristic)
        characteristic = service->getCharacteristic(charUuid);
    if (!characteristic) {
        log_e("could not find '%s' characteristic", name);
        return false;
    }
    if (!characteristic->subscribe(
            true,
            [this](
                BLERemoteCharacteristic* c,
                uint8_t* data,
                size_t length,
                bool isNotify) {
                onNotify(c, data, length, isNotify);
            })) {
        log_e("could not subscribe to '%s' characteristic", name);
        return false;
    }
    log_i("subscribed %s", name);
    return true;
}

bool BlePeerCharacteristic::unSubscribe() {
    if (!characteristic) return false;
    return characteristic->unsubscribe();
}

void BlePeerCharacteristic::onNotify(
    BLERemoteCharacteristic* c,
    uint8_t* data,
    size_t length,
    bool isNotify) {
    char buf[length];
    strncpy(buf, (char*)data, length);
    log_i("char: %s, data: '%s', len: %d",
          c->getUUID().toString().c_str(),
          buf,
          length);
}

template <class T>
BlePeerCharacteristicTemplate<T>::~BlePeerCharacteristicTemplate() {}

template <class T>
T BlePeerCharacteristicTemplate<T>::decode(const uint8_t* data, const size_t length) {
    log_i("not implemented");
    return (T)NULL;
}

template <class T>
bool BlePeerCharacteristicTemplate<T>::encode(const T value, uint8_t* data, size_t length) {
    log_i("not implemented");
    return true;
}

template <class T>
T BlePeerCharacteristicTemplate<T>::read() {
    if (!characteristic) return lastValue;
    if (!characteristic->canRead()) {
        log_e("not readable");
        return lastValue;
    }
    lastValue = characteristic->readValue<T>();
    return lastValue;
}

template <class T>
bool BlePeerCharacteristicTemplate<T>::write(T value, size_t length) {
    if (!characteristic) return false;
    if (!characteristic->canWrite()) {
        log_e("not writable");
        return false;
    }
    return characteristic->writeValue<T>(value, length);
}

template <class T>
void BlePeerCharacteristicTemplate<T>::onNotify(
    BLERemoteCharacteristic* c,
    uint8_t* data,
    size_t length,
    bool isNotify) {
    T x = decode(data, length);
    log_i("BlePeerCharacteristicTemplate<T>::onNotify %d", x);
}

PowerPeerCharacteristic::~PowerPeerCharacteristic() {}