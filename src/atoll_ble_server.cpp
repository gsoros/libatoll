#include "atoll_ble.h"
#include "atoll_ble_server.h"

using namespace Atoll;

BleServer::~BleServer() {}  // avoid "undefined reference to ..."

void BleServer::setup(const char *deviceName, ::Preferences *p, const char *asUuidStr) {
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    preferencesSetup(p, "BleServer");
    asUUID = BLEUUID(asUuidStr);
    loadSettings();
    printSettings();
    enabled = true;
    Ble::init(deviceName);

    if (secureApi) {
        NimBLEDevice::setSecurityAuth(true, true, true);
        NimBLEDevice::setSecurityPasskey(passkey);
        NimBLEDevice::setSecurityIOCap(BLE_HS_IO_DISPLAY_ONLY);
    }

    server = BLEDevice::createServer();
    server->setCallbacks(this);
    advertising = server->getAdvertising();
    advertising->setAppearance(BLE_APPEARANCE);
    // advertising->setManufacturerData("G");
    // advertising->setScanResponse(false);
    // advertising->setMinPreferred(0x0);

    startServices();
    server->start();

    lastBatteryNotification = 0;
}

void BleServer::startServices() {
    startDiService();
    startBlService();
    startApiService();
}

void BleServer::loop() {
    if (!enabled) return;
    if (!advertising->isAdvertising()) startAdvertising();
}

// Start Device Information service
void BleServer::startDiService() {
    Serial.println("[AtollBle] Starting DIS");
    disUUID = BLEUUID(DEVICE_INFORMATION_SERVICE_UUID);
    dis = server->createService(disUUID);
    diChar = dis->createCharacteristic(
        BLEUUID(DEVICE_NAME_CHAR_UUID),
        NIMBLE_PROPERTY::READ
        //| NIMBLE_PROPERTY::NOTIFY
        //| NIMBLE_PROPERTY::INDICATE
    );
    diChar->setValue((uint8_t *)&deviceName, strlen(deviceName));
    diChar->setCallbacks(this);
    dis->start();
    advertising->addServiceUUID(disUUID);
}

// Start Battey Level service
void BleServer::startBlService() {
    Serial.println("[AtollBle] Starting BLS");
    blsUUID = BLEUUID(BATTERY_SERVICE_UUID);
    bls = server->createService(blsUUID);
    blChar = bls->createCharacteristic(
        BLEUUID(BATTERY_LEVEL_CHAR_UUID),
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY
        //| NIMBLE_PROPERTY::INDICATE
    );
    blChar->setCallbacks(this);
    BLEDescriptor *blDesc = blChar->createDescriptor(
        BLEUUID(BATTERY_LEVEL_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    char s[SETTINGS_STR_LENGTH] = "Percentage";
    blDesc->setValue((uint8_t *)s, strlen(s));
    bls->start();
    advertising->addServiceUUID(blsUUID);
}

void BleServer::startApiService() {
    Serial.println("[AtollBle] Starting APIS");
    char s[SETTINGS_STR_LENGTH] = "";
    as = server->createService(asUUID);

    // api char for writing commands
    uint32_t properties = NIMBLE_PROPERTY::WRITE;
    if (secureApi)
        properties |= NIMBLE_PROPERTY::WRITE_ENC | NIMBLE_PROPERTY::WRITE_AUTHEN;
    apiRxChar = as->createCharacteristic(BLEUUID(API_RX_CHAR_UUID), properties);
    apiRxChar->setCallbacks(this);
    // api char for reading commands
    properties = NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::INDICATE | NIMBLE_PROPERTY::NOTIFY;
    if (secureApi)
        properties |= NIMBLE_PROPERTY::READ_ENC | NIMBLE_PROPERTY::READ_AUTHEN;
    apiTxChar = as->createCharacteristic(BLEUUID(API_TX_CHAR_UUID), properties);
    apiTxChar->setCallbacks(this);
    strncpy(s, "Ready", 32);
    apiTxChar->setValue((uint8_t *)s, strlen(s));
    BLEDescriptor *apiDesc = apiRxChar->createDescriptor(
        BLEUUID(API_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    strncpy(s, "ESPM API v0.1", 32);
    apiDesc->setValue((uint8_t *)s, strlen(s));

    as->start();

    // this UUID will not fit in the advertisement packet, add it to the scan response
    BLEAdvertisementData responseData;
    responseData.setCompleteServices(asUUID);
    advertising->setScanResponseData(responseData);

    // advertising->addServiceUUID(asUUID);
}

// Notify Battery Level service
void BleServer::notifyBl(const ulong t, const uint8_t level) {
    if (!enabled) {
        Serial.println("[AtollBle] Not enabled, not notifying BL");
        return;
    }

    if (t - 2000 < lastBatteryNotification) return;
    lastBatteryLevel = level;
    lastBatteryNotification = t;
    blChar->setValue(&lastBatteryLevel, 1);
    blChar->notify();
}

void BleServer::setApiValue(const char *value) {
    if (!enabled) {
        log_i("not enabled, not setting API value");
        return;
    }
    apiTxChar->setValue((uint8_t *)value, strlen(value));
    apiTxChar->notify();
}

const char *BleServer::characteristicStr(BLECharacteristic *c) {
    if (c == nullptr) return "unknown characteristic";
    if (blChar != nullptr && blChar->getHandle() == c->getHandle()) return "BL";
    if (apiTxChar != nullptr && apiTxChar->getHandle() == c->getHandle()) return "APITX";
    if (apiRxChar != nullptr && apiRxChar->getHandle() == c->getHandle()) return "APIRX";
    return c->getUUID().toString().c_str();
}

// disconnect clients, stop advertising and shutdown AtollBle
void BleServer::stop() {
    while (!_clients.isEmpty())
        server->disconnect(_clients.shift());
    server->stopAdvertising();
    enabled = false;
    delay(100);  // give the AtollBle stack a chance to clear packets
    // BLEDevice::deinit(true);  // TODO never returns
}

void BleServer::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
    Serial.printf("[AtollBle] Client connected, ID: %d Address: %s\n",
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
    Serial.println("[AtollBle] Server onDisconnect");
}

void BleServer::startAdvertising() {
    if (!enabled) {
        Serial.println("[AtollBle] Not enabled, not starting advertising");
        return;
    }
    delay(300);
    if (!advertising->isAdvertising()) {
        server->startAdvertising();
        Serial.println("[AtollBle] Start advertising");
    }
}

void BleServer::onRead(BLECharacteristic *c) {
    Serial.printf("[AtollBle] %s: onRead(), value: %s\n",
                  characteristicStr(c),
                  c->getValue().c_str());
};

void BleServer::onWrite(BLECharacteristic *c) {
    log_i("char: %s, value: %s", characteristicStr(c), c->getValue().c_str());
};

void BleServer::onNotify(BLECharacteristic *pCharacteristic){
    // Serial.printf("[AtollBle] Sending notification: %d\n", pCharacteristic->getValue<int>());
};

void BleServer::onSubscribe(BLECharacteristic *c, ble_gap_conn_desc *desc, uint16_t subValue) {
    Serial.printf("[AtollBle] Client ID: %d Address: %s ",
                  desc->conn_handle,
                  BLEAddress(desc->peer_ota_addr).toString().c_str());
    if (subValue == 0)
        Serial.print("unsubscribed from ");
    else if (subValue == 1)
        Serial.print("subscribed to notfications for ");
    else if (subValue == 2)
        Serial.print("Subscribed to indications for ");
    else if (subValue == 3)
        Serial.print("subscribed to notifications and indications for ");
    Serial.println(characteristicStr(c));
};

void BleServer::setSecureApi(bool state) {
    if (state == secureApi) return;
    secureApi = state;
    saveSettings();
    Serial.printf("[AtollBle] SecureAPI %sabled\n", secureApi ? "en" : "dis");
    /* TODO deinit() does not return
    stop();
    setup(deviceName, preferences);
    */
}

void BleServer::setPasskey(uint32_t newPasskey) {
    if (newPasskey == passkey) return;
    passkey = newPasskey;
    saveSettings();
    Serial.printf("[AtollBle] New passkey: %d\n", passkey);
    /* TODO deinit() does not return
    stop();
    setup(deviceName, preferences);
    */
}

void BleServer::loadSettings() {
    if (!preferencesStartLoad()) return;
    secureApi = preferences->getBool("secureApi", secureApi);
    passkey = (uint32_t)preferences->getInt("passkey", passkey);
    preferencesEnd();
}

void BleServer::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("secureApi", secureApi);
    preferences->putInt("passkey", (int32_t)passkey);
    preferencesEnd();
}

void BleServer::printSettings() {
    Serial.printf("[AtollBle] SecureAPI: %s\n", secureApi ? "Yes" : "No");
    Serial.printf("[AtollBle] Passkey: %d\n", passkey);
}