#include "atoll_ble.h"
#include "atoll_ble_client.h"

using namespace Atoll;

BleClient::~BleClient() {}  // avoid "undefined reference to ..."

void BleClient::setup(const char* deviceName, ::Preferences* p) {
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    preferencesSetup(p, "BleClient");
    loadSettings();
    printSettings();
    enabled = true;

    BLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
    BLEDevice::setScanDuplicateCacheSize(200);

    Ble::init(deviceName);

    // client = BLEDevice::createClient();
    // client->setCallbacks(this);
    scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this);

    // client->start();
}

void BleClient::loop() {
    if (!enabled) return;
    for (int8_t i = 0; i < ATOLL_BLE_CLIENT_PEERS; i++) {
        if (0 == strcmp(peers[i].address, "")) continue;
        if ((peers[i].client == nullptr ||
             !peers[i].client->isConnected()) &&
            !peers[i].connecting)
            connect(&peers[i]);
    }
    delay(2000);
}

// delete clients and stop scanning
void BleClient::stop() {
    for (uint8_t i = 0; i < sizeof(peers); i++)
        if (peers[i].client != nullptr) {
            BLEDevice::deleteClient(peers[i].client);
            peers[i].client = nullptr;
        }
    if (scan->isScanning()) scan->stop();
    enabled = false;
    delay(100);  // give the ble stack a chance to clear packets
}

void BleClient::startScan(uint32_t duration) {
    if (scan->isScanning()) return;

    scan->setActiveScan(true);  // Set active scanning, this will get more data from the advertiser.
    scan->setInterval(97);      // How often the scan occurs / switches channels; in milliseconds
    scan->setWindow(37);        // How long to scan during the interval; in milliseconds.
    scan->setMaxResults(0);     // do not store the scan results, use callback only.

    scan->start(duration, BleClient::onScanComplete, false);
}

void BleClient::connect(PeerDevice* device) {
    // https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino
    if (device->connecting) {
        log_i("already connecting to %s", device->name);
        return;
    }
    device->connecting = true;
    if (nullptr != device->client) {
        if (device->client->isConnected()) {
            log_i("already connected to %s", device->name);
            device->connecting = false;
            return;
        }
        // BLEDevice::deleteClient(device->client);
    }

    if (!device->client && BLEDevice::getClientListSize()) {
        device->client = BLEDevice::getClientByPeerAddress((BLEAddress)device->address);
        if (device->client) {
            if (!device->client->connect((BLEAddress)device->address, false)) {
                log_i("reconnect failed");
                device->connecting = false;
                return;
            }
            log_i("reconnected client");
        } else
            device->client = BLEDevice::getDisconnectedClient();
    }
    if (!device->client) {
        if (BLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            log_i("max clients reached - no more connections available");
            device->connecting = false;
            return;
        }
        device->client = BLEDevice::createClient();
        log_i("New client created");
        device->client->setClientCallbacks(this, false);
        device->client->setConnectionParams(12, 12, 0, 51);
        device->client->setConnectTimeout(5);
        if (!device->client->connect((BLEAddress)device->address)) {
            BLEDevice::deleteClient(device->client);
            device->client = nullptr;
            log_i("failed to connect, deleted client");
            device->connecting = false;
            return;
        }
    }
    if (!device->client->isConnected()) {
        if (!device->client->connect((BLEAddress)device->address)) {
            log_i("failed to connect");
            device->connecting = false;
            return;
        }
    }
    log_i("Connected to %s", device->client->getPeerAddress().toString().c_str());
    device->connecting = false;

    /** Now we can read/write/subscribe the charateristics of the services we are interested in */

    /*
    NimBLERemoteService* pSvc = nullptr;
    NimBLERemoteCharacteristic* pChr = nullptr;
    NimBLERemoteDescriptor* pDsc = nullptr;

    if (pSvc) {
        pChr = pSvc->getCharacteristic("BEEF");

        if (pChr) {
            if (pChr->canRead()) {
                Serial.print(pChr->getUUID().toString().c_str());
                Serial.print(" Value: ");
                Serial.println(pChr->readValue().c_str());
            }

            if (pChr->canWrite()) {
                if (pChr->writeValue("Tasty")) {
                    Serial.print("Wrote new value to: ");
                    Serial.println(pChr->getUUID().toString().c_str());
                } else {
                    device->client->disconnect();
                    return false;
                }

                if (pChr->canRead()) {
                    Serial.print("The value of: ");
                    Serial.print(pChr->getUUID().toString().c_str());
                    Serial.print(" is now: ");
                    Serial.println(pChr->readValue().c_str());
                }
            }
            if (pChr->canNotify()) {
                // if(!pChr->registerForNotify(notifyCB)) {
                if (!pChr->subscribe(true, notifyCB)) {
         device->client->disconnect();
    return ;
}
}
else if (pChr->canIndicate()) {
    // if(!pChr->registerForNotify(notifyCB, false)) {
    if (!pChr->subscribe(false, notifyCB)) {
        device->client->disconnect();
        return false;
    }
}
}
}
else {
    Serial.println("DEAD service not found.");
}

pSvc = device->client->getService("BAAD");
if (pSvc) {
    pChr = pSvc->getCharacteristic("F00D");

    if (pChr) {
        if (pChr->canRead()) {
            Serial.print(pChr->getUUID().toString().c_str());
            Serial.print(" Value: ");
            Serial.println(pChr->readValue().c_str());
        }

        pDsc = pChr->getDescriptor(NimBLEUUID("C01D"));
        if (pDsc) {
            Serial.print("Descriptor: ");
            Serial.print(pDsc->getUUID().toString().c_str());
            Serial.print(" Value: ");
            Serial.println(pDsc->readValue().c_str());
        }

        if (pChr->canWrite()) {
            if (pChr->writeValue("No tip!")) {
                Serial.print("Wrote new value to: ");
                Serial.println(pChr->getUUID().toString().c_str());
            } else {
                device->client->disconnect();
                return false;
            }

            if (pChr->canRead()) {
                Serial.print("The value of: ");
                Serial.print(pChr->getUUID().toString().c_str());
                Serial.print(" is now: ");
                Serial.println(pChr->readValue().c_str());
            }
        }
        if (pChr->canNotify()) {
            // if(!pChr->registerForNotify(notifyCB)) {
            if (!pChr->subscribe(true, notifyCB)) {
                device->client->disconnect();
                return false;
            }
        } else if (pChr->canIndicate()) {
            // if(!pChr->registerForNotify(notifyCB, false)) {
            if (!pChr->subscribe(false, notifyCB)) {
                device->client->disconnect();
                return false;
            }
        }
    }

} else {
    Serial.println("BAAD service not found.");
}
*/

    log_i("Done with this device!");

    ///////////////////////////////////////////////
}

void BleClient::disconnect(PeerDevice* device) {
    if (nullptr == device->client) return;
    if (device->client->isConnected())
        device->client->disconnect();
    BLEDevice::deleteClient(device->client);
    device->client = nullptr;
}

int8_t BleClient::firstPeerIndex(const char* address) {
    for (int8_t i = 0; i < ATOLL_BLE_CLIENT_PEERS; i++)
        if (0 == strcmp(peers[i].address, address)) return i;
    return -1;
}

void BleClient::addPeer(PeerDevice device) {
    int8_t index = firstPeerIndex(device.address);
    if (index < 0) index = firstPeerIndex("");
    if (index < 0) {
        log_e("no slots left to add peer %s", device.name);
        return;
    }
    log_i("adding peer %s %s", device.name, device.address);
    peers[index] = device;
}

void BleClient::removePeer(PeerDevice device) {
    for (int8_t i = 0; i < ATOLL_BLE_CLIENT_PEERS; i++)
        if (peers[i].address == device.address) {
            if (nullptr != peers[i].client) {
                BLEDevice::deleteClient(peers[i].client);
                peers[i].client = nullptr;
            }
            peers[i] = PeerDevice();
        }
}

void BleClient::onNotify(BLECharacteristic* pCharacteristic) {
    log_i("TODO");
}

void BleClient::loadSettings() {
    if (!preferencesStartLoad()) return;
    log_i("TODO");
    // hosts = unpackHosts(preferences->getString("hosts"));
    preferencesEnd();
}

void BleClient::saveSettings() {
    if (!preferencesStartSave()) return;
    log_i("TODO");
    // preferences->putString("hosts", packHosts());
    preferencesEnd();
}

void BleClient::printSettings() {
    log_i("TODO");
    // log_i("hosts: %s", packHosts());
}

/**
 * @brief Called after client connects.
 * @param [in] device->client A pointer to the calling client object.
 */
void BleClient::onConnect(BLEClient* client) { log_i("TODO"); }

/**
 * @brief Called when disconnected from the server.
 * @param [in] device->client A pointer to the calling client object.
 */
void BleClient::onDisconnect(BLEClient* client) { log_i("TODO"); }

/**
 * @brief Called when server requests to update the connection parameters.
 * @param [in] device->client A pointer to the calling client object.
 * @param [in] params A pointer to the struct containing the connection parameters requested.
 * @return True to accept the parmeters.
 */
bool BleClient::onConnParamsUpdateRequest(BLEClient* client, const ble_gap_upd_params* params) {
    log_i("TODO client: %s st: %d", client->getPeerAddress().toString().c_str(), params->supervision_timeout);
    return true;
}

/**
 * @brief Called when server requests a passkey for pairing.
 * @return The passkey to be sent to the server.
 */
uint32_t BleClient::onPassKeyRequest() {
    log_e("TODO send saved passkey");
    return 123;
}

/*
void BleClient::onPassKeyNotify(uint32_t pass_key);
bool BleClient::onSecurityRequest() {}
*/

/**
 * @brief Called when the pairing procedure is complete.
 * @param [in] desc A pointer to the struct containing the connection information.\n
 * This can be used to check the status of the connection encryption/pairing.
 */
void BleClient::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    log_i("TODO state: %d", desc->sec_state);
}

/**
 * @brief Called when using numeric comparision for pairing.
 * @param [in] pin The pin to compare with the server.
 * @return True to accept the pin.
 */
bool BleClient::onConfirmPIN(uint32_t pin) {
    log_i("TODO");
    return true;
}

/**
 * @brief Called when a new scan result is detected.
 *
 * As we are scanning, we will find new devices.  When found, this call back is invoked with a reference to the
 * device that was found.  During any individual scan, a device will only be detected one time.
 */
void BleClient::onResult(BLEAdvertisedDevice* advertisedDevice) {
    log_i("scan found %s", advertisedDevice->toString().c_str());
}

void BleClient::onScanComplete(BLEScanResults results) {
    log_i("scan end");
}