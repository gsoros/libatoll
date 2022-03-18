#ifndef __atoll_ble_peer_device_h
#define __atoll_ble_peer_device_h

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "atoll_ble_constants.h"
#include "atoll_ble_peer_characteristic.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

#ifndef ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS
#define ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS 8
#endif

namespace Atoll {

class BlePeerDevice : public BLEClientCallbacks {
   public:
    char address[18];
    char type[4];
    char name[SETTINGS_STR_LENGTH];
    bool connecting = false;

    virtual ~BlePeerDevice() {
        log_i("checking to delete client");
        deleteClient();
    }

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
        if (BLEDevice::deleteClient(client)) {
            log_i("deleted client");
            unsetClient();
            return true;
        }
        return false;
    }

    virtual bool hasClient() {
        return client != nullptr;
    }

    virtual bool isConnected() {
        return hasClient() && client->isConnected();
    }

    virtual void connect();

    virtual void disconnect() {
        if (!hasClient()) return;
        if (isConnected())
            client->disconnect();
        deleteClient();
    }

    virtual void subscribeChars();
    virtual int8_t charIndex(const char* name);
    virtual bool charExists(const char* name);
    virtual bool addChar(BlePeerCharacteristic* c);
    virtual BlePeerCharacteristic* getChar(const char* name);
    virtual bool removeCharAt(int8_t index);

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
    PowerMeter(const char* address,
               const char* type,
               const char* name) : BlePeerDevice(address,
                                                 type,
                                                 name) {
        log_i("PowerMeter construct, adding char");
        PowerPeerCharacteristic* p = new PowerPeerCharacteristic();
        addChar(p);
        log_i("PowerMeter constructed");
    }
    virtual ~PowerMeter() {}
};

class HeartrateMonitor : public BlePeerDevice {};

}  // namespace Atoll

#endif