#ifndef __atoll_recorder_h
#define __atoll_recorder_h

#include <Arduino.h>

#include "atoll_task.h"
#include "atoll_gps.h"
#include "atoll_fs.h"

#ifndef ATOLL_RECORDER_BUFFER_SIZE
#define ATOLL_RECORDER_BUFFER_SIZE 60
#endif

namespace Atoll {

class Recorder : public Task {
   public:
    struct DataPoint {
        time_t time = 0;        // uts
        double lat = 0.0;       // latitude
        double lon = 0.0;       // longitude
        int16_t alt = 0;        // altitude in m
        uint16_t power = 0;     // W
        uint8_t cadence = 0;    // rpm
        uint8_t heartrate = 0;  // bpm
    };

    const char *taskName() { return "Recorder"; }
    uint16_t interval = 1000;                       // recording interval in milliseconds
    DataPoint buffer[ATOLL_RECORDER_BUFFER_SIZE];   // recording buffer
    uint16_t bufSize = ATOLL_RECORDER_BUFFER_SIZE;  //
    uint16_t bufLastIndex = 0;                      //
    bool isRecording = false;                       //
    GPS *gps = nullptr;                             //
    FS *fs = nullptr;                               //
    const char basePath[5] = "/rec";                //
    char filePath[30] = "";

    void setup(GPS *gps, Fs *fs) {
        if (nullptr == gps) {
            log_e("no gps");
            return;
        }
        this->gps = gps;
        if (nullptr == fs) {
            log_e("no fs");
            return;
        }
        if (!fs->mounted) {
            log_e("fs not mounted");
            return;
        }
        this->fs = fs->fsp();
    }

    void
    loop() {
        static ulong lastDataPointTime = 0;
        ulong t = millis();
        if ((lastDataPointTime < t - interval) && interval < t) {
            addDataPoint();
            if (bufSize == bufLastIndex) {
                if (!saveBuffer())
                    log_e("could not save buffer");
                resetBuffer();
            }
        }
    }

    void addDataPoint() {
        if (!isRecording) return;
        if (nullptr == gps) {
            log_e("no gps");
            return;
        }
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
        tms.tm_mon = gpsd->month() - 1;
        tms.tm_mday = gpsd->day();
        tms.tm_hour = gpst->hour();
        tms.tm_min = gpst->minute();
        tms.tm_sec = gpst->second();

        buffer[bufLastIndex].time = mktime(&tms);
        if (-1 == buffer[bufLastIndex].time) {
            log_e("invalid time");
            return;
        }
        buffer[bufLastIndex].lat = gpsl->lat();
        buffer[bufLastIndex].lon = gpsl->lng();
        buffer[bufLastIndex].alt = (int16_t)gps->gps.altitude.meters();

        // log_i("#%d time: %d loc: %.9f %.9f alt: %d, double vs float: %.9fm",
        //       bufLastIndex,
        //       time,
        //       buffer[bufLastIndex].lat,
        //       buffer[bufLastIndex].lon,
        //       buffer[bufLastIndex].alt,
        //       gps->gps.distanceBetween(gpsl->lat(), gpsl->lng(), (float)gpsl->lat(), (float)gpsl->lng()));

        bufLastIndex++;
    }

    bool saveBuffer() {
        size_t toWrite = sizeof(DataPoint) * bufLastIndex;
        if (0 == toWrite) {
            log_e("buffer is empty");
            return true;
        }
        log_i("saving buffer, %d entries %d bytes each, total %d bytes",
              bufLastIndex,
              sizeof(DataPoint),
              toWrite);
        if (nullptr == fs) {
            log_e("no fs");
            return false;
        }
        char *path = currentFilePath();
        if (nullptr == path) {
            log_e("could not get current file path");
            return false;
        }
        File file = fs->open(path, FILE_APPEND);
        if (!file) {
            log_e("could not open %s for appending", path);
            return false;
        }
        size_t wrote = file.write((uint8_t *)buffer, toWrite);
        if (toWrite != wrote) {
            if (0 == wrote) {
                log_e("cannot write to %s", path);
                file.close();
                return false;
            }
            log_e("buffer is %d bytes but wrote only %d bytes to %s",
                  toWrite,
                  wrote,
                  path);
            file.close();
            return false;
        }
        log_i("wrote %d bytes to %s (%d bytes)", wrote, path, file.size());
        file.close();
        bufLastIndex = 0;
        return true;
    }

    char *currentFilePath() {
        if (0 != strcmp(filePath, ""))
            return filePath;

        if (nullptr == fs) {
            log_e("no fs");
            return nullptr;
        }
        File file = fs->open(basePath);
        if (!file) {
            if (!fs->mkdir(basePath)) {
                log_e("could not create %s", basePath);
                return nullptr;
            }
            file = fs->open(basePath);
        }
        if (!file) {
            log_e("could not open %s", basePath);
            return nullptr;
        }
        file.close();
        if (0 == buffer[0].time) {
            log_e("first datapoint is not valid");
            return nullptr;
        }
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
        struct tm *tms = gmtime(&buffer[0].time);
        // snprintf(filePath, sizeof(filePath), "%s/%d-%01d-%01d_%01d:%01d.rec",
        //          basePath,
        //          tms->tm_year + 1900,
        //          tms->tm_mon + 1,
        //          tms->tm_mday,
        //          tms->tm_hour,
        //          tms->tm_min);
        snprintf(filePath, sizeof(filePath), "%s/%02d%02d%02d%02d.rec",
                 basePath,
                 tms->tm_mon + 1,
                 tms->tm_mday,
                 tms->tm_hour,
                 tms->tm_min);
        return filePath;
    }

    void resetBuffer(bool clearPoints = false) {
        bufLastIndex = 0;
        if (clearPoints)
            for (uint16_t i = 0; i < bufSize; i++)
                buffer[i] = DataPoint();
    }

    bool start() {
        if (nullptr == fs) {
            log_e("no fs");
            return false;
        }
        log_i("starting recording");
        if (isRecording) {
            log_e("already recording");
            return false;
        }
        strncpy(filePath, "", sizeof(filePath));
        resetBuffer();
        isRecording = true;
        return true;
    }
    bool stop() {
        if (!isRecording) return false;
        log_i("stopping recording");
        isRecording = false;
        strncpy(filePath, "", sizeof(filePath));
        if (!saveBuffer())
            log_e("could not save buffer");
        resetBuffer();
        return true;
    }
};

}  // namespace Atoll

#endif