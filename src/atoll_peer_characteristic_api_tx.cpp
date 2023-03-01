#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic_api_tx.h"

using namespace Atoll;

PeerCharacteristicApiTX::PeerCharacteristicApiTX(const char* label,
                                                 BLEUUID serviceUuid,
                                                 BLEUUID charUuid) {
    snprintf(this->label, sizeof(this->label), "%s", label);
    this->serviceUuid = serviceUuid;
    this->charUuid = charUuid;
}

String PeerCharacteristicApiTX::decode(const uint8_t* data, const size_t length) {
    char value[length + 1];
    strncpy(value, (char*)data, length);
    value[length] = 0;
    lastValue = String(value);
    // log_d("%s length=%d '%s'", label, length, lastValue.c_str());
    return lastValue;
}

bool PeerCharacteristicApiTX::encode(const String value, uint8_t* data, size_t length) {
    log_e("not implemented");
    return true;
}

void PeerCharacteristicApiTX::onNotify(BLERemoteCharacteristic* rc, uint8_t* data, size_t length, bool isNotify) {
    {
        // log_d("%s length: %d", label, length);
        BLERemoteService* rs = rc->getRemoteService();
        if (nullptr == rs) {
            log_e("%s service is null", label);
            goto decode;
        }
        BLEClient* client = rs->getClient();
        if (nullptr == client) {
            log_e("%s client is null", label);
            goto decode;
        }
        if (!client->isConnected()) {
            log_e("%s client not connected", label);
            goto decode;
        }
        uint16_t mtu = client->getMTU();
        // log_d("%s mtu is %d", label, mtu);
        if (mtu < 3) {
            log_e("%s mtu < 3", label);
            goto decode;
        }
        if (length < mtu - 3) {
            // log_d("%s length is %d, not reading", label, length);
            goto decode;
        }
        // log_e("%s cannot read full value from inside a callback, workaround: increase mtu (received %d, mtu is %d)", label, length, mtu);
        /*
        // goto decode;
        log_d("%s reading full value (received %d, mtu is %d)", label, length, mtu);
        // read(client);
        NimBLEAttValue av = rc->readValue();
        log_d("%s readValue() done", label, length, mtu);
        lastValue = av.getValue<String>();
        log_d("%s full value(%d): %s", label, lastValue.length(), lastValue.c_str());
        notify();
        */
        // log_d("%s marking dirty (received %d, mtu is %d)", label, length, mtu);
        lastValueDirty = true;
        return;
    }
decode:
    lastValue = decode(data, length);
    notify();
}

void PeerCharacteristicApiTX::notify() {
    log_d("%s received '%s'", label, lastValue.c_str());
}

bool PeerCharacteristicApiTX::readOnSubscribe() {
    return false;
}

void PeerCharacteristicApiTX::loop() {
    if (lastValueDirty) {
        // log_d("%s is dirty", label);
        BLEClient* client = getClient();
        if (nullptr == client) {
            log_e("%s no client", label);
            return;
        }
        if (!client->isConnected()) {
            log_e("%s not connected", label);
            return;
        }
        // log_d("%s reading...", label);
        read(client);
        // log_d("%s lastValue(%d): '%s'", label, lastValue.length(), lastValue.c_str());
        lastValueDirty = false;
        notify();
    }
}

#endif