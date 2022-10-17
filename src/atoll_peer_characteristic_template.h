#ifndef __atoll_peer_characteristic_template_h
#define __atoll_peer_characteristic_template_h

#include "atoll_peer_characteristic.h"

#ifndef ATOLL_PEER_CHARACTERISTIC_READ_BUF_SIZE
#define ATOLL_PEER_CHARACTERISTIC_READ_BUF_SIZE 512
#endif

namespace Atoll {

// TODO get rid of the template, it's useless as we manually do the conversions in decode() and encode() anyway

template <class T>
class PeerCharacteristicTemplate : public PeerCharacteristic {
   public:
    T lastValue = T();
    char readBuffer[ATOLL_PEER_CHARACTERISTIC_READ_BUF_SIZE] = "";

    virtual T decode(const uint8_t* data, const size_t length) = 0;
    virtual bool encode(const T value, uint8_t* data, size_t length) = 0;

    virtual T read() {
        if (!characteristic) return lastValue;
        if (!characteristic->canRead()) {
            log_e("%s not readable", label);
            return lastValue;
        }
        // lastValue = characteristic->readValue<T>();
        snprintf(readBuffer, sizeof(readBuffer), "%s", characteristic->readValue().c_str());
        size_t len = strlen(readBuffer);
        log_i("readBuf len=%d", len);
        if (!len) return lastValue;
        lastValue = decode((uint8_t*)readBuffer, len);
        log_i("0x%x", lastValue);
        return lastValue;
    }

    virtual bool write(T value, size_t length) {
        if (!characteristic) return false;
        if (!characteristic->canWrite()) {
            log_e("%s not writable", label);
            return false;
        }
        return characteristic->writeValue<T>(value, length);
    }

    virtual void onNotify(
        BLERemoteCharacteristic* c,
        uint8_t* data,
        size_t length,
        bool isNotify) override {
        lastValue = decode(data, length);
        // log_i("%x", lastValue);
        notify();
    }

    virtual void notify() {
        // log_i("0x%x", lastValue);
    }

    virtual bool subscribe(BLEClient* client) override {
        bool res = PeerCharacteristic::subscribe(client);
        if (res && readOnSubscribe()) {
            read();
            notify();
        }
        return res;
    }
};

}  // namespace Atoll

#endif