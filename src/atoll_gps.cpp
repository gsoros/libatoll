#include "atoll_gps.h"
#include "atoll_time.h"

using namespace Atoll;

GPS::~GPS() {}

void GPS::loop() {
    while (ss.available() > 0)
        gps.encode(ss.read());

    // if (gps.speed.kmph() < 0.01) return;
    // static unsigned long lastStatus = millis();
    // if (millis() < lastStatus + 3000) return;
    // lastStatus = millis();
    // log_i(
    //     "[%s] %f %f %.1fm %.2fkm/h %d %d%ssat %04d-%02d-%02d%s %02d:%02d:%02d%s(%04d) F%d P%d S%d",  //
    //     gps.location.isValid() ? "F" : "N",
    //     gps.location.lat(),
    //     gps.location.lng(),
    //     gps.altitude.meters(),
    //     gps.speed.kmph(),
    //     gps.hdop.value(),
    //     gps.satellites.value(),
    //     gps.satellites.isValid() ? "" : "X",
    //     gps.date.year(),
    //     gps.date.month(),
    //     gps.date.day(),
    //     gps.date.isValid() ? "" : "X",
    //     gps.time.hour(),
    //     gps.time.minute(),
    //     gps.time.second(),
    //     gps.time.isValid() ? "" : "X",
    //     gps.time.age(),
    //     gps.failedChecksum(),
    //     gps.passedChecksum(),
    //     gps.sentencesWithFix());
}

bool GPS::syncSystemTime() {
    if (!gps.time.isValid() || !gps.date.isValid())
        return false;

    log_i("syncing system time to gps time %04d-%02d-%02d %02d:%02d:%02d",
          gps.date.year(),
          gps.date.month(),
          gps.date.day(),
          gps.time.hour(),
          gps.time.minute(),
          gps.time.second());

    setSytemTimeFromUtc(
        gps.date.year(),
        gps.date.month(),
        gps.date.day(),
        gps.time.hour(),
        gps.time.minute(),
        gps.time.second(),
        gps.time.centisecond());

    return true;
}
