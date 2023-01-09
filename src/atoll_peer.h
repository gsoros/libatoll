#ifndef __atoll_peer_h
#define __atoll_peer_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_log.h"

#include "atoll_peer_characteristic_battery.h"
#include "atoll_peer_characteristic_power.h"
#include "atoll_peer_characteristic_heartrate.h"
#include "atoll_peer_characteristic_weightscale.h"
#include "atoll_peer_characteristic_api_rx.h"
#include "atoll_peer_characteristic_api_tx.h"
#include "atoll_peer_characteristic_vesc_rx.h"
#include "atoll_peer_characteristic_vesc_tx.h"

#include "VescUart.h"

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

// ="address,addressType,type,name,passkey"
#define ATOLL_BLE_PEER_DEVICE_PACKED_LENGTH (ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH + ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH + ATOLL_BLE_PEER_DEVICE_NAME_LENGTH + 19)

namespace Atoll {

class Peer : public BLEClientCallbacks {
   public:
    struct Saved {
        char address[ATOLL_BLE_PEER_DEVICE_ADDRESS_LENGTH] = "";
        uint8_t addressType = 0;
        char type[ATOLL_BLE_PEER_DEVICE_TYPE_LENGTH] = "";
        char name[ATOLL_BLE_PEER_DEVICE_NAME_LENGTH] = "";
        uint32_t passkey = 0;
    } saved;

    bool connecting = false;
    bool shouldConnect = true;
    bool markedForRemoval = false;
    uint8_t connParamsProfile = APCPP_INITIAL;
    static const uint8_t packedMaxLength = ATOLL_BLE_PEER_DEVICE_PACKED_LENGTH;  // for convenience

    enum ConnectionParamsProfile {
        APCPP_INITIAL,     // initial connection phase
        APCPP_ESTABLISHED  // established phase
    };

    virtual ~Peer();

    Peer(Saved saved,
         PeerCharacteristicBattery* customBattChar = nullptr);

    virtual void loop();

    // format: address,addressType,type,name[,passkey]
    virtual bool pack(char* packed, size_t len, bool includePasskey = true);

    // format: address,addressType,type,name,passkey
    static bool unpack(
        const char* packed,
        Saved* saved);

    virtual void setConnectionParams(uint8_t profile);
    virtual void connect();

    virtual void disconnect();

    virtual bool setClient(BLEClient* client);
    virtual BLEClient* getClient();
    virtual void unsetClient();
    virtual bool hasClient();
    virtual bool isConnected();

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
    virtual bool isVesc();

   protected:
    BLEClient* client = nullptr;                                           // our NimBLE client
    PeerCharacteristic* chars[ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS];  // characteristics
    int8_t charsMax = ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS;           // convenience for iterating

    virtual bool connectClient(bool deleteAttributes = true);

    // client callbacks
    virtual void onConnect(BLEClient* pClient) override;
    virtual void onDisconnect(BLEClient* pClient, int reason) override;
    virtual bool onConnParamsUpdateRequest(BLEClient* pClient, const ble_gap_upd_params* params) override;
    virtual uint32_t onPassKeyRequest() override;
    // virtual void onPassKeyNotify(uint32_t pass_key) override;
    // virtual bool onSecurityRequest() override;
    virtual void onAuthenticationComplete(NimBLEConnInfo& connInfo) override;
    virtual bool onConfirmPIN(uint32_t pin) override;

    // notification callback
    virtual void onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify);
};

class PowerMeter : public Peer {
   public:
    PowerMeter(
        Saved saved,
        PeerCharacteristicPower* customPowerChar = nullptr,
        PeerCharacteristicBattery* customBattChar = nullptr);
};

class ESPM : public PowerMeter {
   public:
    ESPM(
        Saved saved,
        PeerCharacteristicPower* customPowerChar = nullptr,
        PeerCharacteristicBattery* customBattChar = nullptr,
        PeerCharacteristicApiTX* customApiTxChar = nullptr,
        PeerCharacteristicApiRX* customApiRxChar = nullptr,
        PeerCharacteristicWeightscale* customWeightChar = nullptr);

    virtual void loop() override;

    virtual void onConnect(BLEClient* pClient) override;

    virtual bool sendApiCommand(const char* command);
};

class HeartrateMonitor : public Peer {
   public:
    HeartrateMonitor(
        Saved saved,
        PeerCharacteristicHeartrate* customHrChar = nullptr,
        PeerCharacteristicBattery* customBattChar = nullptr);
};

class PeerCharacteristicVescTX;
class VescUartBleStream;
class Vesc : public Peer {
   public:
    Vesc(Saved saved,
         PeerCharacteristicVescRX* customVescRX = nullptr,
         PeerCharacteristicVescTX* customVescTX = nullptr);

    virtual void setConnectionParams(uint8_t profile) override;
    virtual void loop() override;

    VescUartBleStream* uartBleStream;
    VescUart* uart;

    bool requestUpdate();
    float getVoltage();
    uint16_t getPower();
    void setPower(uint16_t power);
};

}  // namespace Atoll

#endif