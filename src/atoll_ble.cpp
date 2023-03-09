#ifdef FEATURE_BLE
#include "atoll_ble.h"
#include "atoll_ble_constants.h"
#include "atoll_log.h"

using namespace Atoll;

void Ble::init(const char *deviceName, uint16_t mtu, uint8_t iocap) {
    if (initDone) {
        // log_i("init already done");
        return;
    }

    BLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
    BLEDevice::setScanDuplicateCacheSize(200);

    BLEDevice::init(deviceName);

    // log_d("setMTU(%d)", mtu);
    BLEDevice::setMTU(mtu);

    // log_d("power: %d", BLEDevice::getPower());

    securityIOCap = iocap;
    setSecurityIOCap(iocap);

    initDone = true;
}

void Ble::setSecurityIOCap(uint8_t iocap) {
    // log_d("setSecurityIOCap(%d)", iocap);
    BLEDevice::setSecurityIOCap(iocap);
}

void Ble::restoreSecurityIOCap() {
    setSecurityIOCap(securityIOCap);
}

bool Ble::deleteBond(const char *address) {
    if (0 == strcmp(address, "*")) {
        BLEDevice::deleteAllBonds();
        return true;
    }
    return BLEDevice::deleteBond(BLEAddress(address));
}

std::string Ble::connInfoToStr(BLEConnInfo *info) {
    uint8_t bufSize = 64;
    char buf[bufSize];
    std::string s("connection ");
    snprintf(buf, bufSize, "#%d [", info->getConnHandle());
    s += buf;
    snprintf(buf, bufSize, "address: %s", info->getAddress().toString().c_str());
    s += buf;
    if (0 != strcmp(info->getAddress().toString().c_str(), info->getIdAddress().toString().c_str())) {
        snprintf(buf, bufSize, " (%s)", info->getIdAddress().toString().c_str());
        s += buf;
    }
    snprintf(buf, bufSize, ", interval: %d (%dms)",
             info->getConnInterval(), (int)(info->getConnInterval() * 1.25));
    s += buf;
    snprintf(buf, bufSize, ", latency: %d", info->getConnLatency());
    s += buf;
    snprintf(buf, bufSize, ", timeout: %d (%dms)",
             info->getConnTimeout(), info->getConnTimeout() * 10);
    s += buf;
    snprintf(buf, bufSize, ", MTU: %d", info->getMTU());
    s += buf;
    snprintf(buf, bufSize, ", %s", info->isMaster() ? "master" : info->isSlave() ? "slave"
                                                                                 : "unknown role");
    s += buf;
    snprintf(buf, bufSize, ", %sencrypted", info->isEncrypted() ? "" : "not ");
    s += buf;
    snprintf(buf, bufSize, ", %sauthenticated", info->isAuthenticated() ? "" : "not ");
    s += buf;
    snprintf(buf, bufSize, ", %sbonded", info->isBonded() ? "" : "not ");
    s += buf;
    snprintf(buf, bufSize, ", keySize: %d]", info->getSecKeySize());
    s += buf;
    return s;
}

std::string Ble::charUUIDToStr(BLEUUID uuid) {
    if (uuid.equals(BLEUUID(CSC_MEASUREMENT_CHAR_UUID))) return std::string("Speed and Cadence");
    if (uuid.equals(BLEUUID(CYCLING_POWER_MEASUREMENT_CHAR_UUID))) return std::string("Power");
    if (uuid.equals(BLEUUID(BATTERY_LEVEL_CHAR_UUID))) return std::string("Battery");
    if (uuid.equals(BLEUUID(WEIGHT_MEASUREMENT_CHAR_UUID))) return std::string("Weight");
    if (uuid.equals(BLEUUID(TEMPERATURE_CHAR_UUID))) return std::string("Temperature");
    if (uuid.equals(BLEUUID(API_RX_CHAR_UUID))) return std::string("API RX");
    if (uuid.equals(BLEUUID(API_TX_CHAR_UUID))) return std::string("API TX");
    if (uuid.equals(BLEUUID(API_LOG_CHAR_UUID))) return std::string("API Log");
    if (uuid.equals(BLEUUID(HALL_CHAR_UUID))) return std::string("Hall");
    if (uuid.equals(BLEUUID(VESC_RX_CHAR_UUID))) return std::string("Vesc RX");
    if (uuid.equals(BLEUUID(VESC_TX_CHAR_UUID))) return std::string("Vesc TX");
    return uuid.toString();
}

bool Ble::initDone = false;
uint8_t Ble::securityIOCap = ATOLL_BLE_SECURITY_IOCAP_DEFAULT;

#endif