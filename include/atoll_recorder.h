#ifndef __atoll_recorder_h
#define __atoll_recorder_h

#include <Arduino.h>
#include <CircularBuffer.h>

#include "atoll_task.h"
#include "atoll_gps.h"
#include "atoll_fs.h"

#ifndef ATOLL_RECORDER_BUFFER_SIZE
#define ATOLL_RECORDER_BUFFER_SIZE 60
#endif

#ifndef ATOLL_RECORDER_INTERVAL
#define ATOLL_RECORDER_INTERVAL 1000
#endif

#ifndef ATOLL_RECORDER_POWER_RINGBUF_SIZE
#define ATOLL_RECORDER_POWER_RINGBUF_SIZE 32
#endif

#ifndef ATOLL_RECORDER_CADENCE_RINGBUF_SIZE
#define ATOLL_RECORDER_CADENCE_RINGBUF_SIZE 8
#endif

#ifndef ATOLL_RECORDER_HR_RINGBUF_SIZE
#define ATOLL_RECORDER_HR_RINGBUF_SIZE 8
#endif

#ifndef ATOLL_RECORDER_BASE_PATH
#define ATOLL_RECORDER_BASE_PATH "/rec"
#endif

#ifndef ATOLL_RECORDER_EXT_STATS
#define ATOLL_RECORDER_EXT_STATS ".stx"
#endif

#ifndef ATOLL_RECORDER_CONTINUE_PATH
#define ATOLL_RECORDER_CONTINUE_PATH "/rec/continue"
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

    struct Stats {
        double distance = 0.0;  // distance in meters
        uint16_t altGain = 0;   // altitude gain in meters
    };

    const char *taskName() { return "Recorder"; }
    uint16_t interval = ATOLL_RECORDER_INTERVAL;              // recording interval in milliseconds
    DataPoint buffer[ATOLL_RECORDER_BUFFER_SIZE];             // recording buffer
    uint16_t bufSize = ATOLL_RECORDER_BUFFER_SIZE;            // for convenience
    uint16_t bufIndex = 0;                                    // current buffer index
    Stats stats;                                              // current recording stats
    bool isRecording = false;                                 //
    GPS *gps = nullptr;                                       //
    FS *fs = nullptr;                                         //
    const char *basePath = ATOLL_RECORDER_BASE_PATH;          // base path to the recordings
    char filePath[32] = "";                                   // full path to the current recording
    char statsPath[32] = "";                                  // full path to the file containing the current stats
    const char *extStats = ATOLL_RECORDER_EXT_STATS;          // stats file extension
    const char *continuePath = ATOLL_RECORDER_CONTINUE_PATH;  // full path to the file containing the path to the recording to be continued after an interruption

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

    void loop() {
        static ulong lastDataPointTime = 0;
        ulong t = millis();
        if ((lastDataPointTime < t - interval) && interval < t) {
            addDataPoint();
            if (bufSize == bufIndex) {
                if (!saveBuffer())
                    log_e("could not save buffer");
                resetBuffer();
                saveStats();
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

        if (sizeof(buffer) <= bufIndex) {
            log_e("index %d out of range", bufIndex);
            return;
        }

        DataPoint *point = &buffer[bufIndex];

        point->time = mktime(&tms);
        if (-1 == point->time) {
            log_e("invalid time");
            return;
        }
        point->lat = gpsl->lat();
        point->lon = gpsl->lng();
        point->alt = (int16_t)gps->gps.altitude.meters();

        static double prevLat = 0.0;
        static double prevLon = 0.0;
        static int16_t prevAlt = 0.0;
        static bool prevPositionValid = false;
        if (prevPositionValid) {
            stats.distance += gps->gps.distanceBetween(prevLat, prevLon, point->lat, point->lon);
            if (prevAlt < point->alt) stats.altGain += point->alt - prevAlt;
        } else
            prevPositionValid = true;
        prevLat = point->lat;
        prevLon = point->lon;
        prevAlt = point->alt;

        point->power = avgPower(true);
        point->cadence = avgCadence(true);
        point->heartrate = avgHeartrate(true);
        log_i("#%d time: %d loc: %.9f %.9f alt: %dm, gain: %dm, dist: %.1fm %dW %drpm %dbpm",
              bufIndex,
              time,
              point->lat,
              point->lon,
              point->alt,
              stats.altGain,
              stats.distance,
              point->power,
              point->cadence,
              point->heartrate);

        bufIndex++;
    }

    bool saveBuffer() {
        size_t toWrite = sizeof(DataPoint) * bufIndex;
        if (0 == toWrite) {
            log_e("buffer is empty");
            return true;
        }
        log_i("saving buffer, %d entries %d bytes each, total %d bytes",
              bufIndex,
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
                  toWrite, wrote, path);
            file.close();
            return false;
        }
        log_i("wrote %d bytes to %s (%d bytes)", wrote, path, file.size());
        file.close();
        bufIndex = 0;
        return true;
    }

    bool saveStats() {
        if (stats.distance < 1.0) {
            log_i("distance is less than a meter");
            return false;
        }
        const char *sp = currentStatsPath();
        if (nullptr == sp || strlen(sp) < 1) {
            log_e("could not get current stats path");
            return false;
        }
        File file = fs->open(sp, FILE_WRITE);
        if (!file) {
            log_e("could not open %s for writing", sp);
            return false;
        }
        size_t wrote = file.write((uint8_t *)&stats, sizeof(stats));
        if (sizeof(stats) != wrote) {
            if (0 == wrote) {
                log_e("cannot write to %s", sp);
                file.close();
                return false;
            }
            log_e("stats is %d bytes but wrote only %d bytes to %s",
                  sizeof(stats), wrote, sp);
            file.close();
            return false;
        }
        log_i("wrote %d bytes to %s", wrote, sp);
        file.close();
        return true;
    }

    bool loadStats() {
        const char *sp = currentStatsPath();
        if (nullptr == sp || strlen(sp) < 1) {
            log_e("could not get current stats path");
            return false;
        }
        File file = fs->open(sp);
        if (!file) {
            log_e("could not open %s for reading", sp);
            return false;
        }
        Stats tmpStats;
        size_t read = file.read((uint8_t *)&tmpStats, sizeof(tmpStats));
        if (read < sizeof(tmpStats)) {
            log_e("cannot read from %s", sp);
            file.close();
            return false;
        }
        log_i("read %d bytes from %s", read, sp);
        file.close();
        stats.distance += tmpStats.distance;
        log_i("distance: %.0f, altGain: %d", stats.distance, stats.altGain);
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
        file = fs->open(continuePath);
        if (file) {
            char testPath[sizeof(filePath)] = "";
            file.read((uint8_t *)testPath, sizeof(testPath));
            file.close();
            if (strlen(basePath) + 5 <= strlen(testPath)) {
                if (fs->exists(testPath)) {
                    file = fs->open(testPath, FILE_APPEND);
                    if (file) {
                        file.close();
                        strncpy(filePath, testPath, sizeof(filePath));
                        log_i("continuing recording of %s", filePath);
                        loadStats();
                        return filePath;
                    }
                }
            }
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
        snprintf(filePath, sizeof(filePath), "%s/%02d%02d%02d%02d",
                 basePath,
                 tms->tm_mon + 1,
                 tms->tm_mday,
                 tms->tm_hour,
                 tms->tm_min);
        log_i("recording to %s", filePath);
        file = fs->open(continuePath, FILE_WRITE);
        if (file) {
            if (file.write((uint8_t *)filePath, strlen(filePath)) == strlen(filePath))
                log_i("wrote continue file %s", continuePath);
            else
                log_e("could not write continue file %s", continuePath);
            file.close();
        }
        return filePath;
    }

    const char *currentStatsPath() {
        if (0 != strcmp(statsPath, ""))
            return statsPath;

        char *fp = currentFilePath();
        if (nullptr == fp) {
            log_i("could not get current file path");
            return nullptr;
        }
        snprintf(statsPath, sizeof(statsPath), "%s%s", fp, extStats);
        return statsPath;
    }

    void resetBuffer(bool clearPoints = false) {
        bufIndex = 0;
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

    bool stop(bool forgetLast = true) {
        if (!isRecording) return false;
        log_i("stopping recording");
        isRecording = false;
        if (!saveBuffer())
            log_e("could not save buffer");
        resetBuffer();
        saveStats();
        strncpy(filePath, "", sizeof(filePath));
        if (forgetLast) {
            fs->remove(continuePath);
            stats = Stats();
        }
        return true;
    }

    CircularBuffer<uint16_t, ATOLL_RECORDER_POWER_RINGBUF_SIZE> powerBuf;
    SemaphoreHandle_t powerMutex = xSemaphoreCreateMutex();

    void onPower(uint16_t value) {
        if (!aquireMutex(powerMutex)) {
            log_e("could not aquire powerMutex to add %d", value);
            return;
        }
        powerBuf.push(value);
        releaseMutex(powerMutex);
    }

    // TODO fix CircularBuffer.avg()
    uint16_t avgPower(bool clearBuffer = false) {
        if (!aquireMutex(powerMutex)) {
            log_e("could not aquire powerMutex to compute avg");
            return 0;
        }
        if (powerBuf.isEmpty()) {
            releaseMutex(powerMutex);
            return 0;
        }
        uint16_t avg = 0;
        for (decltype(powerBuf)::index_t i = 0; i < powerBuf.size(); i++)
            avg += powerBuf[i] / powerBuf.size();
        if (clearBuffer) powerBuf.clear();
        releaseMutex(powerMutex);
        return avg;
    }

    CircularBuffer<uint8_t, ATOLL_RECORDER_CADENCE_RINGBUF_SIZE> cadenceBuf;
    SemaphoreHandle_t cadenceMutex = xSemaphoreCreateMutex();

    void onCadence(uint8_t value) {
        if (!aquireMutex(cadenceMutex)) {
            log_e("could not aquire cadenceMutex to add %d", value);
            return;
        }
        cadenceBuf.push(value);
        releaseMutex(cadenceMutex);
    }

    // TODO fix CircularBuffer.avg()
    uint8_t avgCadence(bool clearBuffer = false) {
        if (!aquireMutex(cadenceMutex)) {
            log_e("could not aquire cadenceMutex to compute avg");
            return 0;
        }
        if (cadenceBuf.isEmpty()) {
            releaseMutex(cadenceMutex);
            return 0;
        }
        uint8_t avg = 0;
        for (decltype(cadenceBuf)::index_t i = 0; i < cadenceBuf.size(); i++)
            avg += cadenceBuf[i] / cadenceBuf.size();
        if (clearBuffer) cadenceBuf.clear();
        releaseMutex(cadenceMutex);
        return avg;
    }

    CircularBuffer<uint8_t, ATOLL_RECORDER_HR_RINGBUF_SIZE> heartrateBuf;
    SemaphoreHandle_t heartrateMutex = xSemaphoreCreateMutex();

    void onHeartrate(uint8_t value) {
        if (!aquireMutex(heartrateMutex)) {
            log_e("could not aquire heartrateMutex to add %d", value);
            return;
        }
        heartrateBuf.push(value);
        releaseMutex(heartrateMutex);
    }

    // TODO fix CircularBuffer.avg()
    uint8_t avgHeartrate(bool clearBuffer = false) {
        if (!aquireMutex(heartrateMutex)) {
            log_e("could not aquire heartrateMutex to compute avg");
            return 0;
        }
        if (heartrateBuf.isEmpty()) {
            releaseMutex(heartrateMutex);
            return 0;
        }
        uint8_t avg = 0;
        for (decltype(heartrateBuf)::index_t i = 0; i < heartrateBuf.size(); i++)
            avg += heartrateBuf[i] / heartrateBuf.size();
        if (clearBuffer) heartrateBuf.clear();
        releaseMutex(heartrateMutex);
        return avg;
    }

    bool aquireMutex(SemaphoreHandle_t mutex, uint32_t timeout = 100) {
        // log_i("aquireMutex %d", mutex);
        if (xSemaphoreTake(mutex, (TickType_t)timeout) == pdTRUE)
            return true;
        log_e("Could not aquire mutex");
        return false;
    }

    void releaseMutex(SemaphoreHandle_t mutex) {
        // log_i("releaseMutex %d", mutex);
        xSemaphoreGive(mutex);
    }
};

}  // namespace Atoll

#endif