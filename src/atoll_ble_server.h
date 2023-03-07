#if !defined(__atoll_ble_server_h) && defined(FEATURE_BLE_SERVER)
#define __atoll_ble_server_h

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <CircularBuffer.h>

#include "atoll_ble.h"
#include "atoll_ble_constants.h"
#include "atoll_task.h"
#include "atoll_ble_characteristic_callbacks.h"

#ifndef ATOLL_BLE_SERVER_CHAR_VALUE_MAXLENGTH
#ifdef BLE_CHAR_VALUE_MAXLENGTH
#define ATOLL_BLE_SERVER_CHAR_VALUE_MAXLENGTH BLE_CHAR_VALUE_MAXLENGTH
#else
#define ATOLL_BLE_SERVER_CHAR_VALUE_MAXLENGTH 512
#endif
#endif

#ifndef ATOLL_BLE_SERVER_MAX_SERVICES
#define ATOLL_BLE_SERVER_MAX_SERVICES 8
#endif

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

#ifndef HOSTNAME
#define HOSTNAME "libAtollBle_unnamed"
#endif

namespace Atoll {

class BleServer : public Task,
                  public BLEServerCallbacks,
                  public BleCharacteristicCallbacks {
   public:
    const char *taskName() { return "BleServer"; }
    uint32_t taskStack = 4096;                        // task stack size in bytes
    char deviceName[SETTINGS_STR_LENGTH] = HOSTNAME;  // advertised device name
    bool enabled = true;                              // whether ble server is enabled
    bool started = false;                             // whether the NimBLE server has been started

    BleServer();
    virtual ~BleServer();

    virtual void setup(const char *deviceName);
    virtual void init();
    virtual uint16_t getAppearance();
    virtual void setupAdvertising();
    virtual bool createDeviceInformationService();
    virtual void loop();

    virtual void setSecurity(bool state, const uint32_t passkey = 0);
    virtual BLEService *createService(const BLEUUID &uuid);
    virtual void removeService(BLEService *s);
    virtual void advertiseService(const BLEUUID &uuid, uint8_t advType = 0);
    virtual void unAdvertiseService(const BLEUUID &uuid, uint8_t advType = 0);
    virtual BLEService *getService(const BLEUUID &uuid);
    virtual BLECharacteristic *getChar(const BLEUUID &serviceUuid, const BLEUUID &charUuid);

    virtual void start();
    virtual void startAdvertising();
    virtual void stopAdvertising();

    virtual void notify(const BLEUUID &serviceUuid,
                        const BLEUUID &charUuid,
                        uint8_t *data,
                        size_t size);

    virtual void stop();

    // BLEServerCallbacks
    virtual void onConnect(BLEServer *pServer, BLEConnInfo &connInfo) override;
    virtual void onDisconnect(BLEServer *pServer, BLEConnInfo &connInfo, int reason) override;
    virtual void onMTUChange(uint16_t MTU, BLEConnInfo &connInfo) override;
    virtual uint32_t onPassKeyRequest() override;
    virtual void onAuthenticationComplete(BLEConnInfo &connInfo) override;
    virtual bool onConfirmPIN(uint32_t pin) override;

   protected:
    // keeps track of connected client handles, in order to gracefully disconnect them before deep sleep or reboot; TODO add has() to CircularBuffer
    CircularBuffer<uint16_t, 8> _clients;   //
    BLEServer *server = nullptr;            // pointer to the ble server
    BLEAdvertising *advertising = nullptr;  // pointer to advertising object
    ulong advertisingTimeoutMs = 0;         // stop advertising after this time after boot, 0 = never,  TODO get from settings
};

}  // namespace Atoll
#endif