#ifndef __atoll_ble_client_h
#define __atoll_ble_client_h

#include <Arduino.h>
#include <list>
#include <NimBLEDevice.h>
#include <CircularBuffer.h>

#include "atoll_ble_constants.h"
#include "atoll_task.h"
#include "atoll_preferences.h"

#ifndef BLE_CHAR_VALUE_MAXLENGTH
#define BLE_CHAR_VALUE_MAXLENGTH 128
#endif

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

#ifndef HOSTNAME
#define HOSTNAME "libAtollBle_unnamed"
#endif

#ifndef BLE_APPEARANCE
#define BLE_APPEARANCE APPEARANCE_CYCLING_POWER_SENSOR
#endif

#ifndef ATOLL_BLE_CLIENT_PEERS
#define ATOLL_BLE_CLIENT_PEERS 8
#endif

namespace Atoll {

class PeerDevice {
   public:
    char address[18] = "";
    char type[4] = "";
    char name[SETTINGS_STR_LENGTH] = "";
    BLEClient* client = nullptr;
    bool connecting = false;

    PeerDevice() {}
    virtual ~PeerDevice() {}

    PeerDevice(const char* address, const char* type, const char* name) {
        strncpy(this->address, address, sizeof(this->address));
        strncpy(this->type, type, sizeof(this->type));
        strncpy(this->name, name, sizeof(this->name));
    }

    /*
    // format: "address,type,name"
    PeerDevice(const char* str) {
        sscanf(str, "%s,%s,%s", this->address, this->type, this->name);  // TODO do this properly
        // if (0 != strcmp(toString(), str))
        //     log_e("Warning: could not parse %s (got %s)", str, toString());
        log_i("'%s' ===> '%s', '%s', '%s'", str, this->address, this->type, this->name);
    }
    */
    /*
        void pack(char *packed) {
            // sizeof(this->address) + sizeof(this->type) + sizeof(this->name) + 3;
            //log_e("buffer too small");
            sprintf(packed, "%s,%s,%s", this->address, this->type, this->name);
        }
    */
};

class BleClient : public Task,
                  public Preferences,
                  // public BLEServerCallbacks,
                  // public BLECharacteristicCallbacks
                  public BLEClientCallbacks,
                  public BLEAdvertisedDeviceCallbacks {
   public:
    uint32_t taskStack = 4096;                        // task stack size in bytes
    char deviceName[SETTINGS_STR_LENGTH] = HOSTNAME;  // advertised device name
    bool enabled = true;                              // whether ble client is enabled
    BLEScan* scan;                                    // pointer to scan object
    PeerDevice peers[ATOLL_BLE_CLIENT_PEERS];         // peer devices to connect to

    BleClient() {}
    virtual ~BleClient();

    virtual void setup(const char* deviceName, ::Preferences* p);
    virtual void loop();
    virtual void stop();

    virtual void startScan(uint32_t duration);
    virtual void connect(PeerDevice* device);
    virtual void disconnect(PeerDevice* device);

    int8_t firstPeerIndex(const char* address);
    virtual void addPeer(PeerDevice device);
    virtual void removePeer(PeerDevice device);

    virtual void onNotify(BLECharacteristic* pCharacteristic);

    virtual void loadSettings();
    virtual void saveSettings();
    virtual void printSettings();

   protected:
    /* client callbacks */
    virtual void onConnect(BLEClient* pClient);
    virtual void onDisconnect(BLEClient* pClient);
    virtual bool onConnParamsUpdateRequest(BLEClient* pClient, const ble_gap_upd_params* params);
    virtual uint32_t onPassKeyRequest();
    /*
    virtual void onPassKeyNotify(uint32_t pass_key);
    virtual bool onSecurityRequest();
    */
    virtual void onAuthenticationComplete(ble_gap_conn_desc* desc);
    virtual bool onConfirmPIN(uint32_t pin);

    /* advertised device callback */
    virtual void onResult(BLEAdvertisedDevice* advertisedDevice);

    /* scan results callback */
    static void onScanComplete(BLEScanResults results);

   private:
};

}  // namespace Atoll
#endif