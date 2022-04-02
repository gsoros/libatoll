#ifndef __atoll_gps_h
#define __atoll_gps_h

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPS++.h>

#include "atoll_task.h"
#include "atoll_touch.h"
#include "atoll_oled.h"
#include "atoll_log.h"

namespace Atoll {

class GPS : public Atoll::Task {
   public:
    const char *taskName() { return "GPS"; }
    // SoftwareSerial ss;
    HardwareSerial *serial;
    TinyGPSPlus gps;

    GPS() {}
    virtual ~GPS();

    void setup(
        uint32_t baud,
        uint32_t config,
        int8_t rxPin,
        int8_t txPin) {
        // ss.begin(baud, config, rxPin, txPin);
        serial = new HardwareSerial(1);
        serial->begin(baud, config, rxPin, txPin);
    }

    uint32_t satellites() {
        return gps.satellites.value();
    }

    void loop();

    bool syncSystemTime();
};

}  // namespace Atoll
#endif