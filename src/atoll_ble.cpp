#include "atoll_ble.h"

using namespace Atoll;

void Ble::init(const char *deviceName, uint8_t securityIOCap) {
    if (initDone) {
        // log_i("init already done");
        return;
    }

    BLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
    BLEDevice::setScanDuplicateCacheSize(200);

    BLEDevice::init(deviceName);

    log_i("setMTU(512)");
    BLEDevice::setMTU(512);

    log_i("setSecurityIOCap(%d)", securityIOCap);
    BLEDevice::setSecurityIOCap(securityIOCap);

    initDone = true;
}

bool Ble::initDone = false;
