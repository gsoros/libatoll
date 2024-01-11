#if !defined(__atoll_ble_client_h) && defined(FEATURE_BLE_CLIENT)
#define __atoll_ble_client_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_ble_constants.h"
#include "atoll_task.h"
#include "atoll_preferences.h"
#include "atoll_peer.h"
#ifdef FEATURE_API
#include "atoll_api.h"
#endif

#ifndef BLE_CHAR_VALUE_MAXLENGTH
#define BLE_CHAR_VALUE_MAXLENGTH 128
#endif

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

#ifndef HOSTNAME
#define HOSTNAME "libAtollBle_unnamed"
#endif

#ifndef ATOLL_BLE_CLIENT_PEERS
#define ATOLL_BLE_CLIENT_PEERS 8
#endif

namespace Atoll {

class BleClient : public Task,
                  public Preferences,
                  public NimBLEScanCallbacks {
   public:
    const char* taskName() { return "BleClient"; }
    char deviceName[SETTINGS_STR_LENGTH] = HOSTNAME;         // device name
    bool enabled = true;                                     // whether ble client is enabled
    BLEScan* scan;                                           // pointer to scan object
    Peer* peers[ATOLL_BLE_CLIENT_PEERS];                     // peer devices we want connected
    static const uint8_t peersMax = ATOLL_BLE_CLIENT_PEERS;  // convenience for iterations
#ifdef FEATURE_API
    Api* api = nullptr;
#endif

    BleClient() {
        for (uint8_t i = 0; i < peersMax; i++) peers[i] = nullptr;  // initialize pointers
    }
    virtual ~BleClient();

    virtual void setup(const char* deviceName, ::Preferences* p
#ifdef FEATURE_API
                       ,
                       Api* api = nullptr
#endif
    );
    virtual void init();
    virtual void loop();
    virtual void stop();

    virtual void loadSettings();
    virtual void saveSettings();
    virtual void printSettings();

    virtual void disconnectPeers();
    virtual void deleteClients();

    // duration is in milliseconds
    virtual bool startScan(uint32_t duration);

    virtual Peer* createPeer(Peer::Saved saved);
    virtual Peer* createPeer(BLEAdvertisedDevice* advertisedDevice);

    virtual int8_t peerIndex(const char* address);
    virtual bool peerExists(const char* address);
    virtual bool addPeer(Peer* peer);
    virtual uint8_t removePeer(Peer* peer, bool markOnly = true);
    virtual uint8_t removePeer(const char* address, bool markOnly = true);
    virtual uint8_t disablePeer(const char* name);
    virtual Peer* getFirstPeerByName(const char* name);

    virtual void onNotify(BLECharacteristic* pCharacteristic);

    typedef std::function<void(Peer*)> Callback;
    Callback onPeerConnected = [](Peer*) {};

   protected:
    bool shouldStop = false;

    // NimBLEScanCallbacks
    virtual void onResult(BLEAdvertisedDevice* advertisedDevice) override;
    virtual void onScanEnd(BLEScanResults results) override;

#ifdef FEATURE_API
    Api::Result* peersProcessor(Api::Message*);
#endif
};

}  // namespace Atoll
#endif