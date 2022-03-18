#include "atoll_ble_client.h"
#include "atoll_ble_peer_device.h"

using namespace Atoll;

void BlePeerDevice::connect() {
    // https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino
    if (!BleClient::aquireMutex()) return;
    if (connecting) {
        log_i("already connecting to %s", name);
        return;
    }
    connecting = true;

    if (isConnected()) {
        log_i("already connected to %s", name);
        goto end;
    }
    if (!hasClient() && BLEDevice::getClientListSize()) {
        setClient(BLEDevice::getClientByPeerAddress((BLEAddress)address));
        if (hasClient()) {
            if (connectClient(false)) {
                log_i("reconnect failed");
                goto end;
            }
            log_i("reconnected client");
        } else
            setClient(BLEDevice::getDisconnectedClient());
    }
    if (!hasClient()) {
        if (BLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
            log_i("max clients reached - no more connections available");
            goto end;
        }
        setClient(BLEDevice::createClient());
        log_i("new client created");
        if (!connectClient()) {
            deleteClient();
            log_i("failed to connect");
            goto end;
        }
    }
    if (!isConnected()) {
        if (!connectClient()) {
            log_i("failed to connect");
            goto end;
        }
    }
    log_i("connected to %s %s", name, getClient()->getPeerAddress().toString().c_str());
    goto end;

end:
    log_i("end");
    connecting = false;
    BleClient::releaseMutex();
}

void BlePeerDevice::subscribeChars() {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            log_i("subscribing %s", chars[i]->name);
            chars[i]->subscribe(client);
        }
}

// get index of existing characteristic by name
// or first unused index for an empty name
// returns -1 on error
int8_t BlePeerDevice::charIndex(const char* name) {
    size_t len = strlen(name);
    log_i("checking char name '%s' len: %d", name, len);
    for (int8_t i = 0; i < charsMax; i++) {
        if (nullptr == chars[i] && !len) {
            log_i("found unused index %d for empty name", i);
            return i;
        }
        if (len && nullptr != chars[i] && 0 == strcmp(chars[i]->name, name)) {
            log_i("found index %d of char '%s'", i, name);
            return i;
        }
    }
    // log_i("could not find index of char '%s'", name);
    return -1;
}

bool BlePeerDevice::charExists(const char* name) {
    return 0 <= charIndex(name);
}

bool BlePeerDevice::addChar(BlePeerCharacteristic* c) {
    int8_t index = charIndex(c->name);
    if (index < 0) index = charIndex("");
    if (index < 0) {
        log_e("cannot add char '%s'", c->name);
        return false;
    }
    log_i("adding char name: '%s', uuid: '%s'", c->name, c->charUuid.toString().c_str());
    chars[index] = c;
    return true;
}

BlePeerCharacteristic* BlePeerDevice::getChar(const char* name) {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i] && 0 == strcmp(chars[i]->name, name))
            return chars[i];
    return nullptr;
}

bool BlePeerDevice::removeCharAt(int8_t index) {
    if (charsMax <= index) return false;
    chars[index] = nullptr;
    return true;
}

/**
 * @brief Called after client connects.
 * @param [in] device->client A pointer to the calling client object.
 */
void BlePeerDevice::onConnect(BLEClient* client) {
    log_i("subscribing");
    subscribeChars();
}

/**
 * @brief Called when disconnected from the server.
 * @param [in] device->client A pointer to the calling client object.
 */
void BlePeerDevice::onDisconnect(BLEClient* client) { log_i("TODO"); }

/**
 * @brief Called when server requests to update the connection parameters.
 * @param [in] device->client A pointer to the calling client object.
 * @param [in] params A pointer to the struct containing the connection parameters requested.
 * @return True to accept the parmeters.
 */
bool BlePeerDevice::onConnParamsUpdateRequest(BLEClient* client, const ble_gap_upd_params* params) {
    log_i("TODO client: %s st: %d", client->getPeerAddress().toString().c_str(), params->supervision_timeout);
    return true;
}

/**
 * @brief Called when server requests a passkey for pairing.
 * @return The passkey to be sent to the server.
 */
uint32_t BlePeerDevice::onPassKeyRequest() {
    log_e("TODO send saved passkey");
    return 123;
}

/*
void BlePeerDevice::onPassKeyNotify(uint32_t pass_key);
bool BlePeerDevice::onSecurityRequest() {}
*/

/**
 * @brief Called when the pairing procedure is complete.
 * @param [in] desc A pointer to the struct containing the connection information.\n
 * This can be used to check the status of the connection encryption/pairing.
 */
void BlePeerDevice::onAuthenticationComplete(ble_gap_conn_desc* desc) {
    log_i("TODO state: %d", desc->sec_state);
}

/**
 * @brief Called when using numeric comparision for pairing.
 * @param [in] pin The pin to compare with the server.
 * @return True to accept the pin.
 */
bool BlePeerDevice::onConfirmPIN(uint32_t pin) {
    log_i("TODO");
    return true;
}

void BlePeerDevice::onNotify(BLERemoteCharacteristic* c, uint8_t* data, size_t length, bool isNotify) {
    char buf[length];
    strncpy(buf, (char*)data, length);
    log_i("char: %s, data: '%s', len: %d", c->getUUID().toString().c_str(), buf, length);
}