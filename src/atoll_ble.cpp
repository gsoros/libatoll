#ifdef FEATURE_BLE
#include "atoll_ble.h"
#include "atoll_ble_constants.h"
#include "atoll_log.h"

using namespace Atoll;

void Ble::init(const char *deviceName, uint16_t mtu, uint8_t iocap) {
    log_d("init");
    if (initDone()) {
        log_i("init already done");
        return;
    }

    BLEDevice::setScanFilterMode(CONFIG_BTDM_SCAN_DUPL_TYPE_DEVICE);
    BLEDevice::setScanDuplicateCacheSize(200);

    snprintf(Ble::deviceName, sizeof(Ble::deviceName), "%s", deviceName);
    BLEDevice::init(Ble::deviceName);

    // log_d("setMTU(%d)", mtu);
    setMTU(mtu);

    // log_d("power: %d", BLEDevice::getPower());

    setSecurityIOCap(iocap);
    defaultIOCap = iocap;
}

void Ble::deinit() {
    log_d("deinit");
    if (!initDone()) {
        log_i("init not done");
        return;
    }
    BLEDevice::deinit(true);
}

void Ble::reinit() {
    log_d("reinit");
    if (!initDone()) {
        log_i("init not done");
        return;
    }
    uint16_t mtu = BLEDevice::getMTU();
    log_d("calling deinit");
    deinit();
    log_d("calling init");
    init(Ble::deviceName, mtu, currentIOCap);
}

bool Ble::initDone() {
    return BLEDevice::getInitialized();
}

void Ble::setSecurityIOCap(uint8_t iocap) {
    // log_d("setSecurityIOCap(%d)", iocap);
    BLEDevice::setSecurityIOCap(iocap);
    currentIOCap = iocap;
}

void Ble::setDefaultIOCap() {
    setSecurityIOCap(defaultIOCap);
}

bool Ble::deleteBond(const char *address) {
    if (0 == strcmp(address, "*")) {
        BLEDevice::deleteAllBonds();
        return true;
    }
    return BLEDevice::deleteBond(BLEAddress(address));
}

uint16_t Ble::getMTU() {
    return BLEDevice::getMTU();
}

bool Ble::setMTU(uint16_t mtu) {
    return 0 == BLEDevice::setMTU(mtu);
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

uint8_t Ble::defaultIOCap = ATOLL_BLE_SECURITY_IOCAP_DEFAULT;
uint8_t Ble::currentIOCap = ATOLL_BLE_SECURITY_IOCAP_DEFAULT;
char Ble::deviceName[16] = "unnamed";

#endif