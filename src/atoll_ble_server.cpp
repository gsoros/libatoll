#if !defined(CONFIG_BT_NIMBLE_ROLE_PERIPHERAL_DISABLED)

#include "atoll_ble.h"
#include "atoll_ble_server.h"

using namespace Atoll;

BleServer::BleServer() {
    _clients.clear();
}

BleServer::~BleServer() {}

void BleServer::setup(const char *deviceName) {
    static bool done = false;
    if (done) {
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

    done = true;
}

void BleServer::init() {
    Ble::init(deviceName);
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
        NIMBLE_PROPERTY::READ);
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
    // if (server->getConnectedCount() >= NIMBLE_MAX_CONNECTIONS) {
    // }
    if (nullptr != advertising) {
        if (advertising->isAdvertising() && 0 < advertisingTimeoutMs && millis() < advertisingTimeoutMs) {
            stopAdvertising();
        } else if (!advertising->isAdvertising()) {
            startAdvertising();
        }
    }
}

void BleServer::setSecurity(bool state, const uint32_t passkey) {
    log_i("state: %s, passkey: %d", state ? "true" : "false", passkey);
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
    if (!advertising->isAdvertising()) {
        if (server->startAdvertising()) {
            log_i("started");
            return;
        }
        log_e("failed to start");
    }
}

void BleServer::stopAdvertising() {
    if (!enabled) {
        log_d("not enabled");
        return;
    }
    if (advertising->isAdvertising()) {
        if (server->stopAdvertising()) {
            log_i("stopped");
            return;
        }
        log_e("failed to stop");
    }
}

void BleServer::notify(
    const BLEUUID &serviceUuid,
    const BLEUUID &charUuid,
    uint8_t *data,
    size_t size) {
    if (!enabled) {
        log_i("not enabled, not notifying %s %s",
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
        log_i("no clients connected, not notifying");
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
    bool handleExists = false;
    for (decltype(_clients)::index_t i = 0; i < _clients.size(); i++)
        if (_clients[i] == info.getConnHandle())
            handleExists = true;
    if (!handleExists)
        _clients.push(info.getConnHandle());
}

void BleServer::onDisconnect(BLEServer *pServer, BLEConnInfo &info, int reason) {
    log_i("client disconnected (reason %d), %s", reason, Ble::connInfoToStr(&info).c_str());
    uint16_t handle;
    // remove saved handle
    for (decltype(_clients)::index_t i = 0; i < _clients.size(); i++) {
        if (_clients.isEmpty()) break;
        handle = _clients.shift();
        if (handle != info.getConnHandle())
            _clients.push(handle);
    }
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

// BLECharacteristicCallbacks

void BleServer::onRead(BLECharacteristic *c, BLEConnInfo &connInfo) {
    log_i("%s: value: %s", c->getUUID().toString().c_str(), c->getValue().c_str());
}

void BleServer::onWrite(BLECharacteristic *c, BLEConnInfo &connInfo) {
    log_i("%s: value: %s", c->getUUID().toString().c_str(), c->getValue().c_str());
}

void BleServer::onNotify(BLECharacteristic *c) {
    // log_d("%d", c->getValue<int>());
}

void BleServer::onStatus(BLECharacteristic *c, int code) {
    // log_d("char: %s, code: %d", c->getUUID().toString().c_str(), code);
}

void BleServer::onSubscribe(BLECharacteristic *c, BLEConnInfo &info, uint16_t subValue) {
    char remote[64];
    snprintf(remote, sizeof(remote), "client ID: %d Address: %s (%s) ",
             info.getConnHandle(),
             info.getAddress().toString().c_str(), info.getIdAddress().toString().c_str());
    switch (subValue) {
        case 0:
            log_i("%s unsubscribed from %s", remote, c->getUUID().toString().c_str());
            break;
        case 1:
            log_i("%s subscribed to notfications for %s", remote, c->getUUID().toString().c_str());
            break;
        case 2:
            log_i("%s subscribed to indications for %s", remote, c->getUUID().toString().c_str());
            break;
        case 3:
            log_i("%s subscribed to notifications and indications for %s", remote, c->getUUID().toString().c_str());
            break;
        default:
            log_i("%s did something to %s", remote, c->getUUID().toString().c_str());
    }
    if (c->getUUID() == BLEUUID(API_LOG_CHAR_UUID) && 0 < subValue) {
        Atoll::Log::dumpBootLog();
    }
}
#endif