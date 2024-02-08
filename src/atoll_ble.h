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

typedef NIMBLE_PROPERTY BLE_PROP;

namespace Atoll {

class Ble {
   public:
    static void init(
        const char *deviceName,
        uint16_t mtu = ATOLL_BLE_MTU_DEFAULT,
        uint8_t defaultIOCap = ATOLL_BLE_SECURITY_IOCAP_DEFAULT);
    static void deinit();
    static void reinit();
    static bool initDone();

    static void setSecurityIOCap(uint8_t ioCap);
    static void setDefaultIOCap();
    // address = "*" to delete all bonds
    static bool deleteBond(const char *address);
    static uint16_t getMTU();
    static bool setMTU(uint16_t mtu);

    static std::string connInfoToStr(BLEConnInfo *info);
    static std::string charUUIDToStr(BLEUUID uuid);

   protected:
    static char deviceName[16];
    static uint8_t defaultIOCap;  // default io capabilty
    static uint8_t currentIOCap;  // current io capabilty
};

}  // namespace Atoll
#endif