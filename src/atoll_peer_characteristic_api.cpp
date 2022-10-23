#if !defined(CONFIG_BT_NIMBLE_ROLE_CENTRAL_DISABLED)

#include "atoll_peer_characteristic_api.h"
#include "atoll_ble.h"

using namespace Atoll;

void PeerCharacteristicApi::remoteOpStart(BLEClient* client) {
    if (needsSecurity(client))
        Ble::setSecurityIOCap(remoteOpIOCap);
}

void PeerCharacteristicApi::remoteOpEnd(BLEClient* client) {
    if (needsSecurity(client))
        Ble::restoreSecurityIOCap();
}

bool PeerCharacteristicApi::needsSecurity(BLEClient* client) {
    if (nullptr == client) {
        log_e("%s client is null", label);
        return false;
    }
    NimBLEConnInfo info = client->getConnInfo();
    if (info.isAuthenticated() && info.isBonded() && info.isEncrypted())
        return false;
    return true;
}

#endif