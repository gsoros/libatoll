#ifndef __atoll_ble_h
#define __atoll_ble_h

#include <Arduino.h>
#include <NimBLEDevice.h>

namespace Atoll {

class Ble {
   public:
    static void init(const char *deviceName) {
        if (initDone) {
            log_e("init already done");
            return;
        }
        BLEDevice::init(deviceName);
        // BLEDevice::setMTU(517);
        initDone = true;
    }

   protected:
    static bool initDone;
};

}  // namespace Atoll
#endif