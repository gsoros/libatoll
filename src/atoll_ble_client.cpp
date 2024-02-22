#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED) && !defined(CONFIG_BT_NIMBLE_ROLE_OBSERVER_DISABLED) && defined(FEATURE_BLE_CLIENT)

#include "atoll_ble.h"
#include "atoll_ble_client.h"

using namespace Atoll;

BleClient::~BleClient() {
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        delete peers[i];  // delete nullptr should be safe!
    }
}

void BleClient::setup(const char* deviceName, ::Preferences* p
#ifdef FEATURE_API
                      ,
                      Api* api
#endif
) {
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    preferencesSetup(p, "BleClient");
    loadSettings();
    printSettings();
    enabled = true;

    init();

    scan = BLEDevice::getScan();

    scan->setScanCallbacks(this);

    // client->start();
#ifdef FEATURE_API
    this->api = api;
    api->addCommand(Api::Command("peers", [this](Api::Message* m) -> Api::Result* {
        return peersProcessor(m);
    }));
#endif
}

void BleClient::init() {
    Ble::init(deviceName);
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
    ulong t = millis();
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        // log_d("checking peer %d %s en: %d, isConn:%d shouldConn:%d conning:%d remov:%d",
        //       i,
        //       peers[i]->saved.name,
        //       peers[i]->enabled,
        //       peers[i]->isConnected(),
        //       peers[i]->shouldConnect,
        //       peers[i]->connecting,
        //       peers[i]->markedForRemoval);
        if (0 == strcmp(peers[i]->saved.address, "")) {
            log_e("peer %d %s %s address is empty", i, peers[i]->saved.name, peers[i]->saved.type);
            continue;
        }
        if (peers[i]->markedForRemoval) {
            if (peers[i]->isConnected()) {
                log_i("disconnecting peer %s", peers[i]->saved.name);
                peers[i]->disconnect();
                while (peers[i]->isConnected()) {
                    log_d("waiting for disconnect...");
                    delay(333);
                }
            }
            log_i("removing peer %s", peers[i]->saved.name);
            removePeer(peers[i], false);
            continue;
        } else if (peers[i]->enabled &&
                   peers[i]->shouldConnect &&
                   !peers[i]->isConnected() &&
                   !peers[i]->connecting &&
                   (0 == peers[i]->lastConnectionAttempt ||
                    peers[i]->lastConnectionAttempt + reconnectDelay < t)) {
            // log_d("connecting peer %s %s(%d)",
            //       peers[i]->saved.name, peers[i]->saved.address, peers[i]->saved.addressType);
            peers[i]->connect();
        } else if ((!peers[i]->enabled ||
                    !peers[i]->shouldConnect) &&
                   peers[i]->isConnected()) {
            log_i("disconnecting peer %s", peers[i]->saved.name);
            peers[i]->disconnect();
            continue;
        } else if (peers[i]->isConnected())
            peers[i]->loop();
    }
    // log_i("loop end");
}

void BleClient::stop() {
    shouldStop = true;
}

void BleClient::loadSettings() {
    if (!preferencesStartLoad()) return;
    for (uint8_t i = 0; i < peersMax; i++) {
        char key[8] = "";
        snprintf(key, sizeof(key), "peer%d", i);
        char packed[Peer::packedMaxLength] = "";
        strncpy(packed,
                preferences->getString(key, packed).c_str(),
                sizeof(packed));
        if (strlen(packed) < 1) continue;
        // log_d("loading %s: %s", key, packed);
        Peer::Saved saved;
        if (Peer::unpack(packed, &saved)) {
            Peer* peer = createPeer(saved);
            if (nullptr == peer) continue;  // delete nullptr should be safe!
            if (!addPeer(peer)) delete peer;
        }
    }
    preferencesEnd();
}

void BleClient::saveSettings() {
    if (!preferencesStartSave()) return;
    char key[8] = "";
    char packed[Peer::packedMaxLength] = "";
    char saved[Peer::packedMaxLength] = "";
    for (uint8_t i = 0; i < peersMax; i++) {
        snprintf(key, sizeof(key), "peer%d", i);
        strncpy(packed, "", sizeof(packed));
        strncpy(saved,
                preferences->getString(key).c_str(),
                sizeof(saved));
        if (nullptr != peers[i] &&
            !peers[i]->markedForRemoval &&
            !peers[i]->pack(packed, sizeof(packed))) {
            log_e("could not pack %s: %s", key, peers[i]->saved.address);
            continue;
        }
        log_i("saving %s: %s", key, packed);
        preferences->putString(key, packed);
    }
    preferencesEnd();
}

void BleClient::printSettings() {
    for (uint8_t i = 0; i < peersMax; i++)
        if (nullptr != peers[i])
            log_i("peer %d name: %s, type: %s, address: %s(%d), passkey: %d",
                  i, peers[i]->saved.name, peers[i]->saved.type, peers[i]->saved.address, peers[i]->saved.addressType, peers[i]->saved.passkey);
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

bool BleClient::startScan(uint32_t duration) {
    if (scan->isScanning()) return false;

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

    bool ret = scan->start(duration, false);
#ifdef FEATURE_API
    if (nullptr == api) {
        log_e("api is null");
    } else {
        char reply[ATOLL_API_MSG_REPLY_LENGTH];
        snprintf(reply, sizeof(reply), "%d;%d=scan:%d",
                 ret ? api->success()->code : api->error()->code,
                 api->command("peers")->code,
                 (uint8_t)ret);

        log_d("calling api.notifyTXChar('%s')", reply);
        api->notifyTxChar(reply);
    }
#endif
    return ret;
}

Peer* BleClient::createPeer(Peer::Saved saved) {
    // log_d("creating %s,%d,%s,%s,%d", saved.address, saved.addressType, saved.type, saved.name, saved.passkey);
    Peer* peer;
    if (strstr(saved.type, "E"))
        peer = new ESPM(saved);
    else if (strstr(saved.type, "P"))
        peer = new PowerMeter(saved);
    else if (strstr(saved.type, "H"))
        peer = new HeartrateMonitor(saved);
    else if (strstr(saved.type, "V"))
        peer = new Vesc(saved);
    else if (strstr(saved.type, "B"))
        peer = new JkBms(saved);
    else
        return nullptr;
    return peer;
}

Peer* BleClient::createPeer(BLEAdvertisedDevice* device) {
    Peer::Saved saved;
    strncpy(saved.address, device->getAddress().toString().c_str(), sizeof(saved.address));
    saved.addressType = device->getAddress().getType();
    strncpy(saved.name, device->getName().c_str(), sizeof(saved.name));

    Peer* peer = nullptr;
    if (device->isAdvertisingService(BLEUUID(ESPM_API_SERVICE_UUID))) {
        strncpy(saved.type, "E", sizeof(saved.type));
        peer = new ESPM(saved);
    } else if (device->isAdvertisingService(BLEUUID(CYCLING_POWER_SERVICE_UUID))) {
        strncpy(saved.type, "P", sizeof(saved.type));
        peer = new PowerMeter(saved);
    } else if (device->isAdvertisingService(BLEUUID(HEART_RATE_SERVICE_UUID))) {
        strncpy(saved.type, "H", sizeof(saved.type));
        peer = new HeartrateMonitor(saved);
    } else if (device->isAdvertisingService(BLEUUID(VESC_SERVICE_UUID))) {
        strncpy(saved.type, "V", sizeof(saved.type));
        peer = new Vesc(saved);
    } else if (device->isAdvertisingService(BLEUUID(PeerCharacteristicJkBms::SERVICE_UUID))) {
        strncpy(saved.type, "B", sizeof(saved.type));
        peer = new JkBms(saved);
    }
    return peer;
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
        if (nullptr != peers[i] && 0 == strcmp(peers[i]->saved.address, address)) {
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
    if (strlen(peer->saved.address) < sizeof(Peer::Saved::address) - 1) {
        log_e("not adding peer, address too short (%d)", strlen(peer->saved.address));
        return false;
    }
    if (strlen(peer->saved.type) < 1) {
        log_e("not adding peer with empty type");
        return false;
    }
    if (strlen(peer->saved.name) < 1) {
        log_e("not adding peer with empty name");
        return false;
    }
    int8_t index = peerIndex(peer->saved.address);
    if (index < 0) index = peerIndex("");
    if (index < 0) {
        log_e("cannot add peer %s", peer->saved.name);
        return false;
    }
    peer->connectedCallback = [this](Peer* peer) { onPeerConnected(peer); };
    // log_i("adding peer %s %s(%d)", peer->name, peer->address, peer->addressType);
    peers[index] = peer;
    return true;
}

uint8_t BleClient::removePeer(const char* address, bool markOnly) {
    if (strlen(address) < 1) return 0;
    uint8_t removed = 0;
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        log_d("comparing '%s' to '%s'", peers[i]->saved.address, address);
        if (0 == strcmp(peers[i]->saved.address, address)) {
            if (markOnly) {
                peers[i]->markedForRemoval = true;
                log_d("peer marked for removal: %s", peers[i]->saved.name);
            } else {
                log_d("deleting peer %s", peers[i]->saved.name);
                delete peers[i];  // delete nullptr should be safe!
                peers[i] = nullptr;
            }
            removed++;
        }
    }
    return removed;
}

uint8_t BleClient::setPeerEnabled(const char* name, bool value) {
    if (strlen(name) < 1) return 0;
    uint8_t changed = 0;
    for (int8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        // log_d("comparing '%s' to '%s'", peers[i]->saved.name, name);
        if (0 == strcmp(peers[i]->saved.name, name) && peers[i]->enabled != value) {
            if (peers[i]->enabled) {
                log_d("disabling %s", peers[i]->saved.name);
                peers[i]->enabled = false;
                peers[i]->shouldConnect = false;
                // if (peers[i]->isConnected()) peers[i]->disconnect();
            } else {
                log_d("enabling %s", peers[i]->saved.name);
                peers[i]->enabled = true;
                peers[i]->shouldConnect = true;
                // if (!peers[i]->isConnected()) peers[i]->connect();
            }
            changed++;
        }
    }
    return changed;
}

uint8_t BleClient::removePeer(Peer* peer, bool markOnly) {
    if (nullptr == peer) return 0;
    return removePeer(peer->saved.address, markOnly);
}

Peer* BleClient::getFirstPeerByName(const char* name) {
    for (uint8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (0 == strcmp(name, peers[i]->saved.name))
            return peers[i];
    }
    return nullptr;
}

Peer* BleClient::getFirstPeerByType(const char* type) {
    for (uint8_t i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (0 == strcmp(type, peers[i]->saved.type))
            return peers[i];
    }
    return nullptr;
}

void BleClient::onNotify(BLECharacteristic* pCharacteristic) {
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

#ifdef FEATURE_API
    if (nullptr == api) {
        log_e("api is null");
        return;
    }
    if (peerExists(advertisedDevice->getAddress().toString().c_str())) {
        log_i("peer exists");
        return;
    }
    if (advertisedDevice->haveTargetAddress())
        for (int i = 0; i < advertisedDevice->getTargetAddressCount(); i++)
            log_i("Target address %i: %s", advertisedDevice->getTargetAddress(i).toString().c_str());

    Peer* peer = createPeer(advertisedDevice);
    if (nullptr == peer) {
        log_i("peer %s is null", advertisedDevice->getName().c_str());
        return;
    }

    char reply[ATOLL_API_MSG_REPLY_LENGTH];
    snprintf(reply, sizeof(reply), "%d;%d=scanResult:%s,%d,%s,%s",
             api->success()->code,
             api->command("peers")->code,
             peer->saved.address,
             peer->saved.addressType,
             peer->saved.type,
             peer->saved.name);

    log_i("calling api.notifyTxChar('%s')", reply);
    api->notifyTxChar(reply);

    delete peer;
#endif
}

void BleClient::onScanEnd(BLEScanResults results) {
    log_i("scan end");
#ifdef FEATURE_API
    if (nullptr == api) {
        log_e("api is null");
        return;
    }
    char reply[ATOLL_API_MSG_REPLY_LENGTH];
    snprintf(reply, sizeof(reply), "%d;%d=scan:%d",
             api->success()->code,
             api->command("peers")->code,
             0);

    log_i("calling api.notifyTxChar('%s')", reply);
    api->notifyTxChar(reply);
#endif
}

#ifdef FEATURE_API

Api::Result* BleClient::peersProcessor(Api::Message* msg) {
    if (msg->argStartsWith("scan")) {
        if (!msg->argStartsWith("scan:")) {
            msg->replyAppend("usage: scan:duration in seconds");
            return Api::argInvalid();
        }
        char* param = msg->arg + 5;
        if (!strlen(param)) return Api::result("argInvalid");
        int duration = atoi(param);
        if (duration < 1 || 120 < duration) return Api::result("argInvalid");
        if (scan->isScanning()) {
            snprintf(msg->reply, sizeof(msg->reply), "%s", "already scanning");
            return Api::error();
        }
        if (!startScan(duration * 1000)) {  // convert duration from s to ms
            snprintf(msg->reply, sizeof(msg->reply), "%s", "could not start");
            return Api::error();
        }
        snprintf(msg->reply, sizeof(msg->reply), "scan:%d", duration);
        return Api::success();
    }
    if (msg->argStartsWith("scanResult")) {
        if (msg->log) log_e("scanResult cannot be called directly, replies are generated after starting a scan");
        return Api::error();
    }
    if (msg->argStartsWith("add")) {
        if (!msg->argStartsWith("add:")) {
            msg->replyAppend("usage: add:address,addressType,type,name,passkey");
            return Api::argInvalid();
        }
        char* param = msg->arg + 4;
        if (strlen(param) < sizeof(Peer::Saved::address) + 5) {
            if (msg->log) log_e("param too short (%d)", strlen(param));
            return Api::result("argInvalid");
        }
        Peer::Saved saved;
        if (!Peer::unpack(
                param,
                &saved)) {
            if (msg->log) log_e("could not unpack %s", param);
            return Api::result("argInvalid");
        }
        if (peerExists(saved.address)) {
            if (msg->log) log_e("peer already exists: %s", saved.address);
            return Api::result("argInvalid");
        }
        Peer* peer = createPeer(saved);
        if (nullptr == peer) {
            if (msg->log) log_e("could not create peer from %s", param);
            return Api::error();
        }
        if (!addPeer(peer)) {
            delete peer;
            if (msg->log) log_e("could not add peer from %s", param);
            return Api::error();
        }
        saveSettings();
        snprintf(msg->reply, sizeof(msg->reply), "%s", msg->arg);
        return Api::success();
    }
    if (msg->argStartsWith("delete")) {
        if (!msg->argStartsWith("delete:")) {
            msg->replyAppend("usage: delete:address");
            return Api::argInvalid();
        }
        char* param = msg->arg + 7;
        if (strlen(param) < sizeof(Peer::Saved::address) - 1) {
            if (msg->log) log_e("arg too short (%d)", strlen(msg->arg));
            return Api::result("argInvalid");
        }
        log_i("removePeer(%s)", param);
        uint8_t changed = removePeer(param);
        if (0 < changed) {
            saveSettings();
            // snprintf(msg->reply, sizeof(msg->reply), "%d", changed);
            snprintf(msg->reply, sizeof(msg->reply), "%s", msg->arg);
            return Api::success();
        }
        return Api::error();
    }
    if (msg->argStartsWith("disable")) {
        if (!msg->argStartsWith("disable:")) {
            msg->replyAppend("usage: disable:name");
            return Api::argInvalid();
        }
        char* param = msg->arg + 8;
        if (strlen(param) < 1) {
            if (msg->log) log_e("arg too short (%d)", strlen(msg->arg));
            return Api::result("argInvalid");
        }
        log_i("setPeerEnabled(%s, false)", param);
        uint8_t changed = setPeerEnabled(param, false);
        if (0 < changed) {
            // snprintf(msg->reply, sizeof(msg->reply), "%d", changed);
            snprintf(msg->reply, sizeof(msg->reply), "%s", msg->arg);
            return Api::success();
        }
        return Api::error();
    }
    if (msg->argStartsWith("enable")) {
        if (!msg->argStartsWith("enable:")) {
            msg->replyAppend("usage: enable:name");
            return Api::argInvalid();
        }
        char* param = msg->arg + 7;
        if (strlen(param) < 1) {
            if (msg->log) log_e("arg too short (%d)", strlen(msg->arg));
            return Api::result("argInvalid");
        }
        log_i("setPeerEnabled(%s, true)", param);
        uint8_t changed = setPeerEnabled(param, true);
        if (0 < changed) {
            // snprintf(msg->reply, sizeof(msg->reply), "%d", changed);
            snprintf(msg->reply, sizeof(msg->reply), "%s", msg->arg);
            return Api::success();
        }
        return Api::error();
    }

    // char value[Api::msgReplyLength] = "";
    char* target = msg->reply;
    strncpy(target, "", 1);
    int16_t remaining = sizeof(msg->reply) - 1;
    char token[Peer::packedMaxLength + 1];
    for (int i = 0; i < peersMax; i++) {
        if (nullptr == peers[i]) continue;
        if (peers[i]->markedForRemoval) continue;
        if (strlen(target)) {
            strncat(target, "|", remaining);
            target++;
        }
        peers[i]->pack(token, sizeof(token) - 1, false);
        size_t tokenLen = strlen(token);
        if (remaining < tokenLen) {
            if (msg->log) log_e("no space left for adding '%s' to '%s'", token, target);
            return Api::internalError();
        }
        strncat(target, token, remaining);
        remaining -= tokenLen;
        target += tokenLen;
    }
    return Api::success();
}

#endif

#endif