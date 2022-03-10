#ifndef __ble_h
#define __ble_h

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <CircularBuffer.h>

#include "ble_constants.h"
#include "task.h"
#include "haspreferences.h"

#ifndef BLE_CHAR_VALUE_MAXLENGTH
#define BLE_CHAR_VALUE_MAXLENGTH 128
#endif

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

#ifndef HOSTNAME
#define HOSTNAME "libAtollBle"
#endif

#ifndef BLE_APPEARANCE
#define BLE_APPEARANCE APPEARANCE_CYCLING_POWER_SENSOR
#endif

class BLE : public Task,
            public HasPreferences,
            // public BLEClientCallbacks,
            public BLEServerCallbacks,
            public BLECharacteristicCallbacks {
   public:
    uint32_t taskStack = 4096;                        // task stack size in bytes
    char deviceName[SETTINGS_STR_LENGTH] = HOSTNAME;  // advertised device name
    bool enabled = true;                              // whether bluetooth is enabled
    BLEServer *server;                                // pointer to the ble server
    BLEUUID disUUID;                                  // device information service uuid
    BLEService *dis;                                  // device information service
    BLECharacteristic *diChar;                        // device information characteristic
    BLEUUID blsUUID;                                  // battery level service uuid
    BLEService *bls;                                  // battery level service
    BLECharacteristic *blChar;                        // battery level characteristic
    BLEUUID asUUID;                                   // api service uuid
    BLEService *as;                                   // api service
    BLECharacteristic *apiTxChar;                     // api tx characteristic
    BLECharacteristic *apiRxChar;                     // api xx characteristic
    BLEAdvertising *advertising;                      // pointer to advertising object

    uint8_t lastBatteryLevel = 0;
    unsigned long lastBatteryNotification = 0;
    bool secureApi = false;     // whether to use LESC for API service
    uint32_t passkey = 696669;  // passkey for API service, max 6 digits

    BLE() {}
    ~BLE() {}

    void setup(const char *deviceName, Preferences *p);
    void loop();
    void startServices();
    void startDiService();
    void startBlService();
    void startApiService();
    void notifyBl(const ulong t, const uint8_t level);
    void handleApiCommand(const char *command);
    void setApiValue(const char *value);
    const char *characteristicStr(BLECharacteristic *c);
    void stop();

    void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc);
    void onDisconnect(BLEServer *pServer);
    void startAdvertising();

    void onRead(BLECharacteristic *pCharacteristic);
    void onWrite(BLECharacteristic *pCharacteristic);
    void onNotify(BLECharacteristic *pCharacteristic);
    void onSubscribe(BLECharacteristic *pCharacteristic, ble_gap_conn_desc *desc, uint16_t subValue);

    void setSecureApi(bool state);
    void setPasskey(uint32_t newPasskey);
    void loadSettings();
    void saveSettings();
    void printSettings();

   private:
    CircularBuffer<uint16_t, 32> _clients;  // keeps track of connected client handles, in order to gracefully disconnect them before deep sleep or reboot; TODO add has() to CircularBuffer
};

#endif