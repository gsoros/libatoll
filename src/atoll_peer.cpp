#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_ble.h"
#include "atoll_ble_client.h"
#include "atoll_peer.h"

using namespace Atoll;

Peer::Peer(const char* address,
           uint8_t addressType,
           const char* type,
           const char* name,
           PeerCharacteristicBattery* customBattChar) {
    if (strlen(address) < 1) {
        log_e("empty address for %s %s", name, type);
        snprintf(this->address, sizeof(this->address), "%s", "invalid");
    } else
        strncpy(this->address, address, sizeof(this->address));
    this->addressType = addressType;
    strncpy(this->type, type, sizeof(this->type));
    strncpy(this->name, name, sizeof(this->name));
    for (int8_t i = 0; i < charsMax; i++)
        removeCharAt(i);  // initialize to nullptrs
    addChar(nullptr != customBattChar
                ? customBattChar
                : new PeerCharacteristicBattery());
}

// format: address,addressType,type,name
bool Peer::pack(char* packed, size_t len) {
    char buf[packedMaxLength];
    snprintf(buf, sizeof(buf), "%s,%d,%s,%s", address, addressType, type, name);
    if (len < strlen(buf)) {
        log_e("buffer too small");
        return false;
    }
    strncpy(packed, buf, len);
    return true;
}

// format: address,addressType,type,name
bool Peer::unpack(
    const char* packed,
    char* address,
    size_t addressLen,
    uint8_t* addressType,
    char* type,
    size_t typeLen,
    char* name,
    size_t nameLen) {
    // log_i("unpacking '%s'", packed);
    size_t packedLen = strlen(packed);
    // log_i("packedLen: %d", packedLen);
    if (packedLen < sizeof(Peer::address) + 7) {
        log_e("packed string too short (%d)", packedLen);
        return false;
    }
    char* colon = strchr(packed, ',');
    if (nullptr == colon) {
        log_e("first colon not present");
        return false;
    }
    char tAddress[sizeof(Peer::address)] = "";
    size_t tAddressLen = colon - packed;
    // log_i("tAddressLen: %d", tAddressLen);
    if (addressLen < tAddressLen) {
        log_e("address buffer too small");
        return false;
    }
    strncpy(tAddress, packed, tAddressLen);

    // rest of packed, without address and colon
    char rest[packedLen - tAddressLen] = "";
    strncpy(rest, colon + 1, sizeof(rest) - 1);
    // log_i("rest: '%s' size: %d", rest, sizeof(rest) - 1);
    colon = strchr(rest, ',');
    if (nullptr == colon) {
        log_e("second colon not present");
        return false;
    }
    char tAddressTypeStr[3] = "";  // strlen(uint8_t)
    strncpy(tAddressTypeStr, rest, colon - rest);
    uint8_t tAddressTypeLen = strlen(tAddressTypeStr);
    uint8_t tAddressType = atoi(tAddressTypeStr);
    // log_i("tAddressType: %d", tAddressType);

    colon = strrchr(packed, ',');
    if (nullptr == colon) {
        log_e("last colon not present");
        return false;
    }
    char tType[sizeof(Peer::type)] = "";
    size_t tTypeLen = colon - tAddressLen - tAddressTypeLen - 2 - packed;
    // log_i("tTypeLen: %d", tTypeLen);
    if (typeLen < tTypeLen) {
        log_e("type buffer too small");
        return false;
    }
    strncpy(tType, packed + tAddressTypeLen + tAddressLen + 2, tTypeLen);
    char tName[sizeof(Peer::name)] = "";
    size_t tNameLen = packedLen - tAddressLen - tAddressTypeLen - tTypeLen - 3;
    // log_i("tNameLen: %d", tNameLen);
    if (nameLen < tNameLen) {
        log_e("name buffer too small");
        return false;
    }
    strncpy(tName, packed + tAddressLen + tAddressTypeLen + tTypeLen + 3, tNameLen);
    // log_i("tAddress: %s, tAddressType: %d, tType: %s, tName: %s",
    //       tAddress, tAddressType, tType, tName);
    strncpy(address, tAddress, addressLen);
    *addressType = tAddressType;
    strncpy(type, tType, typeLen);
    strncpy(name, tName, nameLen);
    // log_i("address: %s, addressType: %d, type: %s, name: %s",
    //       address, *addressType, type, name);
    return true;
}

Peer::~Peer() {
    disconnect();
    // log_i("calling deleteClient() for %s", name);
    // deleteClient();
    unsetClient();
}

void Peer::setConnectionParams(BLEClient* client, uint8_t profile) {
    // minInterval  | The minimum connection interval in 1.25ms units. (6 - 3200)
    // maxInterval  | The maximum connection interval in 1.25ms units. (6 - 3200)
    // latency      | The number of packets allowed to skip (extends max interval). (0 - 499)
    // timeout      | The timeout time in 10ms units before disconnecting. (10 - 3200)
    // scanInterval | The scan interval to use when attempting to connect in 0.625ms units.
    // scanWindow   | The scan window to use when attempting to connect in 0.625ms units.
    //      if (maxinterval * latency > timeout) { return invalidParams; )
    switch (profile) {
        case APCPP_ESTABLISHED:
            if (client->isConnected()) {
                log_i("updating to established profile");
                client->updateConnParams(128, 128, 2, 256);
            } else {
                // log_i("setting to established profile");
                client->setConnectionParams(128, 128, 2, 256);
            }
            break;
        case APCPP_INITIAL:
        default:
            if (client->isConnected()) {
                log_i("updating to initial profile");
                client->updateConnParams(6, 12, 2, 128);
            } else {
                // log_i("setting to initial profile");
                client->setConnectionParams(6, 12, 2, 128);
            }
    }
}

void Peer::connect() {
    if (isConnected()) {
        log_i("%s already connected", name);
        return;
    }
    if (connecting) {
        log_i("%s already connecting", name);
        return;
    }
    connecting = true;

    // log_i("%s connecting", name);

    BLEClient* c = nullptr;

    if (hasClient()) {
        // log_i("%s has client", name);
        goto connect;
    }

    c = BLEDevice::getClientByPeerAddress(BLEAddress(address, addressType));
    if (c) {
        // log_i("%s got client by peer address", name);
        goto set;
    }
    c = BLEDevice::getDisconnectedClient();
    if (c) {
        // log_i("%s got disconnected client", name);
        goto set;
    }
    if (BLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
        log_e("%s max clients reached", name);
        goto fail;
    }
    c = BLEDevice::createClient(BLEAddress(address, addressType));
    if (c) {
        // log_i("%s created new client", name);
        goto set;
    }

set:
    if (!c) {
        log_e("%s could not get client", name);
        goto fail;
    }
    setClient(c);

connect:
    setConnectionParams(client, APCPP_INITIAL);
    client->setConnectTimeout(2000);
    if (!connectClient()) {
        // log_i("%s failed to connect client", name);
        goto fail;
    }
    log_i("%s connected", name);
    goto end;

fail:
    unsetClient();

end:
    // log_i("end");
    connecting = false;
}

void Peer::disconnect() {
    shouldConnect = false;
    if (isConnected()) {
        unsubscribeChars(client);
        client->disconnect();
    }
    while (isConnected()) {
        log_i("%s waiting for disconnect...", name);
        delay(500);
    }
}

void Peer::setClient(BLEClient* client) {
    if (nullptr == client) {
        log_e("client is null");
        return;
    }
    client->setClientCallbacks(this, false);
    this->client = client;
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
    return hasClient() && client->connect(BLEAddress(address, addressType), deleteAttributes);
}

void Peer::subscribeChars(BLEClient* client) {
    if (!isConnected()) {
        log_e("not connected");
        return;
    }
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            if (!chars[i]->subscribeOnConnect()) {
                log_i("%s not subscribing %s", name, chars[i]->label);
                chars[i]->getRemoteChar(client);  // get remote service and char
                continue;
            }
            log_i("%s subscribing %s", name, chars[i]->label);
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
            log_i("%s unsubscribing %s", name, chars[i]->label);
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
    int8_t index = charIndex(c->label);
    if (index < 0) index = charIndex("");
    if (index < 0) {
        log_e("cannot add char '%s'", c->label);
        return false;
    }
    // log_i("adding char label: '%s', uuid: '%s'", c->label, c->charUuid.toString().c_str());
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
    return nullptr != strchr(type, 'P');
}

bool Peer::isESPM() {
    return nullptr != strchr(type, 'E');
}

bool Peer::isHeartrateMonitor() {
    return nullptr != strchr(type, 'H');
}

/**
 * @brief Called after client connects.
 * @param [in] device->client A pointer to the calling client object.
 */
void Peer::onConnect(BLEClient* client) {
    log_i("%s connected", name);

    log_i("%s discovering attributes...", name);
    client->discoverAttributes();

    // log_i("%s subscribing...", name);
    subscribeChars(client);

    // log_i("%s requesting conn param update...", name);
    setConnectionParams(client, APCPP_ESTABLISHED);
}

/**
 * @brief Called when disconnected from the server.
 * @param [in] device->client A pointer to the calling client object.
 */
void Peer::onDisconnect(BLEClient* client, int reason) {
    log_i("%s disconnected, reason %d", name, reason);

    // log_i("%s unsubscribing", name);
    // unsubscribeChars(client);

    // log_i("%s unsetting client", name);
    // unsetClient();
}

/**
 * @brief Called when server requests to update the connection parameters.
 * @param [in] device->client A pointer to the calling client object.
 * @param [in] params A pointer to the struct containing the connection parameters requested.
 * @return True to accept the parmeters.
 */
bool Peer::onConnParamsUpdateRequest(BLEClient* client, const ble_gap_upd_params* params) {
    log_i("%s requested: interval: %d-%d, latency: %d, ce: %d-%d, timeout: %d",
          name,
          params->itvl_min,
          params->itvl_max,
          params->latency,
          params->min_ce_len,
          params->max_ce_len,
          params->supervision_timeout);
    log_i("%s accepting request", name);
    return true;
}

/**
 * @brief Called when server requests a passkey for pairing.
 * @return The passkey to be sent to the server.
 */
uint32_t Peer::onPassKeyRequest() {
    log_w("%s sending 696669, TODO send saved passkey", name);
    return 696669;
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
    log_i("%s TODO", name);
    return true;
}

void Peer::onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) {
    char buf[length];
    strncpy(buf, (char*)data, length);
    log_d("%s uuid: %s, data: '%s', len: %d", name, c->getUUID().toString().c_str(), buf, length);
}

PowerMeter::PowerMeter(
    const char* address,
    uint8_t addressType,
    const char* type,
    const char* name,
    PeerCharacteristicPower* customPowerChar,
    PeerCharacteristicBattery* customBattChar)
    : Peer(
          address,
          addressType,
          type,
          name,
          customBattChar) {
    addChar(nullptr != customPowerChar
                ? customPowerChar
                : new PeerCharacteristicPower());
}

ESPM::ESPM(
    const char* address,
    uint8_t addressType,
    const char* type,
    const char* name,
    PeerCharacteristicPower* customPowerChar,
    PeerCharacteristicBattery* customBattChar,
    PeerCharacteristicApiTX* customApiTxChar,
    PeerCharacteristicApiRX* customApiRxChar,
    PeerCharacteristicWeightscale* customWeightChar)
    : PowerMeter(
          address,
          addressType,
          type,
          name,
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

void ESPM::onConnect(BLEClient* client) {
    PowerMeter::onConnect(client);
    sendApiCommand("init");  // request init
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
    log_i("sending command '%s'", command);
    auto sc = String(command);
    if (!apiRX->write(client, sc, sc.length())) {
        log_i("could not write char");
        return false;
    }
    log_i("command sent");
    return true;
}

HeartrateMonitor::HeartrateMonitor(
    const char* address,
    uint8_t addressType,
    const char* type,
    const char* name,
    PeerCharacteristicHeartrate* customHrChar,
    PeerCharacteristicBattery* customBattChar)
    : Peer(
          address,
          addressType,
          type,
          name,
          customBattChar) {
    addChar(nullptr != customHrChar
                ? customHrChar
                : new PeerCharacteristicHeartrate());
}
#endif