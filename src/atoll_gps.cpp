#include "atoll_gps.h"

using namespace Atoll;

GPS::~GPS() {}

void GPS::loop() {
    // log_i("loop");
    while (ss.available() > 0) {
        gps.encode(ss.read());
        // char c = ss.read();
        // gps.encode(c);
        // Serial.print(c);
    }

    if (nullptr != oled) {
        if (nullptr == touch ||
            (nullptr != touch && !touch->anyPadIsTouched())) {
            if (gps.time.isValid() && gps.time.isUpdated()) {
                static ulong lastTime = 0;
                if (lastTime < millis() - 1000) {
                    oled->onGps(&gps);
                    lastTime = millis();
                }
            } else {
                static uint32_t satellites = UINT32_MAX;
                if (satellites != gps.satellites.value()) {
                    satellites = gps.satellites.value();
                    oled->onSatellites(satellites);
                }
            }
        }
    }

    if (gps.speed.kmph() < 0.01) return;
    static unsigned long lastStatus = millis();
    if (millis() < lastStatus + 3000) return;
    lastStatus = millis();
    log_i(
        "[%s] %f %f %.1fm %.2fkm/h %d %d%ssat %02d:%02d:%02d%s(%04d) F%d P%d S%d",  //
        gps.location.isValid() ? "F" : "N",
        gps.location.lat(),
        gps.location.lng(),
        gps.altitude.meters(),
        gps.speed.kmph(),
        gps.hdop.value(),
        gps.satellites.value(),
        gps.satellites.isValid() ? "" : "X",
        gps.time.hour(),
        gps.time.minute(),
        gps.time.second(),
        gps.time.isValid() ? "" : "X",
        gps.time.age(),
        gps.failedChecksum(),
        gps.passedChecksum(),
        gps.sentencesWithFix());
}