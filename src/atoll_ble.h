#ifndef __atoll_ble_h
#define __atoll_ble_h

#include <Arduino.h>
#include <NimBLEDevice.h>

#include "atoll_log.h"

#ifndef BLE_SECURITY_IOCAP
#define BLE_SECURITY_IOCAP BLE_HS_IO_DISPLAY_ONLY
#endif

namespace Atoll {

class Ble {
   public:
    static void init(const char *deviceName);

   protected:
    static bool initDone;
};

}  // namespace Atoll
#endif