#ifndef __atoll_gps_h
#define __atoll_gps_h

#include <SoftwareSerial.h>
#include <TinyGPS++.h>

#include "atoll_task.h"
#include "atoll_touch.h"
#include "atoll_oled.h"

namespace Atoll {

class GPS : public Atoll::Task {
   public:
    const char *taskName() { return "GPS"; }
    SoftwareSerial ss;
    TinyGPSPlus gps;
    Touch *touch = nullptr;
    Oled *oled = nullptr;

    GPS() {}
    virtual ~GPS();

    void setup(
        uint32_t baud,
        SoftwareSerialConfig config,
        int8_t rxPin,
        int8_t txPin,
        Touch *touch = nullptr,
        Oled *oled = nullptr) {
        this->touch = touch;
        this->oled = oled;
        // ss.begin(9600, SWSERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
        ss.begin(baud, config, rxPin, txPin);
    }

    void loop();
};

}  // namespace Atoll
#endif