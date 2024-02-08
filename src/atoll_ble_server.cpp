#if !defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED) && defined(FEATURE_BLE_SERVER)

#include "atoll_ble.h"
#include "atoll_ble_server.h"

using namespace Atoll;

BleServer::BleServer() {
    _clients.clear();
}

BleServer::~BleServer() {}

void BleServer::setup(const char *deviceName) {
    if (setupDone) {
        log_e("setup already done");
        return;
    }
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    enabled = true;

    init();

    server = BLEDevice::createServer();
    if (nullptr == server) {
        log_e("could not create server");
        return;
    }
    server->setCallbacks(this);

    advertising = server->getAdvertising();
    setupAdvertising();

    createDeviceInformationService();

    setupDone = true;
}

void BleServer::init() {
    Ble::init(deviceName);
}

void BleServer::deinit() {
    // delete advertising;
    // delete server;
    Ble::deinit();
}

void BleServer::reinit() {
    deinit();
    setupDone = false;
    setup(deviceName);
}

uint16_t BleServer::getAppearance() {
    return APPEARANCE_CYCLING_GENERIC;
}

void BleServer::setupAdvertising() {
    advertising->setScanResponse(true);
    advertising->setAppearance(getAppearance());
    advertising->setName(deviceName);
    // advertising->setMinInterval((uint16_t)(1000 / 0.625));  // 1 s
    // advertising->setMaxInterval((uint16_t)(3000 / 0.625));  //  3 s
    //  advertising->setManufacturerData("G");
    //  advertising->setScanResponse(false);
    //  advertising->setMinPreferred(0x0);
}

bool BleServer::createDeviceInformationService() {
    BLEService *s = createService(BLEUUID(DEVICE_INFORMATION_SERVICE_UUID));
    if (nullptr == s) {
        log_e("could not create service");
        return false;
    }
    BLECharacteristic *c = s->createCharacteristic(
        BLEUUID(DEVICE_NAME_CHAR_UUID),
        BLE_PROP::READ);
    if (nullptr == c) {
        log_e("could not create char");
        return false;
    }
    c->setCallbacks(this);
    c->setValue((uint8_t *)deviceName, strlen(deviceName));
    if (!s->start()) {
        log_e("could not start service");
        return false;
    }
    advertiseService(BLEUUID(DEVICE_INFORMATION_SERVICE_UUID));
    return true;
}

void BleServer::loop() {
    if (!enabled) return;
    if (!started) {
        log_e("not started");
        return;
    }
    if (nullptr == advertising) return;
    if (advertising->isAdvertising()) {
        if (0 < advertisingTimeoutMs && advertisingTimeoutMs < millis()) {
            log_d("advertising timeout reached");
            stopAdvertising();
            return;
        }
        if (_clients.capacity <= _clients.size()) {
            log_d("max %d clients reached", _clients.capacity);
            stopAdvertising();
        }
        return;
    }
    // not advertising
    if (_clients.size() < _clients.capacity)
        startAdvertising();
}

void BleServer::setSecurity(bool state, const uint32_t passkey) {
    // log_d("state: %s, passkey: %d", state ? "true" : "false", passkey);
    if (state) {
        BLEDevice::setSecurityAuth(true, true, true);
        if (passkey) {
            BLEDevice::setSecurityPasskey(passkey);
        }
        return;
    }
    BLEDevice::setSecurityAuth(false, false, false);
}

BLEService *BleServer::createService(const BLEUUID &uuid) {
    return server->createService(uuid);
}

void BleServer::removeService(BLEService *s) {
    server->removeService(s);
}

void BleServer::advertiseService(const BLEUUID &uuid, uint8_t advType) {
    if (0 == advType)
        advertising->addServiceUUID(uuid);
    else if (1 == advType) {
        BLEAdvertisementData responseData;
        responseData.setCompleteServices(uuid);
        advertising->setScanResponseData(responseData);
    }
}

void BleServer::unAdvertiseService(const BLEUUID &uuid, uint8_t advType) {
    if (0 == advType)
        advertising->removeServiceUUID(uuid);
    else if (1 == advType) {
        BLEAdvertisementData responseData;
        advertising->setScanResponseData(responseData);
    }
}

BLEService *BleServer::getService(const BLEUUID &uuid) {
    return server->getServiceByUUID(uuid);
}

BLECharacteristic *BleServer::getChar(const BLEUUID &serviceUuid, const BLEUUID &charUuid) {
    BLEService *s = getService(serviceUuid);
    if (nullptr == s) return nullptr;
    return s->getCharacteristic(charUuid);
}

void BleServer::start() {
    log_i("starting ble server");
    server->start();
    started = true;
}

void BleServer::startAdvertising() {
    if (!enabled) {
        log_d("not enabled");
        return;
    }
    if (!Ble::initDone()) {
        log_e("init not done");
        return;
    }
    if (nullptr == advertising) {
        log_e("adv is null");
        return;
    }
    if (!advertising->isAdvertising()) {
        if (!server->startAdvertising()) {
            log_e("failed to start");
            return;
        }
        log_d("started");
    }
}

void BleServer::stopAdvertising() {
    if (!enabled) {
        log_d("not enabled");
        return;
    }
    if (nullptr == advertising) {
        log_e("adv is null");
        return;
    }
    if (advertising->isAdvertising()) {
        if (!server->stopAdvertising()) {
            log_e("failed to stop");
            return;
        }
        log_d("stopped");
    }
}

void BleServer::notify(
    const BLEUUID &serviceUuid,
    const BLEUUID &charUuid,
    uint8_t *data,
    size_t size) {
    if (!enabled) {
        log_d("not enabled, not notifying %s %s",
              serviceUuid.toString().c_str(), charUuid.toString().c_str());
        return;
    }
    BLECharacteristic *c = getChar(serviceUuid, charUuid);
    if (nullptr == c) {
        log_e("could not get %s %s",
              serviceUuid.toString().c_str(), charUuid.toString().c_str());
        return;
    }
    c->setValue(data, size);
    if (!_clients.size()) {
        // log_d("no clients connected, not notifying");
        return;
    }
    c->notify();
}

// disconnect clients, stop advertising and shutdown AtollBle
void BleServer::stop() {
    log_i("stopping");
    server->stopAdvertising();
    while (!_clients.isEmpty())
        server->disconnect(_clients.shift());
    enabled = false;
    delay(100);  // give the stack a chance to clear packets
}

// BLEServerCallbacks

void BleServer::onConnect(BLEServer *pServer, BLEConnInfo &info) {
    log_i("client connected, %s", Ble::connInfoToStr(&info).c_str());
    //  save client handle so we can gracefully disconnect them
    bool exists = false;
    for (decltype(_clients)::index_t i = 0; i < _clients.size(); i++)
        if (_clients[i] == info.getAddress())
            exists = true;
    if (!exists)
        _clients.push(info.getAddress());
}

void BleServer::onDisconnect(BLEServer *pServer, BLEConnInfo &info, int reason) {
    BLEAddress a;
    // remove saved address
    for (decltype(_clients)::index_t i = 0; i < _clients.size(); i++) {
        if (_clients.isEmpty()) break;
        a = _clients.shift();
        if (a != info.getAddress())
            _clients.push(a);
    }
    log_i("client disconnected (reason %d), connInfo: %s, serverConnCount: %d, clientsSize: %d",
          reason, Ble::connInfoToStr(&info).c_str(),
          pServer->getConnectedCount(), _clients.size());
}

void BleServer::onMTUChange(uint16_t mtu, BLEConnInfo &info) {
    log_i("MTU changed: %s", Ble::connInfoToStr(&info).c_str());
}

uint32_t BleServer::onPassKeyRequest() {
    log_e("TODO not implemented, returning 696669");
    return 696669;
}

void BleServer::onAuthenticationComplete(BLEConnInfo &info) {
    log_i("auth complete, %s", Ble::connInfoToStr(&info).c_str());
}

bool BleServer::onConfirmPIN(uint32_t pin) {
    log_e("TODO not implemented, PIN: %d, returning true", pin);
    return true;
}

#endif