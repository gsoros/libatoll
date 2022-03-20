#include "atoll_ble_client.h"
#include "atoll_ble_peer_device.h"

using namespace Atoll;

BlePeerDevice::~BlePeerDevice() {
    log_i("calling deleteClient() for %s", name);
    disconnect();
    deleteClient();
}

void BlePeerDevice::connect() {
    log_i("connecting to %s", name);

    // https://github.com/h2zero/NimBLE-Arduino/blob/master/examples/NimBLE_Client/NimBLE_Client.ino
    if (connecting) {
        log_i("already connecting to %s", name);
        return;
    }
    connecting = true;

    if (isConnected()) {
        log_i("already connected to %s", name);
        goto end;
    }
    if (BLEDevice::getClientListSize()) {
        setClient(BLEDevice::getClientByPeerAddress((BLEAddress)address));
        if (hasClient()) {
            if (!connectClient(false)) {
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
            // deleteClient();
            log_i("failed to connect (1)");
            goto end;
        }
    }
    if (!isConnected()) {
        if (!connectClient()) {
            log_i("failed to connect (2)");
            goto end;
        }
    }
    log_i("connected to %s %s", name, getClient()->getPeerAddress().toString().c_str());
    goto end;

end:
    log_i("end");
    connecting = false;
}

void BlePeerDevice::subscribeChars() {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            log_i("subscribing %s", chars[i]->label);
            chars[i]->subscribe(client);
        }
}

void BlePeerDevice::unsubscribeChars() {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i]) {
            log_i("unsubscribing %s", chars[i]->label);
            chars[i]->unsubscribe();
        }
}

// get index of existing characteristic by label
// or first unused index for an empty label
// returns -1 on error
int8_t BlePeerDevice::charIndex(const char* label) {
    size_t len = strlen(label);
    log_i("checking char label '%s' len: %d", label, len);
    for (int8_t i = 0; i < charsMax; i++) {
        if (nullptr == chars[i] && !len) {
            log_i("found unused index %d for empty label", i);
            return i;
        }
        if (len && nullptr != chars[i] && 0 == strcmp(chars[i]->label, label)) {
            log_i("found index %d of char '%s'", i, label);
            return i;
        }
    }
    // log_i("could not find index of char '%s'", label);
    return -1;
}

// chack if a char with label exists
bool BlePeerDevice::charExists(const char* label) {
    return 0 <= charIndex(label);
}

// add new characteristic
bool BlePeerDevice::addChar(BlePeerCharacteristic* c) {
    int8_t index = charIndex(c->label);
    if (index < 0) index = charIndex("");
    if (index < 0) {
        log_e("cannot add char '%s'", c->label);
        return false;
    }
    log_i("adding char label: '%s', uuid: '%s'", c->label, c->charUuid.toString().c_str());
    chars[index] = c;
    return true;
}

// get first characteristic pointer with label
BlePeerCharacteristic* BlePeerDevice::getChar(const char* label) {
    for (int8_t i = 0; i < charsMax; i++)
        if (nullptr != chars[i] && 0 == strcmp(chars[i]->label, label))
            return chars[i];
    return nullptr;
}

// remove char at index
bool BlePeerDevice::removeCharAt(int8_t index) {
    if (charsMax <= index) return false;
    chars[index] = nullptr;
    return true;
}

// delete chars that have this label
uint8_t BlePeerDevice::deleteChars(const char* label) {
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
void BlePeerDevice::onConnect(BLEClient* client) {
    connected = true;
    log_i("subscribing");
    subscribeChars();
}

/**
 * @brief Called when disconnected from the server.
 * @param [in] device->client A pointer to the calling client object.
 */
void BlePeerDevice::onDisconnect(BLEClient* client) {
    connected = false;
    log_i("disconnected");
}

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
    log_i("uuid: %s, data: '%s', len: %d", c->getUUID().toString().c_str(), buf, length);
}