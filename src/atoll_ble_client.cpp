#include "atoll_ble.h"
#include "atoll_ble_client.h"

using namespace Atoll;

BleClient::~BleClient() {
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        delete peers[i];  // delete nullptr should be safe!
    }
}

void BleClient::setup(const char* deviceName, ::Preferences* p) {
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    preferencesSetup(p, "BleClient");
    loadSettings();
    printSettings();
    enabled = true;

    Ble::init(deviceName);

    scan = BLEDevice::getScan();
    scan->setAdvertisedDeviceCallbacks(this);

    // client->start();
}

void BleClient::loop() {
    if (!enabled) return;
    if (scan->isScanning()) return;
    for (int8_t i = 0; i < peersMax; i++) {
        // log_i("checking peer %d", i);
        if (nullptr == peers[i]) continue;
        if (0 == strcmp(peers[i]->address, "")) continue;
        if (peers[i]->markedForRemoval) {
            log_i("removing peer %s", peers[i]->name);
            removePeer(peers[i], false);
        } else if (peers[i]->shouldConnect &&
                   !peers[i]->isConnected() &&
                   !peers[i]->connecting) {
            log_i("connecting peer %s", peers[i]->name);
            peers[i]->connect();
        } else if (!peers[i]->shouldConnect &&
                   peers[i]->isConnected()) {
            log_i("disconnecting peer %s", peers[i]->name);
            peers[i]->disconnect();
        }
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

    // scan->setDuplicateFilter(false);
    scan->clearDuplicateCache();
    scan->clearResults();
    scan->setFilterPolicy(BLE_HCI_SCAN_FILT_NO_WL);
    // scan->setFilterPolicy(BLE_HCI_SCAN_FILT_NO_WL_INITA);
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
    log_i("searching index for address '%s'", address);
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
    log_i("could not find index of address '%s'", address);
    return -1;
}

bool BleClient::peerExists(const char* address) {
    bool res = 0 <= peerIndex(address);
    log_i("peer with address '%s' %s", address, res ? "exists" : "does not exist");
    return res;
}

bool BleClient::addPeer(BlePeerDevice* peer) {
    if (nullptr == peer) {
        log_e("peer is null");
        return false;
    }
    if (strlen(peer->address) < sizeof(BlePeerDevice::address) - 1) {
        log_e("not adding peer, address too short (%d)", strlen(peer->address));
        return false;
    }
    if (strlen(peer->type) < 1) {
        log_e("not adding peer with empty type");
        return false;
    }
    if (strlen(peer->name) < 1) {
        log_e("not adding peer with empty name");
        return false;
    }
    int8_t index = peerIndex(peer->address);
    if (index < 0) index = peerIndex("");
    if (index < 0) {
        log_e("cannot add peer %s", peer->name);
        return false;
    }
    log_i("adding peer %s %s", peer->name, peer->address);
    peers[index] = peer;
    return true;
}

uint8_t BleClient::removePeer(const char* address, bool markOnly) {
    if (strlen(address) < 1) return 0;
    uint8_t removed = 0;
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (0 == strcmp(peers[i]->address, address)) {
            if (markOnly) {
                peers[i]->markedForRemoval = true;
                log_i("peer marked for removal: %s", peers[i]->name);
            } else {
                log_i("deleting peer %s", peers[i]->name);
                delete peers[i];  // delete nullptr should be safe!
                peers[i] = nullptr;
            }
            removed++;
        }
    }
    return removed;
}

uint8_t BleClient::removePeer(BlePeerDevice* peer, bool markOnly) {
    if (nullptr == peer) return 0;
    return removePeer(peer->address, markOnly);
}

void BleClient::onNotify(BLECharacteristic* pCharacteristic) {
    log_i("not implemented");
}

void BleClient::loadSettings() {
    log_i("not implemented");
}

void BleClient::saveSettings() {
    log_i("not implemented");
}

void BleClient::printSettings() {
    log_i("not implemented");
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