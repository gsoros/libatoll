#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_ble.h"
#include "atoll_ble_client.h"
#include "atoll_peer.h"

using namespace Atoll;

Peer::Peer(Saved saved,
           PeerCharacteristicBattery* customBattChar) {
    if (strlen(saved.address) < 1) {
        log_e("empty address for %s %s", saved.name, saved.type);
        snprintf(this->saved.address, sizeof(this->saved.address), "%s", "invalid");
    } else
        strncpy(this->saved.address, saved.address, sizeof(this->saved.address));
    this->saved.addressType = saved.addressType;
    strncpy(this->saved.type, saved.type, sizeof(this->saved.type));
    strncpy(this->saved.name, saved.name, sizeof(this->saved.name));
    this->saved.passkey = saved.passkey;
    for (int8_t i = 0; i < charsMax; i++)
        chars[i] = nullptr;
    addChar(nullptr != customBattChar
                ? customBattChar
                : new PeerCharacteristicBattery());
}

void Peer::loop() {
    if (connParamsProfile != APCPP_ESTABLISHED && isConnected()) {
        connParamsProfile = APCPP_ESTABLISHED;
        log_d("%s setting conn params", saved.name);
        setConnectionParams(connParamsProfile);
    }
}

// format: address,addressType,type,name[,passkey]
bool Peer::pack(char* packed, size_t len, bool includePasskey) {
    char buf[packedMaxLength];
    if (includePasskey)
        snprintf(buf, sizeof(buf), "%s,%d,%s,%s,%d",
                 saved.address,
                 saved.addressType,
                 saved.type,
                 saved.name,
                 saved.passkey);
    else
        snprintf(buf, sizeof(buf), "%s,%d,%s,%s",
                 saved.address,
                 saved.addressType,
                 saved.type,
                 saved.name);
    size_t s = strlen(buf);
    if (packedMaxLength < s) {
        log_e("packed longer than allowed");
        return false;
    }
    if (len < s) {
        log_e("buffer too small");
        return false;
    }
    strncpy(packed, buf, len);
    return true;
}

// format: address,addressType,type,name,passkey
bool Peer::unpack(
    const char* packed,
    Saved* saved) {
    size_t packedLen = strnlen(packed, packedMaxLength);
    log_d("unpacking '%s'(%d)", packed, packedLen);

    Saved temp;
    bool addressFound = false;
    char tAddressTypeStr[4] = "";  // strlen(itoa(UINT8_MAX)) + 1
    bool addressTypeFound = false;
    bool typeFound = false;
    bool nameFound = false;
    char tPasskeyStr[7] = "";

    for (const char* cur = &packed[0]; cur < packed + packedLen; cur++) {
        // log_d("processing '%c'", *cur);
        if (*cur == ',') {
            if (!addressFound) {
                addressFound = true;
                continue;
            }
            if (!addressTypeFound) {
                addressTypeFound = true;
                continue;
            }
            if (!typeFound) {
                typeFound = true;
                continue;
            }
            if (!nameFound) {
                nameFound = true;
                continue;
            }
            log_e("too many colons");
            return false;
        }
        if (!addressFound) {
            if (sizeof(temp.address) <= strlen(temp.address) + 1) {
                log_e("address too long");
                return false;
            }
            strncat(temp.address, cur, 1);
            continue;
        }
        if (!addressTypeFound) {
            if (sizeof(tAddressTypeStr) <= strlen(tAddressTypeStr) + 1) {
                log_e("address type too long");
                return false;
            }
            strncat(tAddressTypeStr, cur, 1);
            continue;
        }
        if (!typeFound) {
            if (sizeof(temp.type) <= strlen(temp.type) + 1) {
                log_e("type too long");
                return false;
            }
            strncat(temp.type, cur, 1);
            continue;
        }
        if (!nameFound) {
            if (sizeof(temp.name) <= strlen(temp.name) + 1) {
                log_e("name too long");
                return false;
            }
            strncat(temp.name, cur, 1);
            continue;
        }
        if (sizeof(tPasskeyStr) <= strlen(tPasskeyStr) + 1) {
            log_e("passkey too long");
            return false;
        }
        strncat(tPasskeyStr, cur, 1);
    }

    if (strlen(temp.address) < 1) {
        log_e("address too short");
        return false;
    }
    if (strlen(tAddressTypeStr) < 1) {
        log_e("address type too short");
        return false;
    }
    int tAddressTypeInt = atoi(tAddressTypeStr);
    if (tAddressTypeInt < 0 || UINT8_MAX < tAddressTypeInt) {
        log_e("address type out of range");
        return false;
    }
    if (strlen(temp.type) < 1) {
        log_e("type too short");
        return false;
    }
    if (strlen(temp.name) < 1) {
        log_e("name too short");
        return false;
    }
    int tPasskeyInt = atoi(tPasskeyStr);
    if (tPasskeyInt < 0 || 999999 < tPasskeyInt) {
        log_e("passkey out of range");
        return false;
    }

    strncpy(saved->address, temp.address, sizeof(saved->address));
    saved->addressType = (uint8_t)tAddressTypeInt;
    strncpy(saved->type, temp.type, sizeof(saved->type));
    strncpy(saved->name, temp.name, sizeof(saved->name));
    saved->passkey = (uint32_t)tPasskeyInt;

    log_d("address: %s, addressType: %d, type: %s, name: %s, passkey: %d",
          saved->address, saved->addressType, saved->type, saved->name, saved->passkey);
    return true;
}

Peer::~Peer() {
    disconnect();
    // log_i("calling deleteClient() for %s", name);
    // deleteClient();
    unsetClient();
}

void Peer::setConnectionParams(uint8_t profile) {
    // minInterval  | The minimum connection interval in 1.25ms units. (6 - 3200)
    // maxInterval  | The maximum connection interval in 1.25ms units. (6 - 3200)
    // latency      | The number of packets allowed to skip (extends max interval). (0 - 499)
    // timeout      | The timeout time in 10ms units before disconnecting. (10 - 3200)
    //
    //       2 * maxInterval_ms       * (1 + latency) < timeout_ms
    //       2 * maxInterval * 1.25ms * (1 + latency) < timeout * 10ms

    // initial connection params
    uint16_t minInterval = (uint16_t)(10 / 1.25);   // 10 ms
    uint16_t maxInterval = (uint16_t)(100 / 1.25);  // 100 ms
    uint16_t latency = 0;                           //
    uint16_t timeout = (uint16_t)(500 / 10);        // 500 ms

    if (profile == APCPP_ESTABLISHED) {
        minInterval = (uint16_t)(500 / 1.25);   // 500 ms
        maxInterval = (uint16_t)(1000 / 1.25);  // 1000 ms
        latency = 1;                            //
        timeout = (uint16_t)(5000 / 10);        // 5000 ms
    }

    log_d("%s %s: interval(%d-%d), latency %d, timeout %d",
          saved.name, profile == APCPP_ESTABLISHED ? "established" : "initial",
          minInterval, maxInterval, latency, timeout);

    BLEClient* client = getClient();
    if (nullptr == client) {
        log_e("%s client is null", saved.name);
        return;
    }
    if (client->isConnected()) {
        client->updateConnParams(minInterval, maxInterval, latency, timeout);
        return;
    }
    client->setConnectionParams(minInterval, maxInterval, latency, timeout);
}

void Peer::connect() {
    if (isConnected()) {
        log_d("%s already connected", saved.name);
        return;
    }
    if (connecting) {
        log_d("%s already connecting", saved.name);
        return;
    }
    connecting = true;

    log_d("%s connecting", saved.name);

    BLEClient* c = nullptr;

    if (hasClient()) {
        log_d("%s has client", saved.name);
        goto connect;
    }

    c = BLEDevice::getClientByPeerAddress(BLEAddress(saved.address, saved.addressType));
    if (c) {
        log_d("%s got client by peer address", saved.name);
        goto set;
    }
    c = BLEDevice::getDisconnectedClient();
    if (c) {
        log_d("%s got disconnected client", saved.name);
        goto set;
    }
    if (BLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
        log_e("%s max clients reached", saved.name);
        goto fail;
    }
    c = BLEDevice::createClient(BLEAddress(saved.address, saved.addressType));
    if (c) {
        log_d("%s created new client", saved.name);
        goto set;
    }

set:
    if (!c) {
        log_e("%s could not get client", saved.name);
        goto fail;
    }
    setClient(c);

connect:
    lastConnectionAttempt = millis();
    connParamsProfile = APCPP_INITIAL;
    setConnectionParams(connParamsProfile);
    client->setConnectTimeout(2000);
    if (!connectClient()) {
        log_d("%s failed to connect client", saved.name);
        goto fail;
    }
    log_i("%s connected", saved.name);
    goto end;

fail:
    unsetClient();

end:
    // log_d("end");
    connecting = false;
}

void Peer::disconnect() {
    shouldConnect = false;
    if (isConnected()) {
        // setConnectionParams(APCPP_INITIAL);  // speed up unsub // E NimBLEDevice: ble_gap_security_initiate: rc=19 HCI request timed out; controller unresponsive.
        // unsubscribeChars(client);
        client->disconnect();
    }
    // while (isConnected()) {
    //     log_i("%s waiting for disconnect...", saved.name);
    //     delay(500);
    // }
}

bool Peer::setClient(BLEClient* client) {
    if (nullptr == client) {
        log_e("client is null");
        return false;
    }
    this->client = client;
    client->setClientCallbacks(this, false);
    return true;
}

BLEClient* Peer::getClient() {
    return client;
}

void Peer::unsetClient() {
    client = nullptr;
}

bool Peer::hasClient() {
    return nullptr != client;
}

bool Peer::isConnected() {
    return hasClient() && client->isConnected();
}

bool Peer::connectClient(bool deleteAttributes) {
    return hasClient() && client->connect(BLEAddress(saved.address, saved.addressType), deleteAttributes);
}

void Peer::subscribeChars(BLEClient* client) {
    if (!isConnected()) {
        log_e("not connected");
        return;
    }
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            if (!chars[i]->subscribeOnConnect()) {
                log_i("%s not subscribing %s", saved.name, chars[i]->label);
                chars[i]->getRemoteChar(client);  // get remote service and char
                continue;
            }
            log_i("%s subscribing %s", saved.name, chars[i]->label);
            chars[i]->subscribe(client);
        }
}

void Peer::unsubscribeChars(BLEClient* client) {
    if (!isConnected()) {
        log_e("not connected");
        return;
    }
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            log_i("%s unsubscribing %s", saved.name, chars[i]->label);
            chars[i]->unsubscribe(client);
        }
}

// get index of existing characteristic by label
// or first unused index for an empty label
// returns -1 on error
int8_t Peer::charIndex(const char* label) {
    size_t len = strlen(label);
    // log_i("checking char label '%s' len: %d", label, len);
    for (int8_t i = 0; i < charsMax; i++) {
        if (nullptr == chars[i] && !len) {
            // log_i("found unused index %d for empty label", i);
            return i;
        }
        if (len && nullptr != chars[i] && 0 == strcmp(chars[i]->label, label)) {
            // log_i("found index %d of char '%s'", i, label);
            return i;
        }
    }
    // log_i("could not find index of char '%s'", label);
    return -1;
}

// chack if a char with label exists
bool Peer::charExists(const char* label) {
    return 0 <= charIndex(label);
}

// add new characteristic
bool Peer::addChar(PeerCharacteristic* c) {
    if (nullptr != c->peer) {
        log_e("%s %s already has a peer, not adding", saved.name, c->label);
        return false;
    }
    int8_t index = charIndex(c->label);
    if (index < 0) index = charIndex("");
    if (index < 0) {
        log_e("cannot add char '%s'", c->label);
        return false;
    }
    // log_i("adding char label: '%s', uuid: '%s'", c->label, c->charUuid.toString().c_str());
    c->peer = this;
    chars[index] = c;
    return true;
}

// get first characteristic pointer with label
PeerCharacteristic* Peer::getChar(const char* label) {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i] && 0 == strcmp(chars[i]->label, label))
            return chars[i];
    return nullptr;
}

// remove char at index
bool Peer::removeCharAt(int8_t index) {
    if (charsMax <= index) return false;
    if (nullptr != chars[index]) chars[index]->peer = nullptr;
    chars[index] = nullptr;
    return true;
}

// delete chars that have this label
uint8_t Peer::deleteChars(const char* label) {
    uint8_t deleted = 0;
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i] && 0 == strcmp(chars[i]->label, label)) {
            delete chars[i];  // delete nullptr should be safe!
            removeCharAt(i);
            deleted++;
        }
    return deleted;
}

bool Peer::isPowerMeter() {
    return nullptr != strchr(saved.type, 'P');
}

bool Peer::isESPM() {
    return nullptr != strchr(saved.type, 'E');
}

bool Peer::isHeartrateMonitor() {
    return nullptr != strchr(saved.type, 'H');
}

bool Peer::isVesc() {
    return nullptr != strchr(saved.type, 'V');
}

void Peer::onConnect(BLEClient* client) {
    log_d("%s connected", saved.name);

    log_d("%s discovering attributes...", saved.name);
    client->discoverAttributes();

    // log_d("%s subscribing...", name);
    subscribeChars(client);

    // log_d("%s requesting conn param update...", saved.name);
    // setConnectionParams(client, APCPP_ESTABLISHED);
}

/**
 * @brief Called when disconnected from the server.
 * @param [in] device->client A pointer to the calling client object.
 */
void Peer::onDisconnect(BLEClient* client, int reason) {
    log_i("%s disconnected, reason %d", saved.name, reason);
}

/**
 * @brief Called when server requests to update the connection parameters.
 * @param [in] device->client A pointer to the calling client object.
 * @param [in] params A pointer to the struct containing the connection parameters requested.
 * @return True to accept the parmeters.
 */
bool Peer::onConnParamsUpdateRequest(BLEClient* client, const ble_gap_upd_params* params) {
    log_i("%s requested: interval: %d-%d, latency: %d, ce: %d-%d, timeout: %d",
          saved.name,
          params->itvl_min,
          params->itvl_max,
          params->latency,
          params->min_ce_len,
          params->max_ce_len,
          params->supervision_timeout);
    log_i("%s accepting request", saved.name);
    return true;
}

/**
 * @brief Called when server requests a passkey for pairing.
 * @return The passkey to be sent to the server.
 */
uint32_t Peer::onPassKeyRequest() {
    // log_w("%s sending 696669 instead of saved %d, TODO send saved passkey", saved.name, saved.passkey);
    // return 696669;
    log_d("%s sending saved %d", saved.name, saved.passkey);
    return saved.passkey;
}

/*
void Peer::onPassKeyNotify(uint32_t pass_key);
bool Peer::onSecurityRequest() {}
*/

/**
 * @brief Called when the pairing procedure is complete.
 * @param [in] desc A pointer to the struct containing the connection information.\n
 * This can be used to check the status of the connection encryption/pairing.
 */
void Peer::onAuthenticationComplete(NimBLEConnInfo& info) {
    log_i("%s", Ble::connInfoToStr(&info).c_str());
}

/**
 * @brief Called when using numeric comparision for pairing.
 * @param [in] pin The pin to compare with the server.
 * @return True to accept the pin.
 */
bool Peer::onConfirmPIN(uint32_t pin) {
    log_i("%s TODO", saved.name);
    return true;
}

void Peer::onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) {
    char buf[length];
    strncpy(buf, (char*)data, length);
    log_d("%s uuid: %s, data: '%s', len: %d", saved.name, c->getUUID().toString().c_str(), buf, length);
}

PowerMeter::PowerMeter(
    Saved saved,
    PeerCharacteristicPower* customPowerChar,
    PeerCharacteristicBattery* customBattChar)
    : Peer(
          saved,
          customBattChar) {
    addChar(nullptr != customPowerChar
                ? customPowerChar
                : new PeerCharacteristicPower());
}

ESPM::ESPM(
    Saved saved,
    PeerCharacteristicPower* customPowerChar,
    PeerCharacteristicBattery* customBattChar,
    PeerCharacteristicApiTX* customApiTxChar,
    PeerCharacteristicApiRX* customApiRxChar,
    PeerCharacteristicWeightscale* customWeightChar)
    : PowerMeter(
          saved,
          customPowerChar,
          customBattChar) {
    addChar(nullptr != customApiTxChar
                ? customApiTxChar
                : new PeerCharacteristicApiTX());
    addChar(nullptr != customApiRxChar
                ? customApiRxChar
                : new PeerCharacteristicApiRX());
    addChar(nullptr != customWeightChar
                ? customWeightChar
                : new PeerCharacteristicWeightscale());
}

void ESPM::loop() {
    PeerCharacteristicApiTX* apiTx = (PeerCharacteristicApiTX*)getChar("ApiTX");
    if (nullptr == apiTx)
        log_e("%s apiTx is null", saved.name);
    else
        apiTx->loop();
    PowerMeter::loop();
}

void ESPM::onConnect(BLEClient* client) {
    PowerMeter::onConnect(client);
    if (!sendApiCommand("init"))
        log_e("%s could not send init request", saved.name);
}

bool ESPM::sendApiCommand(const char* command) {
    PeerCharacteristicApiRX* apiRX = (PeerCharacteristicApiRX*)getChar("ApiRX");
    if (nullptr == apiRX) {
        log_e("api rx char is null");
        return false;
    }
    if (!hasClient()) {
        log_e("no client");
        return false;
    }
    log_d("%s sending command '%s'", saved.name, command);
    auto sc = String(command);
    if (!apiRX->write(client, sc, sc.length())) {
        log_e("%s could not write char", saved.name);
        return false;
    }
    log_d("%s command sent", saved.name);
    return true;
}

HeartrateMonitor::HeartrateMonitor(
    Saved saved,
    PeerCharacteristicHeartrate* customHrChar,
    PeerCharacteristicBattery* customBattChar)
    : Peer(saved,
           customBattChar) {
    addChar(nullptr != customHrChar
                ? customHrChar
                : new PeerCharacteristicHeartrate());
}

Vesc::Vesc(Saved saved,
           PeerCharacteristicVescRX* customVescRX,
           PeerCharacteristicVescTX* customVescTX)
    : Peer(saved) {
    deleteChars("Battery");
    addChar(nullptr != customVescRX
                ? customVescRX
                : new PeerCharacteristicVescRX());
    PeerCharacteristicVescTX* vescTX = nullptr != customVescTX
                                           ? customVescTX
                                           : new PeerCharacteristicVescTX();
    uart = new VescUart(250);  // 250 ms timeout on waiting for reply
    uartBleStream = new VescUartBleStream();
    vescTX->stream = uartBleStream;
    addChar(vescTX);
    uart->setSerialPort(uartBleStream);
    uartBleStream->vesc = this;
}

void Vesc::setConnectionParams(uint8_t profile) {
    if (APCPP_ESTABLISHED == profile) {
        log_i("%s not setting established profile", saved.name);
        return;
    }
    Peer::setConnectionParams(profile);
}

void Vesc::loop() {
    Peer::loop();
    requestUpdate();
    log_i("%s %2.2fV %dW", saved.name, getVoltage(), getPower());
}

bool Vesc::requestUpdate() {
    static ulong lastUpdate = 0;
    static bool pending = false;
    if (pending) {
        log_e("already pending");
        return false;
    }
    ulong start = millis();
    if (start < lastUpdate + 2000) {
        log_i("%s skipping update", saved.name);
        return false;
    }
    pending = true;
    bool res = uart->getVescValues();
    ulong end = millis();
    if (res) lastUpdate = end;
    log_d("update took %d ms", end - start);
    pending = false;
    return res;
}

float Vesc::getVoltage() {
    return 0.0f < uart->data.inpVoltage && uart->data.inpVoltage < 100.0f ? uart->data.inpVoltage : 0.0f;
}

uint16_t Vesc::getPower() {
    float voltage = getVoltage();
    if (voltage <= 0.01f) {
        log_e("voltage is 0");
        return 0;
    }
    float current = uart->data.avgInputCurrent;
    if (current <= 0.01f)
        current = 0.0f;
    else if (50.0f < current)
        current = 50.0f;
    float power = voltage * current;
    if (power < 0.0f)
        power = 0.0f;
    else if (UINT16_MAX < power)
        power = (float)UINT16_MAX;
    return (uint16_t)power;
}

void Vesc::setPower(uint16_t power) {
    log_d("not implemented");
}

#endif