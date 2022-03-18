#ifndef __atoll_ble_peer_device_h
#define __atoll_ble_peer_device_h

#include <Arduino.h>
#include <NimBLEDevice.h>
#include "atoll_ble_constants.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

namespace Atoll {

class BlePeerDevice :  // public BLECharacteristicCallbacks,
                       public BLEClientCallbacks {
   public:
    char address[18];
    char type[4];
    char name[SETTINGS_STR_LENGTH];
    bool connecting = false;

    BlePeerDevice() {}
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

    virtual bool connectClient(bool deleteAttibutes = true) {
        return client->connect((BLEAddress)address, deleteAttibutes);
    }

    virtual void connect();

    virtual void disconnect() {
        if (!hasClient()) return;
        if (isConnected())
            client->disconnect();
        deleteClient();
    }

    /*
    // format: "address,type,name"
    BlePeerDevice(const char* str) {
        sscanf(str, "%s,%s,%s", this->address, this->type, this->name);  // TODO do this properly
        char packed[60];
        pack(&packed, 60);
        if (0 != strcmp(packed, str))
             log_e("Warning: could not parse %s (got %s)", str, packed);
        log_i("'%s' ===> '%s', '%s', '%s'", str, this->address, this->type, this->name);
    }
    */
    /*
        void pack(char *packed, size_t len) {
            if (len < sizeof(this->address) + sizeof(this->type) + sizeof(this->name) + 3);
              log_e("buffer too small");
            snprintf(packed, len, "%s,%s,%s", this->address, this->type, this->name);
        }
    */

   protected:
    BLEClient* client = nullptr;

    /* client callbacks */
    virtual void onConnect(BLEClient* pClient);
    virtual void onDisconnect(BLEClient* pClient);
    virtual bool onConnParamsUpdateRequest(BLEClient* pClient, const ble_gap_upd_params* params);
    virtual uint32_t onPassKeyRequest();
    // virtual void onPassKeyNotify(uint32_t pass_key);
    // virtual bool onSecurityRequest();
    virtual void onAuthenticationComplete(ble_gap_conn_desc* desc);
    virtual bool onConfirmPIN(uint32_t pin);
};

class PowerMeter : public BlePeerDevice {};

}  // namespace Atoll

#endif