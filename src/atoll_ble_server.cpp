#include "atoll_ble.h"
#include "atoll_ble_server.h"

using namespace Atoll;
BleServer::BleServer() {}

BleServer::~BleServer() {}

void BleServer::setup(const char *deviceName) {
    static bool done = false;
    if (done) {
        log_e("setup already done");
        return;
    }
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    enabled = true;
    Ble::init(deviceName);

    server = BLEDevice::createServer();
    if (nullptr == server) {
        log_e("could not create server");
        return;
    }
    server->setCallbacks(this);
    advertising = server->getAdvertising();
#ifdef BLE_APPEARANCE
    advertising->setAppearance(BLE_APPEARANCE);
#endif
    // advertising->setManufacturerData("G");
    // advertising->setScanResponse(false);
    // advertising->setMinPreferred(0x0);

    createDeviceInformationService();

    done = true;
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
    if (nullptr != advertising && !advertising->isAdvertising())
        startAdvertising();
}

BLEService *BleServer::createService(const BLEUUID &uuid) {
    return server->createService(uuid);
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

BLEService *BleServer::getService(const BLEUUID &uuid) {
    return server->getServiceByUUID(uuid);
}

BLECharacteristic *BleServer::getChar(const BLEUUID &serviceUuid, const BLEUUID &charUuid) {
    BLEService *s = getService(serviceUuid);
    if (nullptr == s) return nullptr;
    return s->getCharacteristic(charUuid);
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
    c->notify();
}

// disconnect clients, stop advertising and shutdown AtollBle
void BleServer::stop() {
    log_i("stop");
    server->stopAdvertising();
    while (!_clients.isEmpty())
        server->disconnect(_clients.shift());
    enabled = false;
    delay(100);  // give the AtollBle stack a chance to clear packets
}

void BleServer::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
    log_i("client connected, ID: %d Address: %s\n",
          desc->conn_handle,
          BLEAddress(desc->peer_ota_addr).toString().c_str());
    // NimBLEDevice::startSecurity(desc->conn_handle);
    //  save client handle so we can gracefully disconnect them
    bool savedClientHandle = false;
    for (decltype(_clients)::index_t i = 0; i < _clients.size(); i++)
        if (_clients[i] == desc->conn_handle)
            savedClientHandle = true;
    if (!savedClientHandle)
        _clients.push(desc->conn_handle);
}

void BleServer::onDisconnect(BLEServer *pServer) {
    log_i("Server onDisconnect");
}

void BleServer::start() {
    log_i("starting ble server");
    server->start();
    started = true;
}

void BleServer::startAdvertising() {
    if (!enabled) {
        log_i("Not enabled, not starting advertising");
        return;
    }
    delay(300);
    if (!advertising->isAdvertising()) {
        server->startAdvertising();
        // log_i("Start advertising");
    }
}
