#if !defined(__atoll_gps_h) && defined(FEATURE_GPS)
#define __atoll_gps_h

#include <Arduino.h>
#include <HardwareSerial.h>
#include <TinyGPSPlus.h>

#include "atoll_task.h"
#include "atoll_touch.h"
#include "atoll_log.h"

namespace Atoll {

class GPS : public Atoll::Task {
   public:
    const char *taskName() { return "GPS"; }
    HardwareSerial *serial;
    TinyGPSPlus device;
    double minWalkingSpeed = 2.0;  // km/h
    double minCyclingSpeed = 8.0;  // km/h

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
        return device.satellites.value();
    }

    bool locationIsValid() {
        return device.location.isValid();
    }

    bool isMoving() {
        return device.speed.isValid() && minWalkingSpeed <= device.speed.kmph();
    }

    void loop();

    bool syncSystemTime();
};

}  // namespace Atoll
#endif