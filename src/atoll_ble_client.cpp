#include "atoll_ble.h"
#include "atoll_ble_client.h"

using namespace Atoll;

BleClient::~BleClient() {
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        delete peers[i];  // delete nullptr should be safe!
    }
}

void BleClient::setup(
    const char* deviceName,
    ::Preferences* p) {
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
    if (shouldStop) {
        if (scan->isScanning()) scan->stop();
        disconnectPeers();
        delay(100);
        deleteClients();
        delay(100);
        enabled = false;
        shouldStop = false;
        taskStop();
        return;
    }
    if (scan->isScanning()) return;
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        // log_i("checking peer %d %s", i, peers[i]->name);
        if (0 == strcmp(peers[i]->address, "")) continue;
        if (peers[i]->markedForRemoval) {
            log_i("removing peer %s", peers[i]->name);
            removePeer(peers[i], false);
        } else if (peers[i]->shouldConnect &&
                   !peers[i]->isConnected() &&
                   !peers[i]->connecting) {
            // log_i("connecting peer %s %s(%d)",
            //       peers[i]->name, peers[i]->address, peers[i]->addressType);
            peers[i]->connect();
            delay(100);
        } else if (!peers[i]->shouldConnect &&
                   peers[i]->isConnected()) {
            log_i("disconnecting peer %s", peers[i]->name);
            peers[i]->disconnect();
        }
    }
}

void BleClient::stop() {
    shouldStop = true;
}

void BleClient::disconnectPeers() {
    for (uint8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (peers[i]->isConnected())
            peers[i]->disconnect();
    }
}

void BleClient::deleteClients() {
    std::__cxx11::list<BLEClient*>* clients = BLEDevice::getClientList();
    for (auto it = clients->cbegin(); it != clients->cend(); ++it)
        if (!BLEDevice::deleteClient((*it)))
            log_i("failed to delete client");

    for (uint8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (peers[i]->hasClient())
            peers[i]->unsetClient();
        // peers[i]->deleteClient();
    }
}

uint32_t BleClient::startScan(uint32_t duration) {
    if (scan->isScanning()) return 0;

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

    if (callScanStart(duration))
        return duration;
    return 0;
}

bool BleClient::callScanStart(uint32_t duration) {
    return scan->start(duration, onScanComplete, false);
}

// get index of existing peer address
// or first unused index for an empty address
int8_t BleClient::peerIndex(const char* address) {
    // log_i("searching index for address '%s'", address);
    size_t addressLen = strlen(address);
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i] && !addressLen) {
            // log_i("found unused index %d for empty address", i);
            return i;
        }
        if (nullptr != peers[i] && 0 == strcmp(peers[i]->address, address)) {
            // log_i("found index %d of address '%s'", i, address);
            return i;
        }
    }
    // log_i("could not find index of address '%s'", address);
    return -1;
}

bool BleClient::peerExists(const char* address) {
    bool res = 0 <= peerIndex(address);
    log_i("peer with address '%s' %s", address, res ? "exists" : "does not exist");
    return res;
}

bool BleClient::addPeer(Peer* peer) {
    if (nullptr == peer) {
        log_e("peer is null");
        return false;
    }
    if (strlen(peer->address) < sizeof(Peer::address) - 1) {
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
    // log_i("adding peer %s %s(%d)", peer->name, peer->address, peer->addressType);
    peers[index] = peer;
    return true;
}

uint8_t BleClient::removePeer(const char* address, bool markOnly) {
    if (strlen(address) < 1) return 0;
    uint8_t removed = 0;
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        log_i("comparing '%s' to '%s'", peers[i]->address, address);
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

uint8_t BleClient::removePeer(Peer* peer, bool markOnly) {
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