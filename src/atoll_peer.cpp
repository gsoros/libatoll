#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_ble_client.h"
#include "atoll_peer.h"

using namespace Atoll;

Peer::~Peer() {
    disconnect();
    // log_i("calling deleteClient() for %s", name);
    // deleteClient();
    unsetClient();
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
        // c->setConnectionParams(120, 120, 0, 60);
        // c->setConnectionParams(32, 160, 0, 500);
        // c->setConnectionParams(24, 48, 0, 60);
        setClient(c);
        log_i("%s new client created", name);
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
    goto end;

end:
    // log_i("end");
    connecting = false;
}

void Peer::subscribeChars(bool onConnect) {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            if (onConnect && !chars[i]->subscribeOnConnect()) {
                log_i("%s not subscribing %s", name, chars[i]->label);
                // chars[i]->getRemoteChar(client);  // get remote service and char
                continue;
            }
            log_i("%s subscribing %s", name, chars[i]->label);
            chars[i]->subscribe(client);
        }
}

void Peer::unsubscribeChars() {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            log_i("%s unsubscribing %s", name, chars[i]->label);
            chars[i]->unsubscribe();
            chars[i]->unsetRemoteChar();
            chars[i]->unsetRemoteService();
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

/**
 * @brief Called after client connects.
 * @param [in] device->client A pointer to the calling client object.
 */
void Peer::onConnect(BLEClient* client) {
    connected = true;
    log_i("%s connected", name);
    // log_i("%s connected, requesting conn params...", name);
    // client->updateConnParams(120, 120, 0, 60);
    // client->updateConnParams(32, 160, 0, 500);
    log_i("%s subscribing...", name);
    subscribeChars();
}

/**
 * @brief Called when disconnected from the server.
 * @param [in] device->client A pointer to the calling client object.
 */
void Peer::onDisconnect(BLEClient* client) {
    connected = false;
    log_i("%s disconnected, unsubscribing", name);
    unsubscribeChars();
}

/**
 * @brief Called when server requests to update the connection parameters.
 * @param [in] device->client A pointer to the calling client object.
 * @param [in] params A pointer to the struct containing the connection parameters requested.
 * @return True to accept the parmeters.
 */
bool Peer::onConnParamsUpdateRequest(BLEClient* client, const ble_gap_upd_params* params) {
    log_i("%s, interval: %d-%d, latency: %d, ce: %d-%d, timeout: %d",
          name,
          params->itvl_min,
          params->itvl_max,
          params->latency,
          params->min_ce_len,
          params->max_ce_len,
          params->supervision_timeout);
    return true;
}

/**
 * @brief Called when server requests a passkey for pairing.
 * @return The passkey to be sent to the server.
 */
uint32_t Peer::onPassKeyRequest() {
    log_e("%s TODO send saved passkey", name);
    return 123;
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
void Peer::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    log_i("%s TODO sext8: %d", name, desc->sec_state);
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
    // addChar(nullptr != customApiTxChar
    //             ? customApiTxChar
    //             : new PeerCharacteristicApiTX());
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
#endif