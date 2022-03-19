#ifndef __atoll_ble_peer_characteristic_template_h
#define __atoll_ble_peer_characteristic_template_h

#include "atoll_ble_peer_characteristic.h"

namespace Atoll {

template <class T>
class BlePeerCharacteristicTemplate : public BlePeerCharacteristic {
   public:
    T lastValue = T();

    virtual T decode(const uint8_t* data, const size_t length) = 0;
    virtual bool encode(const T value, uint8_t* data, size_t length) = 0;

    virtual T read() {
        if (!characteristic) return lastValue;
        if (!characteristic->canRead()) {
            log_e("not readable");
            return lastValue;
        }
        lastValue = characteristic->readValue<T>();
        return lastValue;
    }

    virtual bool write(T value, size_t length) {
        if (!characteristic) return false;
        if (!characteristic->canWrite()) {
            log_e("not writable");
            return false;
        }
        return characteristic->writeValue<T>(value, length);
    }

    virtual void onNotify(
        BLERemoteCharacteristic* c,
        uint8_t* data,
        size_t length,
        bool isNotify) {
        lastValue = decode(data, length);
        log_i("BlePeerCharacteristicTemplate<T>::onNotify %d", lastValue);
    }
};

}  // namespace Atoll

#endif