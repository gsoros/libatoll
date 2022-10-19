#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_ble.h"
#include "atoll_ble_client.h"
#include "atoll_peer.h"

using namespace Atoll;

Peer::~Peer() {
    disconnect();
    // log_i("calling deleteClient() for %s", name);
    // deleteClient();
    unsetClient();
}

void Peer::setConnectionParams(BLEClient* client, uint8_t profile, bool update) {
    // minInterval  | The minimum connection interval in 1.25ms units. (6 - 3200)
    // maxInterval  | The maximum connection interval in 1.25ms units. (6 - 3200)
    // latency      | The number of packets allowed to skip (extends max interval). (0 - 499)
    // timeout      | The timeout time in 10ms units before disconnecting. (10 - 3200)
    // scanInterval | The scan interval to use when attempting to connect in 0.625ms units.
    // scanWindow   | The scan window to use when attempting to connect in 0.625ms units.
    //      if (maxinterval * latency > timeout) { return invalidParams; )
    switch (profile) {
        case APCPP_ESTABLISHED:
            log_i("setting established connection params");
            if (update)
                client->updateConnParams(128, 128, 0, 42);
            else
                client->setConnectionParams(128, 128, 0, 42);
            break;
        case APCPP_INITIAL:
        default:
            log_i("setting initial connection params");
            if (update)
                client->updateConnParams(6, 12, 0, 42);
            else
                client->setConnectionParams(6, 12, 0, 42);
    }
}

void Peer::connect() {
    // https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino
    if (connecting) {
        log_i("%s already connecting", name);
        return;
    }
    connecting = true;

    if (isConnected()) {
        log_i("%s already connected", name);
        goto end;
    }
    log_i("%s connecting", name);
    if (BLEDevice::getClientListSize()) {
        setClient(BLEDevice::getClientByPeerAddress(BLEAddress(address, addressType)));
        if (hasClient()) {
            setConnectionParams(getClient(), APCPP_INITIAL);
            if (!connectClient(false)) {
                // log_i("%s reconnect failed", name);
                goto end;
            }
            log_i("%s reconnected", name);
        } else
            setClient(BLEDevice::getDisconnectedClient());
    }
    if (!hasClient()) {
        if (BLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            log_e("%s max clients reached", name);
            goto end;
        }
        BLEClient* c = BLEDevice::createClient(BLEAddress(address, addressType));
        log_i("%s new client created, setting conn params", name);

        setConnectionParams(c, APCPP_INITIAL);
        c->setConnectTimeout(2000);

        setClient(c);

        if (!connectClient()) {
            // log_i("%s failed to connect new client", name);
            goto end;
        }
    }
    if (!isConnected()) {
        if (!connectClient()) {
            // log_i("%s failed to connect", name);
            goto end;
        }
    }
    log_i("%s connected", name);

    // log_i("%s calling secureConnection()...", name);
    // if (!client->secureConnection()) {
    //     log_i("%s secureConnection() failed", name);
    // }

    goto end;

end:
    // log_i("end");
    connecting = false;
}

void Peer::subscribeChars(BLEClient* client) {
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

    log_i("%s subscribing...", name);
    subscribeChars(client);

    log_i("%s requesting conn param update...", name);
    setConnectionParams(client, APCPP_ESTABLISHED, true);
}

/**
 * @brief Called when disconnected from the server.
 * @param [in] device->client A pointer to the calling client object.
 */
void Peer::onDisconnect(BLEClient* client, int reason) {
    log_i("%s disconnected, reason %d, unsubscribing", name, reason);
    unsubscribeChars(client);
    unsetClient();
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
    log_i("%s uuid: %s, data: '%s', len: %d", name, c->getUUID().toString().c_str(), buf, length);
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
    // log_i("calling secureConnection()");
    // client->secureConnection();
    PowerMeter::onConnect(client);
}

bool ESPM::sendApiCommand(const char* command) {
    PeerCharacteristicApiRX* apiRX = (PeerCharacteristicApiRX*)getChar("ApiRX");
    if (nullptr == apiRX) {
        log_e("api rx char is null");
        return false;
    }
    BLEClient* client = getClient();
    if (nullptr == client) {
        log_e("client is null");
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

#endif