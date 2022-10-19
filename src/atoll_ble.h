#ifndef __atoll_ble_h
#define __atoll_ble_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_log.h"

#ifndef ATOLL_BLE_SECURITY_IOCAP_DEFAULT
#define ATOLL_BLE_SECURITY_IOCAP_DEFAULT BLE_HS_IO_DISPLAY_ONLY
#endif

namespace Atoll {

class Ble {
   public:
    static void init(
        const char *deviceName,
        uint8_t securityIOCap = ATOLL_BLE_SECURITY_IOCAP_DEFAULT);

    static String connInfoToStr(BLEConnInfo *info);

   protected:
    static bool initDone;
};

}  // namespace Atoll
#endif