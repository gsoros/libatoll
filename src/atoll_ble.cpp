#include "atoll_ble.h"

using namespace Atoll;

void Ble::init(const char *deviceName) {
    if (initDone) {
        // log_i("init already done");
        return;
    }

    BLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
    BLEDevice::setScanDuplicateCacheSize(200);

    BLEDevice::init(deviceName);

    log_i("setSecurityIOCap(%d)", BLE_SECURITY_IOCAP);
    BLEDevice::setSecurityIOCap(BLE_SECURITY_IOCAP);

    initDone = true;
}

bool Ble::initDone = false;
