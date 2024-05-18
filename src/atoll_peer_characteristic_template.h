#if !defined(__atoll_peer_characteristic_template_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_characteristic_template_h

#include "atoll_peer_characteristic.h"

#ifndef ATOLL_PEER_CHARACTERISTIC_READ_BUF_SIZE
#define ATOLL_PEER_CHARACTERISTIC_READ_BUF_SIZE 512
#endif

namespace Atoll {

template <class T>
class PeerCharacteristicTemplate : public PeerCharacteristic {
   public:
    T lastValue = T();
    char readBuffer[ATOLL_PEER_CHARACTERISTIC_READ_BUF_SIZE] = "";

    virtual T decode(const uint8_t* data, const size_t length) = 0;
    virtual bool encode(const T value, uint8_t* data, size_t length) = 0;

    virtual T read(BLEClient* client) {
        BLERemoteCharacteristic* rc = getRemoteChar(client);
        if (!rc) {
            log_e("%s could not get char", label);
            return lastValue;
        }
        if (!rc->canRead()) {
            log_e("%s not readable", label);
            return lastValue;
        }
        remoteOpStart(client);
        // lastValue = rc->readValue<T>();
        // log_d("%s reading into buffer", label);
        NimBLEAttValue value = rc->readValue();
        remoteOpEnd(client);
        snprintf(readBuffer, sizeof(readBuffer), "%s", value.c_str());
        size_t len = strlen(readBuffer);
        // log_d("%s readBuf len=%d", label, len);
        if (!len) return lastValue;
        lastValue = decode((uint8_t*)readBuffer, len);
        // log_d("%s 0x%x", label, lastValue);
        return lastValue;
    }

    virtual bool write(T value, size_t length) {
        BLEClient* client = getClient();
        if (nullptr == client) {
            log_e("client is null");
            return false;
        }
        return write(client, value, length);
    }

    virtual bool write(BLEClient* client, T value, size_t length) {
        BLERemoteCharacteristic* rc = getRemoteChar(client);
        if (!rc) return false;
        if (!rc->canWrite()) {
            log_e("%s not writable", label);
            return false;
        }
        remoteOpStart(client);
        bool res = rc->writeValue<T>(value, length);
        remoteOpEnd(client);
        return res;
    }

    virtual void onNotify(
        BLERemoteCharacteristic* c,
        uint8_t* data,
        size_t length,
        bool isNotify) override {
        lastValue = decode(data, length);
        // log_d("%s 0x%x", label, lastValue);
        //log_d("onNotify %s", label);
        notify();
    }

    virtual bool subscribe(BLEClient* client) override {
        bool res = PeerCharacteristic::subscribe(client);
        if (res) {
            onSubscribe(client);
        }
        return res;
    }

    virtual void onSubscribe(BLEClient* client) {
        if (readOnSubscribe()) {
            log_d("%s readOnSubscribe, calling read", label);
            read(client);
            notify();
        }
    }
};

}  // namespace Atoll

#endif