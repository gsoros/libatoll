#ifndef __atoll_peer_h
#define __atoll_peer_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_log.h"

#include "atoll_peer_characteristic_battery.h"
#include "atoll_peer_characteristic_power.h"
#include "atoll_peer_characteristic_heartrate.h"
#include "atoll_peer_characteristic_weightscale.h"
#include "atoll_peer_characteristic_apitx.h"
#include "atoll_peer_characteristic_apirx.h"

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

// ="address,addressType,type,name"
#define ATOLL_BLE_PEER_DEVICE_PACKED_LENGTH (ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH + ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH + ATOLL_BLE_PEER_DEVICE_NAME_LENGTH + 6)

namespace Atoll {

class Peer : public BLEClientCallbacks {
   public:
    char address[ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH];
    uint8_t addressType;
    char type[ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH];
    char name[ATOLL_BLE_PEER_DEVICE_NAME_LENGTH];
    bool connecting = false;
    bool shouldConnect = true;
    bool markedForRemoval = false;
    static const uint8_t packedMaxLength = ATOLL_BLE_PEER_DEVICE_PACKED_LENGTH;  // for convenience

    virtual ~Peer();

    Peer(const char* address,
         uint8_t addressType,
         const char* type,
         const char* name,
         PeerCharacteristicBattery* customBattChar = nullptr) {
        if (strlen(address) < 1) {
            log_e("empty address for %s %s", name, type);
            snprintf(this->address, sizeof(this->address), "%s", "invalid");
        } else
            strncpy(this->address, address, sizeof(this->address));
        this->addressType = addressType;
        strncpy(this->type, type, sizeof(this->type));
        strncpy(this->name, name, sizeof(this->name));
        for (int8_t i = 0; i < charsMax; i++)
            removeCharAt(i);  // initialize to nullptrs
        addChar(nullptr != customBattChar
                    ? customBattChar
                    : new PeerCharacteristicBattery());
    }

    // format: address,addressType,type,name
    virtual bool pack(char* packed, size_t len) {
        char buf[packedMaxLength];
        snprintf(buf, sizeof(buf), "%s,%d,%s,%s", address, addressType, type, name);
        if (len < strlen(buf)) {
            log_e("buffer too small");
            return false;
        }
        strncpy(packed, buf, len);
        return true;
    }

    // format: address,addressType,type,name
    static bool unpack(
        const char* packed,
        char* address,
        size_t addressLen,
        uint8_t* addressType,
        char* type,
        size_t typeLen,
        char* name,
        size_t nameLen) {
        // log_i("unpacking '%s'", packed);
        size_t packedLen = strlen(packed);
        // log_i("packedLen: %d", packedLen);
        if (packedLen < sizeof(Peer::address) + 7) {
            log_e("packed string too short (%d)", packedLen);
            return false;
        }
        char* colon = strchr(packed, ',');
        if (nullptr == colon) {
            log_e("first colon not present");
            return false;
        }
        char tAddress[sizeof(Peer::address)] = "";
        size_t tAddressLen = colon - packed;
        // log_i("tAddressLen: %d", tAddressLen);
        if (addressLen < tAddressLen) {
            log_e("address buffer too small");
            return false;
        }
        strncpy(tAddress, packed, tAddressLen);

        // rest of packed, without address and colon
        char rest[packedLen - tAddressLen] = "";
        strncpy(rest, colon + 1, sizeof(rest) - 1);
        // log_i("rest: '%s' size: %d", rest, sizeof(rest) - 1);
        colon = strchr(rest, ',');
        if (nullptr == colon) {
            log_e("second colon not present");
            return false;
        }
        char tAddressTypeStr[3] = "";  // strlen(uint8_t)
        strncpy(tAddressTypeStr, rest, colon - rest);
        uint8_t tAddressTypeLen = strlen(tAddressTypeStr);
        uint8_t tAddressType = atoi(tAddressTypeStr);
        // log_i("tAddressType: %d", tAddressType);

        colon = strrchr(packed, ',');
        if (nullptr == colon) {
            log_e("last colon not present");
            return false;
        }
        char tType[sizeof(Peer::type)] = "";
        size_t tTypeLen = colon - tAddressLen - tAddressTypeLen - 2 - packed;
        // log_i("tTypeLen: %d", tTypeLen);
        if (typeLen < tTypeLen) {
            log_e("type buffer too small");
            return false;
        }
        strncpy(tType, packed + tAddressTypeLen + tAddressLen + 2, tTypeLen);
        char tName[sizeof(Peer::name)] = "";
        size_t tNameLen = packedLen - tAddressLen - tAddressTypeLen - tTypeLen - 3;
        // log_i("tNameLen: %d", tNameLen);
        if (nameLen < tNameLen) {
            log_e("name buffer too small");
            return false;
        }
        strncpy(tName, packed + tAddressLen + tAddressTypeLen + tTypeLen + 3, tNameLen);
        // log_i("tAddress: %s, tAddressType: %d, tType: %s, tName: %s",
        //       tAddress, tAddressType, tType, tName);
        strncpy(address, tAddress, addressLen);
        *addressType = tAddressType;
        strncpy(type, tType, typeLen);
        strncpy(name, tName, nameLen);
        // log_i("address: %s, addressType: %d, type: %s, name: %s",
        //       address, *addressType, type, name);
        return true;
    }

    virtual void setClient(BLEClient* client) {
        this->client = client;
        if (nullptr == client) {
            // log_e("Setting NULL client (use unsetClient()?)");
            return;
        }
        client->setClientCallbacks(this, false);
    }

    virtual BLEClient* getClient() {
        return client;
    }

    virtual void unsetClient() {
        client = nullptr;
    }

    virtual bool hasClient() {
        return client != nullptr;
    }

    virtual bool isConnected() {
        return hasClient() && getClient()->isConnected();
    }

    virtual void connect();

    virtual void disconnect() {
        if (!hasClient()) {
            log_e("%s no client", name);
            return;
        }
        shouldConnect = false;
        if (isConnected()) {
            unsubscribeChars(client);
            client->disconnect();
        }
        while (isConnected()) {
            log_i("%s waiting for disconnect...", name);
            delay(500);
        }
    }

    virtual void subscribeChars(BLEClient* client);
    virtual void unsubscribeChars(BLEClient* client);
    virtual int8_t charIndex(const char* label);
    virtual bool charExists(const char* label);
    virtual bool addChar(PeerCharacteristic* c);
    virtual PeerCharacteristic* getChar(const char* label);
    virtual bool removeCharAt(int8_t index);
    virtual uint8_t deleteChars(const char* label);

    virtual bool isPowerMeter();
    virtual bool isESPM();
    virtual bool isHeartrateMonitor();

   protected:
    BLEClient* client = nullptr;                                           // our NimBLE client
    PeerCharacteristic* chars[ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS];  // characteristics
    int8_t charsMax = ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS;           // convenience for iterating

    virtual bool connectClient(bool deleteAttibutes = true) {
        // log_i("connecting to %s(%d)", address, addressType);
        return client->connect(BLEAddress(address, addressType), deleteAttibutes);
    }

    // client callbacks
    virtual void onConnect(BLEClient* pClient) override;
    virtual void onDisconnect(BLEClient* pClient) override;
    virtual bool onConnParamsUpdateRequest(BLEClient* pClient, const ble_gap_upd_params* params) override;
    virtual uint32_t onPassKeyRequest() override;
    // virtual void onPassKeyNotify(uint32_t pass_key) override;
    // virtual bool onSecurityRequest() override;
    virtual void onAuthenticationComplete(ble_gap_conn_desc* desc) override;
    virtual bool onConfirmPIN(uint32_t pin) override;

    // notification callback
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify);
};

class PowerMeter : public Peer {
   public:
    PowerMeter(
        const char* address,
        uint8_t addressType,
        const char* type,
        const char* name,
        PeerCharacteristicPower* customPowerChar = nullptr,
        PeerCharacteristicBattery* customBattChar = nullptr)
        : Peer(
              address,
              addressType,
              type,
              name,
              customBattChar) {
        addChar(nullptr != customPowerChar
                    ? customPowerChar
                    : new PeerCharacteristicPower());
    }
};

class ESPM : public PowerMeter {
   public:
    ESPM(
        const char* address,
        uint8_t addressType,
        const char* type,
        const char* name,
        PeerCharacteristicPower* customPowerChar = nullptr,
        PeerCharacteristicBattery* customBattChar = nullptr,
        PeerCharacteristicApiTX* customApiTxChar = nullptr,
        PeerCharacteristicApiRX* customApiRxChar = nullptr,
        PeerCharacteristicWeightscale* customWeightChar = nullptr);

    virtual void onConnect(BLEClient* pClient) override;

    virtual bool sendApiCommand(const char* command);
};

class HeartrateMonitor : public Peer {
   public:
    HeartrateMonitor(
        const char* address,
        uint8_t addressType,
        const char* type,
        const char* name,
        PeerCharacteristicHeartrate* customHrChar = nullptr,
        PeerCharacteristicBattery* customBattChar = nullptr)
        : Peer(
              address,
              addressType,
              type,
              name,
              customBattChar) {
        addChar(nullptr != customHrChar
                    ? customHrChar
                    : new PeerCharacteristicHeartrate());
    }
};

}  // namespace Atoll

#endif