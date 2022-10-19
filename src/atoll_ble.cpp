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

String Ble::connInfoToStr(BLEConnInfo *info) {
    uint8_t bufSize = 64;
    char buf[bufSize];
    String s("Connection info for ");
    snprintf(buf, bufSize, "#%d: ", info->getConnHandle());
    s += buf;
    snprintf(buf, bufSize, "address: %s (%s), ",
             info->getAddress().toString().c_str(),
             info->getIdAddress().toString().c_str());
    s += buf;
    snprintf(buf, bufSize, "interval: %d (%dms), ",
             info->getConnInterval(), (int)(info->getConnInterval() * 1.25));
    s += buf;
    snprintf(buf, bufSize, "latency: %d, ", info->getConnLatency());
    s += buf;
    snprintf(buf, bufSize, "timeout: %d (%dms), ",
             info->getConnTimeout(), info->getConnTimeout() * 10);
    s += buf;
    snprintf(buf, bufSize, "MTU: %dB, ", info->getMTU());
    s += buf;
    snprintf(buf, bufSize, "%s, ", info->isMaster() ? "master" : info->isSlave() ? "slave"
                                                                                 : "unknown role");
    s += buf;
    snprintf(buf, bufSize, "%sencrypted, ", info->isEncrypted() ? "" : "not ");
    s += buf;
    snprintf(buf, bufSize, "%sauthenticated, ", info->isAuthenticated() ? "" : "not ");
    s += buf;
    snprintf(buf, bufSize, "%sbonded, ", info->isBonded() ? "" : "not ");
    s += buf;
    snprintf(buf, bufSize, "keySize: %d", info->getSecKeySize());
    s += buf;
    return s;
}

bool Ble::initDone = false;
