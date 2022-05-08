#include "atoll_gps.h"
#include "atoll_time.h"

using namespace Atoll;

GPS::~GPS() { delete serial; }

void GPS::loop() {
    uint32_t failedChecksum = device.failedChecksum();
    while (0 < serial->available())
        device.encode(serial->read());

    if (device.failedChecksum() != failedChecksum)
        log_i("checksums failed: %d", device.failedChecksum());

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
    if (!device.time.isValid() || !device.date.isValid() || device.date.year() < 2022)
        return false;

    log_i("syncing system time to gps time %04d-%02d-%02d %02d:%02d:%02d",
          device.date.year(),
          device.date.month(),
          device.date.day(),
          device.time.hour(),
          device.time.minute(),
          device.time.second());

    setSytemTimeFromUtc(
        device.date.year(),
        device.date.month(),
        device.date.day(),
        device.time.hour(),
        device.time.minute(),
        device.time.second(),
        device.time.centisecond());

    return true;
}
