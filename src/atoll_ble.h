#if !defined(__atoll_ble_h) && defined(FEATURE_BLE)
#define __atoll_ble_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_log.h"

#ifndef ATOLL_BLE_MTU_DEFAULT
#define ATOLL_BLE_MTU_DEFAULT 79
#endif

#ifndef ATOLL_BLE_SECURITY_IOCAP_DEFAULT
#define ATOLL_BLE_SECURITY_IOCAP_DEFAULT BLE_HS_IO_DISPLAY_ONLY
#endif

namespace Atoll {

typedef NIMBLE_PROPERTY BLE_PROP;

class Ble {
   public:
    static void init(
        const char *deviceName,
        uint16_t mtu = ATOLL_BLE_MTU_DEFAULT,
        uint8_t securityIOCap = ATOLL_BLE_SECURITY_IOCAP_DEFAULT);

    static void setSecurityIOCap(uint8_t ioCap);
    static void restoreSecurityIOCap();
    // address = "*" to delete all bonds
    static bool deleteBond(const char *address);

    static std::string connInfoToStr(BLEConnInfo *info);
    static std::string charUUIDToStr(BLEUUID uuid);

   protected:
    static bool initDone;
    static uint8_t securityIOCap;  // default io capabilty
};

}  // namespace Atoll
#endif