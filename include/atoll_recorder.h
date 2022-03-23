#ifndef __atoll_recorder_h
#define __atoll_recorder_h

#include <Arduino.h>

#include "atoll_task.h"
#include "atoll_gps.h"

#ifndef ATOLL_RECORDER_BUFFER_SIZE
#define ATOLL_RECORDER_BUFFER_SIZE 60
#endif

namespace Atoll {

class Recorder : public Task {
   public:
    struct DataPoint {
        time_t time;        // uts
        double lat;         // latitude
        double lon;         // longitude
        int16_t alt;        // altitude in m
        uint16_t power;     // W
        uint8_t cadence;    // rpm
        uint8_t heartrate;  // bpm
    };

    uint16_t interval = 1000;                          // recording interval in milliseconds
    DataPoint buffer[ATOLL_RECORDER_BUFFER_SIZE];      // recording buffer
    uint16_t bufferSize = ATOLL_RECORDER_BUFFER_SIZE;  //
    uint16_t numDataPoints = 0;                        //
    bool isRecording = false;                          //
    GPS *gps = nullptr;                                //

    void setup(GPS *gps) {
        this->gps = gps;
    }

    void loop() {
        static ulong lastDataPointTime = 0;
        ulong t = millis();
        if ((lastDataPointTime < t - interval) && interval < t) {
            addDataPoint();
            if (bufferSize == numDataPoints)
                saveBuffer();
        }
    }

    void addDataPoint() {
        if (!gps->gps.date.isValid()) {
            log_i("gps date invalid");
            return;
        }
        if (!gps->gps.time.isValid()) {
            log_i("gps time invalid");
            return;
        }
        if (!gps->gps.location.isValid()) {
            log_i("gps location invalid");
            return;
        }
        TinyGPSDate *gpsd = &gps->gps.date;
        TinyGPSTime *gpst = &gps->gps.time;
        TinyGPSLocation *gpsl = &gps->gps.location;

        // struct tm:
        // Member	Type	Meaning	                    Range
        // tm_sec	int	    seconds after the minute	0-60*
        // tm_min	int 	minutes after the hour	    0-59
        // tm_hour	int	    hours since midnight	    0-23
        // tm_mday	int 	day of the month	        1-31
        // tm_mon	int	    months since January	    0-11
        // tm_year	int 	years since 1900
        // tm_wday	int	    days since Sunday	        0-6
        // tm_yday	int 	days since January 1	    0-365
        // tm_isdst	int 	Daylight Saving Time flag
        struct tm tms;
        tms.tm_year = gpsd->year() - 1900;
        tms.tm_mon = gpsd->month();
        tms.tm_mday = gpsd->day();
        tms.tm_hour = gpst->hour();
        tms.tm_min = gpst->minute();
        tms.tm_sec = gpst->second();

        time_t time = mktime(&tms);
        if (-1 == time) {
            log_e("invalid time");
            return;
        }
        buffer[numDataPoints].time = time;
        buffer[numDataPoints].lat = gpsl->lat();
        buffer[numDataPoints].lon = gpsl->lng();
        buffer[numDataPoints].alt = (int16_t)(gps->gps.altitude.meters());

        log_i("#%d time: %d loc: %.9f %.9f alt: %d, dist: %.9fm",
              numDataPoints,
              time,
              buffer[numDataPoints].lat,
              buffer[numDataPoints].lon,
              buffer[numDataPoints].alt,
              gps->gps.distanceBetween(gpsl->lat(), gpsl->lng(), (float)gpsl->lat(), (float)gpsl->lng()));

        numDataPoints++;
    }

    void saveBuffer() {
        log_i("saving buffer");
        numDataPoints = 0;
    }

    void emptyBuffer() {
        for (uint16_t i = 0; i < bufferSize; i++)
            buffer[i] = DataPoint();
    }

    void start() {
        isRecording = true;
    }
    void stop() {
        isRecording = false;
    }
};

}  // namespace Atoll

#endif