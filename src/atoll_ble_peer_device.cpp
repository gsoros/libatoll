#include "atoll_ble_peer_device.h"

using namespace Atoll;

void BlePeerDevice::connect() {
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

    /*
    //  Now we can read/write/subscribe the charateristics of the services we are interested in
    NimBLERemoteService* pSvc = nullptr;
    NimBLERemoteCharacteristic* pChr = nullptr;
    NimBLERemoteDescriptor* pDsc = nullptr;

    if (pSvc) {
        pChr = pSvc->getCharacteristic("BEEF");

        if (pChr) {
            if (pChr->canRead()) {
                Serial.print(pChr->getUUID().toString().c_str());
                Serial.print(" Value: ");
                Serial.println(pChr->readValue().c_str());
            }

            if (pChr->canWrite()) {
                if (pChr->writeValue("Tasty")) {
                    Serial.print("Wrote new value to: ");
                    Serial.println(pChr->getUUID().toString().c_str());
                } else {
                    device->client->disconnect();
                    return false;
                }

                if (pChr->canRead()) {
                    Serial.print("The value of: ");
                    Serial.print(pChr->getUUID().toString().c_str());
                    Serial.print(" is now: ");
                    Serial.println(pChr->readValue().c_str());
                }
            }
            if (pChr->canNotify()) {
                // if(!pChr->registerForNotify(notifyCB)) {
                if (!pChr->subscribe(true, notifyCB)) {
         device->client->disconnect();
    return ;
}
}
else if (pChr->canIndicate()) {
    // if(!pChr->registerForNotify(notifyCB, false)) {
    if (!pChr->subscribe(false, notifyCB)) {
        device->client->disconnect();
        return false;
    }
}
}
}
else {
    Serial.println("DEAD service not found.");
}

pSvc = device->client->getService("BAAD");
if (pSvc) {
    pChr = pSvc->getCharacteristic("F00D");

    if (pChr) {
        if (pChr->canRead()) {
            Serial.print(pChr->getUUID().toString().c_str());
            Serial.print(" Value: ");
            Serial.println(pChr->readValue().c_str());
        }

        pDsc = pChr->getDescriptor(NimBLEUUID("C01D"));
        if (pDsc) {
            Serial.print("Descriptor: ");
            Serial.print(pDsc->getUUID().toString().c_str());
            Serial.print(" Value: ");
            Serial.println(pDsc->readValue().c_str());
        }

        if (pChr->canWrite()) {
            if (pChr->writeValue("No tip!")) {
                Serial.print("Wrote new value to: ");
                Serial.println(pChr->getUUID().toString().c_str());
            } else {
                device->client->disconnect();
                return false;
            }

            if (pChr->canRead()) {
                Serial.print("The value of: ");
                Serial.print(pChr->getUUID().toString().c_str());
                Serial.print(" is now: ");
                Serial.println(pChr->readValue().c_str());
            }
        }
        if (pChr->canNotify()) {
            // if(!pChr->registerForNotify(notifyCB)) {
            if (!pChr->subscribe(true, notifyCB)) {
                device->client->disconnect();
                return false;
            }
        } else if (pChr->canIndicate()) {
            // if(!pChr->registerForNotify(notifyCB, false)) {
            if (!pChr->subscribe(false, notifyCB)) {
                device->client->disconnect();
                return false;
            }
        }
    }

} else {
    Serial.println("BAAD service not found.");
}
*/

end:
    log_i("end");
    connecting = false;
    return;
}

/**
 * @brief Called after client connects.
 * @param [in] device->client A pointer to the calling client object.
 */
void BlePeerDevice::onConnect(BLEClient* client) { log_i("TODO"); }

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
