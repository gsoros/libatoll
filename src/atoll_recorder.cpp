#include "atoll_recorder.h"
#include "atoll_serial.h"

using namespace Atoll;

Recorder *Recorder::instance = nullptr;

void Recorder::setup(GPS *gps,
                     Fs *fs,
                     Api *api,
                     Recorder *instance) {
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
    this->fs = fs->pFs();
    if (nullptr == instance) return;
    this->instance = instance;
    this->api = api;
    if (nullptr != api)
        api->addCommand(ApiCommand("rec", recProcessor));
}

void Recorder::loop() {
    static ulong lastDataPointTime = 0;
    ulong t = millis();
    if ((lastDataPointTime < t - interval) && interval < t) {
        addDataPoint();
        if (bufIndex < bufSize) return;
        if (!saveBuffer()) {
            log_e("could not save buffer");
            return;
        }
        resetBuffer();
        if (!saveStats())
            log_e("could not save stats");
    }
}

void Recorder::addDataPoint() {
    if (!isRecording) return;
    if (nullptr == gps) {
        log_e("no gps");
        return;
    }
    if (!systemTimeLastSet()) {
        log_i("not adding data point, waiting for system time update");
        return;
    }
    // if (!gps->gps.location.isValid()) {
    //     log_i("gps location invalid");
    //     return;
    // }
    if (!gps->isMoving()) {
        return;
    }
    if (sizeof(buffer) <= bufIndex) {
        log_e("index %d out of range", bufIndex);
        return;
    }

    static double prevLat = 0.0;
    static double prevLon = 0.0;
    static bool prevPositionValid = false;
    static int16_t prevAlt = 0;
    static bool prevAltValid = false;

    buffer[bufIndex] = DataPoint();  // clear
    DataPoint *point = &buffer[bufIndex];
    point->time = time(nullptr);

    if (gps->gps.location.isValid()) {
        point->flags |= Flags.location;
        point->lat = gps->gps.location.lat();
        point->lon = gps->gps.location.lng();
        if (prevPositionValid) {
            double diff = gps->gps.distanceBetween(
                prevLat, prevLon, point->lat, point->lon);
            stats.distance += diff;
            // log_i("diff: %f", diff);
            if (0.01 < diff) onDistanceChanged(stats.distance);
        } else {
            prevPositionValid = true;
        }
        prevLat = point->lat;
        prevLon = point->lon;
    }
    if (gps->gps.altitude.isValid()) {
        point->flags |= Flags.altitude;
        point->altitude = (int16_t)gps->gps.altitude.meters();
        if (prevAltValid) {
            if (prevAlt < point->altitude) {
                stats.altGain += point->altitude - prevAlt;
                onAltGainChanged(stats.altGain);
            }
        } else {
            prevAltValid = true;
        }
        prevAlt = point->altitude;
    }

    int16_t avg = avgPower(true);
    if (0 <= avg) {
        point->flags |= Flags.power;
        point->power = avg;
    }  // else log_i("avgPower returned < 0");
    avg = avgCadence(true);
    if (0 <= avg) {
        point->flags |= Flags.cadence;
        point->cadence = avg;
    }  // else log_i("avgCadence returned < 0");
    avg = avgHeartrate(true);
    if (0 <= avg) {
        point->flags |= Flags.heartrate;
        point->heartrate = avg;
    }  // else log_i("avgHr returned < 0");

    log_i("#%2d T%ld F%d %.7f %.7f ^%d+%dm >%.1fm P%4d C%3d H%3d",
          bufIndex,
          point->time,
          point->flags,
          point->lat,
          point->lon,
          point->altitude,
          stats.altGain,
          stats.distance,
          point->power,
          point->cadence,
          point->heartrate);

    bufIndex++;
}

bool Recorder::saveBuffer() {
    size_t toWrite = sizeof(DataPoint) * bufIndex;
    if (0 == toWrite) {
        log_e("buffer is empty");
        return true;
    }
    if (0 == buffer[0].time) {
        log_e("could not get time from first datapoint");
        return false;
    }
    // log_i("saving buffer, %d entries %d bytes each, total %d bytes",
    //       bufIndex,
    //       sizeof(DataPoint),
    //       toWrite);
    if (nullptr == fs) {
        log_e("no fs");
        return false;
    }
    const char *path = currentPath();
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
    log_i("wrote %d bytes to %s (was %d bytes)", wrote, path, file.size());
    file.close();
    bufIndex = 0;
    return true;
}

bool Recorder::saveStats() {
    if (stats.distance < 1.0) {
        log_i("distance is less than a meter");
        // return false;
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
    // log_i("wrote %d bytes to %s", wrote, sp);
    file.close();
    return true;
}

bool Recorder::loadStats(bool reportFail) {
    const char *sp = currentStatsPath();
    if (nullptr == sp || strlen(sp) < 1) {
        if (reportFail) log_e("could not get current stats path");
        return false;
    }
    File file;
    if (fs->exists(sp))
        file = fs->open(sp);
    if (!file) {
        if (reportFail) log_e("could not open %s for reading", sp);
        return false;
    }
    Stats tmpStats;
    size_t read = file.read((uint8_t *)&tmpStats, sizeof(tmpStats));
    if (read < sizeof(tmpStats)) {
        if (reportFail) log_e("cannot read from %s", sp);
        file.close();
        return false;
    }
    log_i("read %d bytes from %s", read, sp);
    file.close();
    stats.distance += tmpStats.distance;
    stats.altGain += tmpStats.altGain;
    onDistanceChanged(stats.distance);
    onAltGainChanged(stats.altGain);
    log_i("distance: %.1f, altGain: %d", stats.distance, stats.altGain);
    return true;
}

// reset or get full path to the current recording or null
const char *Recorder::currentPath(bool reset) {
    static char path[ATOLL_RECORDER_PATH_LENGTH] = "";
    if (reset) {
        strncpy(path, "", sizeof(path));
        return nullptr;
    }
    if (!isRecording) return nullptr;
    if (0 != strcmp(path, ""))
        return path;
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
    if (fs->exists(continuePath))
        file = fs->open(continuePath);
    if (file) {
        char testPath[sizeof(path)] = "";
        file.read((uint8_t *)testPath, sizeof(testPath));
        file.close();
        if (strlen(basePath) + 5 <= strlen(testPath)) {
            if (fs->exists(testPath)) {
                file = fs->open(testPath, FILE_APPEND);
                if (file) {
                    if (0 == file.size() % sizeof(DataPoint)) {
                        file.close();
                        strncpy(path, testPath, sizeof(path));
                        log_i("continuing recording of %s", path);
                        // loadStats(); already called by start()
                        return path;
                    } else
                        log_e("%s size %d is not multiple of %d (corrupt file?)",
                              path, file.size(), sizeof(DataPoint));
                    file.close();
                }
            }
        }
    }
    time_t t = buffer[0].time;
    if (0 == t) {
        if (!systemTimeLastSet()) {
            log_e("system time has not been set");
            return nullptr;
        }
        t = now();
        log_i("could not get time from first datapoint, using current time");
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
    struct tm *tms = localtime(&t);
    snprintf(path, sizeof(path), "%s/%02d%02d%02d%02d",
             basePath,
             tms->tm_mon + 1,
             tms->tm_mday,
             tms->tm_hour,
             tms->tm_min);
    log_i("recording to %s", path);
    file = fs->open(continuePath, FILE_WRITE);
    if (file) {
        if (file.write((uint8_t *)path, strlen(path)) == strlen(path))
            log_i("wrote '%s' to %s", path, continuePath);
        else
            log_e("could not write '%s' to %s", path, continuePath);
        file.close();
    } else
        log_e("could not open %s for writing", continuePath);
    return path;
}

// reset or get full path to the file containing the current stats or null
const char *Recorder::currentStatsPath(bool reset) {
    static char path[ATOLL_RECORDER_PATH_LENGTH] = "";
    if (reset) {
        strncpy(path, "", sizeof(path));
        return nullptr;
    }
    if (0 != strcmp(path, ""))
        return path;
    const char *fp = currentPath();
    if (nullptr == fp) {
        log_i("could not get current file path");
        return nullptr;
    }
    snprintf(path, sizeof(path), "%s%s", fp, statsExt);
    return path;
}

void Recorder::resetBuffer(bool clearPoints) {
    bufIndex = 0;
    if (clearPoints)
        for (uint16_t i = 0; i < bufSize; i++)
            buffer[i] = DataPoint();
}

bool Recorder::resume() { return start(); }

bool Recorder::start() {
    if (nullptr == fs) {
        log_e("no fs");
        return false;
    }
    log_i("starting recording");
    if (isRecording) {
        log_e("already recording");
        return false;
    }
    currentPath(true);  // reset
    resetBuffer(true);
    isRecording = true;
    loadStats(false);
    return true;
}

bool Recorder::pause() { return stop(false); }

bool Recorder::end() { return stop(true); }

bool Recorder::stop(bool forgetLast) {
    if (!isRecording) return false;
    log_i("%sing recording", forgetLast ? "stopp" : "paus");
    if (!saveBuffer())
        log_e("could not save buffer");
    if (!saveStats())
        log_e("could not save stats");
    char recPath[strlen(currentPath()) + 1] = "";
    char gpxPath[sizeof(recPath) + 5] = "";
    if (forgetLast) {
        strncpy(recPath, currentPath(), sizeof(recPath));
        snprintf(gpxPath, sizeof(gpxPath), "%s.gpx", recPath);
        log_i("rec: %s gpx: %s", recPath, gpxPath);
    }
    isRecording = false;
    resetBuffer();
    currentPath(true);       // reset
    currentStatsPath(true);  // reset
    if (forgetLast) {
        log_i("recording end");
        fs->remove(continuePath);
        stats = Stats();
        rec2gpx(recPath, gpxPath);
        // some datapoints may have been created since we started writing the gpx file
        resetBuffer();
        currentPath(true);       // reset
        currentStatsPath(true);  // reset
        onDistanceChanged(0.0);
        onAltGainChanged(0);
    }
    return true;
}

void Recorder::onPower(uint16_t value) {
    if (!aquireMutex(powerMutex)) {
        log_e("could not aquire powerMutex to add %d", value);
        return;
    }
    powerBuf.push(value);
    releaseMutex(powerMutex);
}

// returns average of powerBuf or -1 on error or if buf is empty
int16_t Recorder::avgPower(bool clearBuffer) {
    if (!aquireMutex(powerMutex)) {
        log_e("could not aquire powerMutex to compute avg");
        return -1;
    }
    if (powerBuf.isEmpty()) {
        releaseMutex(powerMutex);
        return -1;
    }
    uint16_t avg = 0;
    for (decltype(powerBuf)::index_t i = 0; i < powerBuf.size(); i++)
        avg += powerBuf[i] / powerBuf.size();
    if (clearBuffer) powerBuf.clear();
    releaseMutex(powerMutex);
    if (INT16_MAX < avg) return -1;
    return (int16_t)avg;
}

void Recorder::onCadence(uint8_t value) {
    if (!aquireMutex(cadenceMutex)) {
        log_e("could not aquire cadenceMutex to add %d", value);
        return;
    }
    cadenceBuf.push(value);
    releaseMutex(cadenceMutex);
}

// returns average of cadenceBuf or -1 on error or if buf is empty
int16_t Recorder::avgCadence(bool clearBuffer) {
    if (!aquireMutex(cadenceMutex)) {
        log_e("could not aquire cadenceMutex to compute avg");
        return -1;
    }
    if (cadenceBuf.isEmpty()) {
        releaseMutex(cadenceMutex);
        return -1;
    }
    uint8_t avg = 0;
    for (decltype(cadenceBuf)::index_t i = 0; i < cadenceBuf.size(); i++)
        avg += cadenceBuf[i] / cadenceBuf.size();
    if (clearBuffer) cadenceBuf.clear();
    releaseMutex(cadenceMutex);
    return (int16_t)avg;
}

void Recorder::onHeartrate(uint8_t value) {
    if (!aquireMutex(heartrateMutex)) {
        log_e("could not aquire heartrateMutex to add %d", value);
        return;
    }
    heartrateBuf.push(value);
    releaseMutex(heartrateMutex);
}

// returns average of heartrateBuf or -1 on error or if buf is empty
int16_t Recorder::avgHeartrate(bool clearBuffer) {
    if (!aquireMutex(heartrateMutex)) {
        log_e("could not aquire heartrateMutex to compute avg");
        return -1;
    }
    if (heartrateBuf.isEmpty()) {
        releaseMutex(heartrateMutex);
        return -1;
    }
    uint8_t avg = 0;
    for (decltype(heartrateBuf)::index_t i = 0; i < heartrateBuf.size(); i++)
        avg += heartrateBuf[i] / heartrateBuf.size();
    if (clearBuffer) heartrateBuf.clear();
    releaseMutex(heartrateMutex);
    return (int16_t)avg;
}

bool Recorder::aquireMutex(SemaphoreHandle_t mutex, uint32_t timeout) {
    // log_i("aquireMutex %d", mutex);
    if (xSemaphoreTake(mutex, (TickType_t)timeout) == pdTRUE)
        return true;
    log_e("Could not aquire mutex");
    return false;
}

void Recorder::releaseMutex(SemaphoreHandle_t mutex) {
    // log_i("releaseMutex %d", mutex);
    xSemaphoreGive(mutex);
}

// creates non-standard gpx that can be parsed by Str*v*
bool Recorder::rec2gpx(const char *recPath, const char *gpxPathIn) {
    if (nullptr == instance) {
        log_e("instance is null");
        return false;
    }
    fs::FS *fs = instance->fs;
    if (nullptr == fs) {
        log_e("fs is null");
        return false;
    }
    if (!fs->exists(recPath)) {
        log_e("%s does not exist", recPath);
        return false;
    }

    char gpxPath[strlen(gpxPathIn) + 1] = "";
    strncat(gpxPath, gpxPathIn, sizeof(gpxPath));
    if (fs->exists(gpxPath)) {
        log_e("%s already exists", gpxPath);
        // return false;
        bool found = false;
        char tmpPath[strlen(recPath) + 1] = "";
        char testPath[sizeof(tmpPath) + 5] = "";
        strncat(tmpPath, recPath, sizeof(tmpPath) - 2);
        const char *pool = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVXYZ";
        for (uint8_t i = 0; i < strlen(pool); i++) {
            snprintf(testPath, sizeof(testPath), "%s%c.gpx", tmpPath, pool[i]);
            log_i("testing %s", testPath);
            if (!fs->exists(testPath)) {
                found = true;
                log_i("%s does not exist");
                strncpy(gpxPath, testPath, sizeof(gpxPath));
                break;
            }
        }
        if (!found) {
            log_e("could not create unused gpx file name");
            return false;
        }
    }
    File rec = fs->open(recPath);
    if (!rec) {
        log_e("could not open %s", recPath);
        return false;
    }
    File gpx = fs->open(gpxPath, FILE_WRITE);
    if (!gpx) {
        log_e("could not open %s", gpxPath);
        rec.close();
        return false;
    }
    log_i("rec: %s gpx: %s", recPath, gpxPath);
    log_i("creating %s", gpxPath);

    const char *header = R"====(<?xml version="1.0" encoding="UTF-8"?>
<gpx creator="libAtoll" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" xsi:schemaLocation="http://www.topografix.com/GPX/1/1 http://www.topografix.com/GPX/1/1/gpx.xsd http://www.garmin.com/xmlschemas/GpxExtensions/v3 http://www.garmin.com/xmlschemas/GpxExtensionsv3.xsd http://www.garmin.com/xmlschemas/TrackPointExtension/v1 http://www.garmin.com/xmlschemas/TrackPointExtensionv1.xsd" version="1.1" xmlns="http://www.topografix.com/GPX/1/1" xmlns:gpxtpx="http://www.garmin.com/xmlschemas/TrackPointExtension/v1" xmlns:gpxx="http://www.garmin.com/xmlschemas/GpxExtensions/v3">)====";

    const char *metaTrkFormat = R"====(
  <metadata>
    <time>%s</time>
  </metadata>
  <trk>
    <name>ride</name>
    <type>1</type>
    <trkseg>)====";

    // TODO break up tags
    const char *pointFormat = R"====(
      <trkpt lat="%.7f" lon="%.7f">
        <ele>%d</ele>
        <time>%s</time>
        <extensions>
          <power>%d</power>
          <gpxtpx:TrackPointExtension>
          <gpxtpx:hr>%d</gpxtpx:hr>
          <gpxtpx:cad>%d</gpxtpx:cad>
          </gpxtpx:TrackPointExtension>
        </extensions>
      </trkpt>)====";

    const char *footer = R"====(
    </trkseg>
  </trk>
</gpx>)====";

    if (gpx.write((uint8_t *)header, strlen(header)) != strlen(header)) {
        log_e("could not write header to %s", gpxPath);
        rec.close();
        gpx.close();
        fs->remove(gpxPath);
        return false;
    }
    // Serial.print(header);
    DataPoint point;

    char time[21] = "";  // ISO 8601 (2022-03-25T12:58:13Z)
    struct tm tms;
    char pointBuf[strlen(pointFormat)  //
                  + 10                 // lat (0...90).(7 decimals)
                  + 11                 // lon (0...180).(7 decimals)
                  + 5                  // altitude (uint16)
                  + sizeof(time)       //
                  + 5                  // power (uint16)
                  + 3                  // heartrate (uint8)
                  + 3                  // cadence (uint8)
    ];
    uint16_t pointBufLen = 0;
    bool metaTrkAdded = false;
    uint32_t points = 0;
    time_t prevTime = 0;
    while (rec.read((uint8_t *)&point, sizeof(point)) == sizeof(point)) {
        if (0 == point.time) {
            log_e("point %d time is zero", points);
            continue;
        }
        if (point.time < prevTime)
            log_e("point #%d time < prevTime", points);
        prevTime = point.time;
        gmtime_r(&point.time, &tms);
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
        snprintf(time, sizeof(time), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 tms.tm_year + 1900,
                 tms.tm_mon + 1,
                 tms.tm_mday,
                 tms.tm_hour,
                 tms.tm_min,
                 tms.tm_sec);
        if (!metaTrkAdded) {
            char metaTrk[strlen(metaTrkFormat) + sizeof(time)];
            snprintf(metaTrk, sizeof(metaTrk), metaTrkFormat, time);
            if (gpx.write((uint8_t *)&metaTrk, strlen(metaTrk)) != strlen(metaTrk)) {
                log_e("could not write meta section to %s", gpxPath);
                rec.close();
                gpx.close();
                fs->remove(gpxPath);
                return false;
            }
            metaTrkAdded = true;
            // Serial.print(metaTrk);
        }
        // TODO only add lat, lon, altitude, power, cadence, heartrate if flags present
        snprintf(pointBuf, sizeof(pointBuf), pointFormat,
                 point.lat,
                 point.lon,
                 point.altitude,
                 time,
                 point.power,
                 point.heartrate,
                 point.cadence);
        pointBufLen = strlen(pointBuf);
        if (gpx.write((uint8_t *)&pointBuf, pointBufLen) != pointBufLen) {
            log_e("could not write point #%d to %s", points, gpxPath);
            continue;
        }
        // Serial.print(pointBuf);
        points++;
    }
    if (gpx.write((uint8_t *)footer, strlen(footer)) != strlen(footer)) {
        log_e("could not write footer to %s", gpxPath);
        rec.close();
        gpx.close();
        fs->remove(gpxPath);
        return false;
    }
    // Serial.print(footer);
    rec.close();
    gpx.close();  // need to reopen to get size
    gpx = fs->open(gpxPath);
    if (!gpx) {
        log_e("could not open %s", gpxPath);
        gpx.close();
        return false;
    }
    log_i("%s created, size: %d bytes", gpxPath, gpx.size());
    gpx.close();
    return true;
}

ApiResult *Recorder::recProcessor(ApiReply *reply) {
    if (nullptr == instance) return Api::error();
    if (0 <= strlen(reply->arg)) {
    }
    return Api::success();
}