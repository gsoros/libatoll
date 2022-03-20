#ifndef __atoll_ble_peer_device_h
#define __atoll_ble_peer_device_h

#include <Arduino.h>
#include <NimBLEDevice.h>
// #include "atoll_ble_constants.h"
// #include "atoll_ble_peer_characteristic.h"
#include "atoll_ble_peer_characteristic_power.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

#ifndef ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS
#define ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS 8
#endif

#ifndef ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH
#define ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH 18
#endif

#ifndef ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH
#define ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH 4
#endif

#ifndef ATOLL_BLE_PEER_DEVICE_NAME_LENGTH
#define ATOLL_BLE_PEER_DEVICE_NAME_LENGTH SETTINGS_STR_LENGTH
#endif

#define ATOLL_BLE_PEER_DEVICE_PACKED_LENGTH (ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH + ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH + ATOLL_BLE_PEER_DEVICE_NAME_LENGTH + 3)

namespace Atoll {

class BlePeerDevice : public BLEClientCallbacks {
   public:
    char address[ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH];
    char type[ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH];
    char name[ATOLL_BLE_PEER_DEVICE_NAME_LENGTH];
    bool connecting = false;
    bool connected = false;
    bool shouldConnect = true;
    bool markedForRemoval = false;
    static const uint8_t packedMaxLength = ATOLL_BLE_PEER_DEVICE_PACKED_LENGTH;  // for convenience

    virtual ~BlePeerDevice();

    BlePeerDevice(const char* address, const char* type, const char* name) {
        if (strlen(address) < 1) {
            log_e("empty address for %s %s", name, type);
            strncpy(this->address, "invalid", sizeof(this->address));
        } else
            strncpy(this->address, address, sizeof(this->address));
        strncpy(this->type, type, sizeof(this->type));
        strncpy(this->name, name, sizeof(this->name));
        for (int8_t i = 0; i < charsMax; i++) removeCharAt(i);  // initialize to nullptrs
    }

    // format: address,type,name
    virtual bool pack(char* packed, size_t len) {
        char buf[packedMaxLength];
        snprintf(buf, sizeof(buf), "%s,%s,%s", address, type, name);
        if (len < strlen(buf)) {
            log_e("buffer too small");
            return false;
        }
        strncpy(packed, buf, len);
        return true;
    }

    // format: address,type,name
    static bool unpack(
        const char* packed,
        char* address,
        size_t addressLen,
        char* type,
        size_t typeLen,
        char* name,
        size_t nameLen) {
        // log_i("unpacking '%s'", packed);
        size_t packedLen = strlen(packed);
        if (packedLen < sizeof(BlePeerDevice::address) + 4) {
            log_e("packed string too short (%d)", packedLen);
            return false;
        }
        char* firstColon = strchr(packed, ',');
        if (nullptr == firstColon) {
            log_e("first colon not present");
            return false;
        }
        char addressT[sizeof(BlePeerDevice::address)] = "";
        size_t addressTLen = firstColon - packed;
        // log_i("actual address length: %d", addressTLen);
        if (addressLen < addressTLen) {
            log_e("address buffer too small");
            return false;
        }
        strncpy(addressT, packed, addressTLen);
        char* lastColon = strrchr(packed, ',');
        if (nullptr == lastColon) {
            log_e("last colon not present");
            return false;
        }
        char typeT[sizeof(BlePeerDevice::type)] = "";
        size_t typeTLen = lastColon - addressTLen - 1 - packed;
        // log_i("actual type length: %d", typeTLen);
        if (typeLen < typeTLen) {
            log_e("type buffer too small");
            return false;
        }
        strncpy(typeT, packed + addressTLen + 1, typeTLen);
        char nameT[sizeof(BlePeerDevice::name)] = "";
        size_t nameTLen = packedLen - addressTLen - typeTLen - 2;
        // log_i("actual name length: %d", nameTLen);
        if (nameLen < nameTLen) {
            log_e("name buffer too small");
            return false;
        }
        strncpy(nameT, packed + addressTLen + typeTLen + 2, nameTLen);
        // log_i("addressT: %s, typeT: %s, nameT: %s", addressT, typeT, nameT);
        strncpy(address, addressT, addressLen);
        strncpy(type, typeT, typeLen);
        strncpy(name, nameT, nameLen);
        // log_i("address: %s, type: %s, name: %s", address, type, name);
        return true;
    }

    virtual void setClient(BLEClient* client) {
        this->client = client;
        if (nullptr == client) {
            // log_i("Warning: setting null client (use unsetClient()?)");
            return;
        }
        client->setClientCallbacks(this, false);
        client->setConnectionParams(12, 12, 0, 51);
        client->setConnectTimeout(5);
    }

    virtual BLEClient* getClient() {
        return client;
    }

    virtual void unsetClient() {
        client = nullptr;
    }

    virtual bool deleteClient() {
        if (!hasClient()) return true;
        disconnect();
        // TODO BLEDevice::deleteClient crashes
        log_i("calling BLEDevice::deleteClient()");
        if (!BLEDevice::deleteClient(client)) {
            log_i("could not delete client for %s", name);
            return false;
        }
        log_i("client deleted for %s", name);
        unsetClient();
        return true;
    }

    virtual bool hasClient() {
        return client != nullptr;
    }

    virtual bool isConnected() {
        return hasClient() && connected;
    }

    virtual void connect();

    virtual void disconnect() {
        shouldConnect = false;
        if (!hasClient()) {
            log_e("hasClient() is false");
            return;
        }
        unsubscribeChars();
        if (isConnected())
            client->disconnect();
        while (isConnected()) {
            log_i("waiting for disconnect...");
            delay(1000);
        }
        // deleteClient();
    }

    virtual void subscribeChars();
    virtual void unsubscribeChars();
    virtual int8_t charIndex(const char* label);
    virtual bool charExists(const char* label);
    virtual bool addChar(BlePeerCharacteristic* c);
    virtual BlePeerCharacteristic* getChar(const char* label);
    virtual bool removeCharAt(int8_t index);
    virtual uint8_t deleteChars(const char* label);

   protected:
    BLEClient* client = nullptr;                                              // our NimBLE client
    BlePeerCharacteristic* chars[ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS];  // characteristics
    int8_t charsMax = ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS;              // convenience for iterating

    virtual bool
    connectClient(bool deleteAttibutes = true) {
        return client->connect((BLEAddress)address, deleteAttibutes);
    }

    // client callbacks
    virtual void onConnect(BLEClient* pClient);
    virtual void onDisconnect(BLEClient* pClient);
    virtual bool onConnParamsUpdateRequest(BLEClient* pClient, const ble_gap_upd_params* params);
    virtual uint32_t onPassKeyRequest();
    // virtual void onPassKeyNotify(uint32_t pass_key);
    // virtual bool onSecurityRequest();
    virtual void onAuthenticationComplete(ble_gap_conn_desc* desc);
    virtual bool onConfirmPIN(uint32_t pin);

    // notification callback
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify);
};

class PowerMeter : public BlePeerDevice {
   public:
    PowerMeter(
        const char* address,
        const char* type,
        const char* name) : BlePeerDevice(address,
                                          type,
                                          name) {
        addChar(new BlePeerCharacteristicPower());
    }
};

class HeartrateMonitor : public BlePeerDevice {};

}  // namespace Atoll

#endif