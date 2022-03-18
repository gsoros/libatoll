#include "atoll_ble.h"
#include "atoll_ble_client.h"

using namespace Atoll;

BleClient::~BleClient() {
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        delete peers[i];
    }
}

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
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (0 == strcmp(peers[i]->address, "")) continue;
        if (!peers[i]->isConnected() && !peers[i]->connecting)
            peers[i]->connect();
    }
    delay(2000);
}

// delete clients and stop scanning
void BleClient::stop() {
    for (uint8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (peers[i]->hasClient()) {
            peers[i]->deleteClient();
        }
    }
    if (scan->isScanning()) scan->stop();
    enabled = false;
    delay(100);  // give the ble stack a chance to clear packets
}

void BleClient::startScan(uint32_t duration) {
    if (scan->isScanning()) return;

    scan->setDuplicateFilter(false);
    scan->clearDuplicateCache();
    scan->clearResults();
    scan->setFilterPolicy(BLE_HCI_SCAN_FILT_NO_WL);
    scan->setLimitedOnly(false);
    scan->setActiveScan(true);  // Set active scanning, this will get more data from the advertiser.
    scan->setInterval(97);      // How often the scan occurs / switches channels; in milliseconds
    scan->setWindow(37);        // How long to scan during the interval; in milliseconds.
    scan->setMaxResults(0);     // do not store the scan results, use callback only.

    scan->start(duration, BleClient::onScanComplete, false);
}

// get index of existing peer address
// or first unused index for an empty address
int8_t BleClient::peerIndex(const char* address) {
    size_t addressLen = strlen(address);
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i] && !addressLen) {
            log_i("found unused index %d for empty address", i);
            return i;
        }
        if (nullptr != peers[i] && 0 == strcmp(peers[i]->address, address)) {
            log_i("found index %d of address '%s'", i, address);
            return i;
        }
    }
    // log_i("could not find index of address '%s'", address);
    return -1;
}

bool BleClient::peerExists(const char* address) {
    return 0 <= peerIndex(address);
}

bool BleClient::addPeer(BlePeerDevice* device) {
    int8_t index = peerIndex(device->address);
    if (index < 0) index = peerIndex("");
    if (index < 0) {
        log_e("cannot add peer %s", device->name);
        return false;
    }
    log_i("adding peer %s %s", device->name, device->address);
    peers[index] = device;
    return true;
}

void BleClient::removePeer(BlePeerDevice* device) {
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (0 == strcmp(peers[i]->address, device->address)) {
            delete peers[i];
            peers[i] = nullptr;
        }
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