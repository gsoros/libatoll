#ifndef __atoll_ble_server_h
#define __atoll_ble_server_h

#include <Arduino.h>
#include <NimBLEDevice.h>
#include <CircularBuffer.h>

#include "atoll_ble_constants.h"
#include "atoll_task.h"

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
                  public BLECharacteristicCallbacks {
   public:
    const char *taskName() { return "BleServer"; }
    uint32_t taskStack = 4096;                        // task stack size in bytes
    char deviceName[SETTINGS_STR_LENGTH] = HOSTNAME;  // advertised device name
    bool enabled = true;                              // whether bluetooth is enabled

    BleServer();
    virtual ~BleServer();

    virtual void setup(const char *deviceName);
    bool createDeviceInformationService();
    virtual void loop();

    virtual BLEService *createService(const BLEUUID &uuid);
    virtual void advertiseService(const BLEUUID &uuid, uint8_t advType = 0);
    virtual BLEService *getService(const BLEUUID &uuid);
    virtual BLECharacteristic *getChar(const BLEUUID &serviceUuid, const BLEUUID &charUuid);

    void startServices();
    void notify(const BLEUUID &serviceUuid,
                const BLEUUID &charUuid,
                uint8_t *data,
                size_t size);
    void stop();

    void onConnect(BLEServer *pServer, ble_gap_conn_desc *desc);
    void onDisconnect(BLEServer *pServer);

    void startAdvertising();

    void start();

    void onRead(BLECharacteristic *c) {
        log_i("%s: value: %s", c->getUUID().toString().c_str(), c->getValue().c_str());
    }

    void onWrite(BLECharacteristic *c) {
        log_i("%s: value: %s", c->getUUID().toString().c_str(), c->getValue().c_str());
    }

    void onNotify(BLECharacteristic *c) {
        log_i("%d", c->getValue<int>());
    }

    void onSubscribe(BLECharacteristic *c, ble_gap_conn_desc *desc, uint16_t subValue) {
        log_i("client ID: %d Address: %s ...",
              desc->conn_handle,
              BLEAddress(desc->peer_ota_addr).toString().c_str());
        if (subValue == 0)
            log_i("... unsubscribed from %s", c->getUUID().toString().c_str());
        else if (subValue == 1)
            log_i("... subscribed to notfications for %s", c->getUUID().toString().c_str());
        else if (subValue == 2)
            log_i("... Subscribed to indications for %s", c->getUUID().toString().c_str());
        else if (subValue == 3)
            log_i("... subscribed to notifications and indications for %s", c->getUUID().toString().c_str());
    }

   private:
    // keeps track of connected client handles, in order to gracefully disconnect them before deep sleep or reboot; TODO add has() to CircularBuffer
    CircularBuffer<uint16_t, 8> _clients;   //
    BLEServer *server = nullptr;            // pointer to the ble server
    BLEAdvertising *advertising = nullptr;  // pointer to advertising object
};

}  // namespace Atoll
#endif