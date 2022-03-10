#include "ble.h"

void BLE::setup(const char *deviceName, Preferences *p) {
    strncpy(this->deviceName, deviceName, sizeof(this->deviceName));
    preferencesSetup(p, "BLE");
    loadSettings();
    printSettings();
    enabled = true;
    BLEDevice::init(deviceName);

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

void BLE::startServices() {
    startDiService();
    startBlService();
    startApiService();
}

void BLE::loop() {
    if (!enabled) return;
    if (!advertising->isAdvertising()) startAdvertising();
}

// Start Device Information service
void BLE::startDiService() {
    Serial.println("[BLE] Starting DIS");
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
void BLE::startBlService() {
    Serial.println("[BLE] Starting BLS");
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

void BLE::startApiService() {
    Serial.println("[BLE] Starting APIS");
    char s[SETTINGS_STR_LENGTH] = "";
    asUUID = BLEUUID(API_SERVICE_UUID);
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
void BLE::notifyBl(const ulong t, const uint8_t level) {
    if (!enabled) {
        Serial.println("[BLE] Not enabled, not notifying BL");
        return;
    }

    if (t - 2000 < lastBatteryNotification) return;
    lastBatteryLevel = level;
    lastBatteryNotification = t;
    blChar->setValue(&lastBatteryLevel, 1);
    blChar->notify();
}

const char *BLE::characteristicStr(BLECharacteristic *c) {
    if (c == nullptr) return "unknown characteristic";
    if (blChar != nullptr && blChar->getHandle() == c->getHandle()) return "BL";
    if (apiTxChar != nullptr && apiTxChar->getHandle() == c->getHandle()) return "APITX";
    if (apiRxChar != nullptr && apiRxChar->getHandle() == c->getHandle()) return "APIRX";
    return c->getUUID().toString().c_str();
}

// disconnect clients, stop advertising and shutdown BLE
void BLE::stop() {
    while (!_clients.isEmpty())
        server->disconnect(_clients.shift());
    server->stopAdvertising();
    enabled = false;
    delay(100);  // give the BLE stack a chance to clear packets
    // BLEDevice::deinit(true);  // TODO never returns
}

void BLE::onConnect(BLEServer *pServer, ble_gap_conn_desc *desc) {
    Serial.printf("[BLE] Client connected, ID: %d Address: %s\n",
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

void BLE::onDisconnect(BLEServer *pServer) {
    Serial.println("[BLE] Server onDisconnect");
}

void BLE::startAdvertising() {
    if (!enabled) {
        Serial.println("[BLE] Not enabled, not starting advertising");
        return;
    }
    delay(300);
    if (!advertising->isAdvertising()) {
        server->startAdvertising();
        Serial.println("[BLE] Start advertising");
    }
}

void BLE::onRead(BLECharacteristic *c) {
    Serial.printf("[BLE] %s: onRead(), value: %s\n",
                  characteristicStr(c),
                  c->getValue().c_str());
};

void BLE::onWrite(BLECharacteristic *c) {
    char value[BLE_CHAR_VALUE_MAXLENGTH] = "";
    strncpy(value, c->getValue().c_str(), sizeof(value));
    Serial.printf("[BLE] %s: onWrite(), value: %s\n",
                  characteristicStr(c),
                  value);
    if (c->getHandle() == apiRxChar->getHandle())
        handleApiCommand(value);
};

void BLE::onNotify(BLECharacteristic *pCharacteristic){
    // Serial.printf("[BLE] Sending notification: %d\n", pCharacteristic->getValue<int>());
};

void BLE::onSubscribe(BLECharacteristic *c, ble_gap_conn_desc *desc, uint16_t subValue) {
    Serial.printf("[BLE] Client ID: %d Address: %s ",
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

void BLE::setSecureApi(bool state) {
    if (state == secureApi) return;
    secureApi = state;
    saveSettings();
    Serial.printf("[BLE] SecureAPI %sabled\n", secureApi ? "en" : "dis");
    /* TODO deinit() does not return
    stop();
    setup(deviceName, preferences);
    */
}

void BLE::setPasskey(uint32_t newPasskey) {
    if (newPasskey == passkey) return;
    passkey = newPasskey;
    saveSettings();
    Serial.printf("[BLE] New passkey: %d\n", passkey);
    /* TODO deinit() does not return
    stop();
    setup(deviceName, preferences);
    */
}

void BLE::loadSettings() {
    if (!preferencesStartLoad()) return;
    secureApi = preferences->getBool("secureApi", secureApi);
    passkey = (uint32_t)preferences->getInt("passkey", passkey);
    preferencesEnd();
}

void BLE::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("secureApi", secureApi);
    preferences->putInt("passkey", (int32_t)passkey);
    preferencesEnd();
}

void BLE::printSettings() {
    Serial.printf("[BLE] SecureAPI: %s\n", secureApi ? "Yes" : "No");
    Serial.printf("[BLE] Passkey: %d\n", passkey);
}