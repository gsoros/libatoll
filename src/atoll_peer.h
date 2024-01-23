#if !defined(__atoll_peer_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_peer_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_log.h"

#include "atoll_peer_characteristic_battery.h"
#include "atoll_peer_characteristic_power.h"
#include "atoll_peer_characteristic_heartrate.h"
#include "atoll_peer_characteristic_weightscale.h"
#include "atoll_peer_characteristic_temperature.h"
#include "atoll_peer_characteristic_api_rx.h"
#include "atoll_peer_characteristic_api_tx.h"
#include "atoll_peer_characteristic_vesc_rx.h"
#include "atoll_peer_characteristic_vesc_tx.h"
#include "atoll_peer_characteristic_jkbms.h"

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

    bool enabled = true;  // TODO is there a good reason for having both enabled and shouldConnect?
    bool shouldConnect = true;
    bool connecting = false;
    ulong lastConnectionAttempt = 0;
    bool markedForRemoval = false;
    uint8_t connParamsProfile = APCPP_INITIAL;
    static const uint8_t packedMaxLength = ATOLL_BLE_PEER_DEVICE_PACKED_LENGTH;  // for convenience

    enum ConnectionParamsProfile {
        APCPP_INITIAL,     // initial connection phase
        APCPP_ESTABLISHED  // established phase
    };

    virtual ~Peer();

    Peer(Saved saved, PeerCharacteristicBattery* customBattChar = nullptr);

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
    virtual bool isJkBms();

    typedef std::function<void(Peer*)> Callback;
    Callback connectedCallback = [](Peer*) {};

   protected:
    BLEClient* client = nullptr;                                           // our NimBLE client
    PeerCharacteristic* chars[ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS];  // characteristics
    int8_t charsMax = ATOLL_BLE_PEER_DEVICE_MAX_CHARACTERISTICS;           // convenience for iterating

    virtual bool connectClient(bool deleteAttributes = true);
    virtual void beforeConnect();
    virtual void afterConnect();

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
        PeerCharacteristicWeightscale* customWeightChar = nullptr,
        PeerCharacteristicTemperature* customTemperatureChar = nullptr);

    virtual void loop() override;

    uint16_t savedMtu = 0;
    virtual void beforeConnect() override;
    virtual void afterConnect() override;

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

    virtual bool requestUpdate();
    virtual float getVoltage();
    virtual uint16_t getPower();
    virtual void setPower(uint16_t power);
};

class JkBms : public Peer {
   public:
    JkBms(
        Saved saved,
        PeerCharacteristicJkBms* customJkBmsChar = nullptr) : Peer(saved) {
        deleteChars("Battery");
        addChar(nullptr != customJkBmsChar
                    ? customJkBmsChar
                    : new PeerCharacteristicJkBms());
        characteristic = (PeerCharacteristicJkBms*)getChar("JkBms");
        if (nullptr == characteristic) {
            log_e("char is null");
            return;
        }
        characteristic->onDeviceInfoUpdate = [this](PeerCharacteristicJkBms* c) {
            if (nullptr != c) strncpy(name, c->deviceInfo.deviceName, sizeof(name));
        };
        deviceInfo = &characteristic->deviceInfo;
        settings = &characteristic->settings;
        cellInfo = &characteristic->cellInfo;
    }

    void loop() override {
        // don't set established conn params
    }

    PeerCharacteristicJkBms* characteristic = nullptr;
    PeerCharacteristicJkBms::DeviceInfo* deviceInfo = nullptr;
    PeerCharacteristicJkBms::Settings* settings = nullptr;
    PeerCharacteristicJkBms::CellInfo* cellInfo = nullptr;
    char name[16] = "";

    void printStatus() {
        if (nullptr == characteristic) {
            log_e("char is null");
            return;
        }
        characteristic->printCellInfo();
    }

    virtual void onDisconnect(BLEClient* pClient, int reason) override {
        Peer::onDisconnect(pClient, reason);
        characteristic->onDisconnect();
        if (nullptr != disconnectCustomCallback) disconnectCustomCallback(this);
    }

    std::function<void(JkBms*)> disconnectCustomCallback = nullptr;

   protected:
};

}  // namespace Atoll

#endif