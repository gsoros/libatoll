#ifdef FEATURE_RECORDER

#include "atoll_recorder.h"
#include "atoll_serial.h"
#include "atoll_time.h"

using namespace Atoll;

Recorder *Recorder::instance = nullptr;

void Recorder::setup(GPS *gps,
                     Fs *device,
                     Api *api,
                     Recorder *instance) {
    powerBuf.clear();
    cadenceBuf.clear();
    heartrateBuf.clear();

    if (nullptr == gps) {
        log_e("no gps");
        return;
    }
    this->gps = gps;
    if (nullptr == device) {
        log_e("no device");
        return;
    }
    this->device = device;
    if (!device->mounted) {
        log_e("device fs not mounted");
        return;
    }
    this->fs = device->pFs();
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
            // return;
        }
        resetBuffer();
        if (!saveStats())
            log_e("could not save stats");
    }
}

void Recorder::addDataPoint() {
    if (!isRecording) return;
    if (!gps) {
        log_e("no gps");
        return;
    }
    // if (!gps->gps.location.isValid()) {
    //     log_i("gps location invalid");
    //     return;
    // }
    if (!gps->isMoving()) {
        return;
    }
    if (!systemTimeLastSet()) {
        log_i("not adding data point, waiting for system time update");
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

    if (gps->device.location.isValid()) {
        point->flags |= Flags.location;
        point->lat = gps->device.location.lat();
        point->lon = gps->device.location.lng();
        if (prevPositionValid) {
            double diff = gps->device.distanceBetween(
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
    if (gps->device.altitude.isValid()) {
        point->flags |= Flags.altitude;
        point->altitude = (int16_t)gps->device.altitude.meters();
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

    // char bufDump[sizeof(DataPoint) * 5] = "";
    // char bufDec[4] = "";
    // uint8_t *p = (uint8_t *)point;
    // for (size_t i = 0; i < sizeof(DataPoint); i++) {
    //     if (0 < i) strncat(bufDump, ", ", sizeof(bufDump) - strlen(bufDump) - 1);
    //     snprintf(bufDec, sizeof(bufDec), "%d", p[i]);
    //     strncat(bufDump, bufDec, sizeof(bufDump) - strlen(bufDump) - 1);
    // }
    // log_i("Point time: %d, dump: %s", point->time, bufDump);

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
    if (!device->aquireMutex()) {
        log_e("could not aquire mutex");
        return false;
    }
    File file = fs->open(path, FILE_APPEND);
    if (!file) {
        log_e("could not open %s for appending", path);
        device->releaseMutex();
        return false;
    }
    size_t wrote = file.write((uint8_t *)buffer, toWrite);
    if (toWrite != wrote) {
        if (0 == wrote) {
            log_e("cannot write to %s", path);
            file.close();
            device->releaseMutex();
            return false;
        }
        log_e("buffer is %d bytes but wrote only %d bytes to %s",
              toWrite, wrote, path);
        file.close();
        device->releaseMutex();
        return false;
    }
    log_i("wrote %d bytes to %s (was %d bytes)", wrote, path, file.size());
    file.close();
    device->releaseMutex();
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
    if (!device->aquireMutex()) {
        log_e("could not aquire mutex");
        return false;
    }
    File file = fs->open(sp, FILE_WRITE);
    if (!file) {
        log_e("could not open %s for writing", sp);
        device->releaseMutex();
        return false;
    }
    size_t wrote = file.write((uint8_t *)&stats, sizeof(stats));
    if (sizeof(stats) != wrote) {
        if (0 == wrote) {
            log_e("cannot write to %s", sp);
            file.close();
            device->releaseMutex();
            return false;
        }
        log_e("stats is %d bytes but wrote only %d bytes to %s",
              sizeof(stats), wrote, sp);
        file.close();
        device->releaseMutex();
        return false;
    }
    log_i("wrote %d bytes to %s", wrote, sp);
    file.close();
    device->releaseMutex();
    return true;
}

bool Recorder::loadStats(bool reportFail) {
    const char *sp = currentStatsPath();
    if (nullptr == sp || strlen(sp) < 1) {
        if (reportFail) log_e("could not get current stats path");
        return false;
    }
    File file;
    if (!device->aquireMutex()) {
        if (reportFail) log_e("could not aquire mutex");
        return false;
    }
    if (fs->exists(sp))
        file = fs->open(sp);
    if (!file) {
        if (reportFail) log_e("could not open %s for reading", sp);
        device->releaseMutex();
        return false;
    }
    Stats tmpStats;
    size_t read = file.read((uint8_t *)&tmpStats, sizeof(tmpStats));
    if (read < sizeof(tmpStats)) {
        if (reportFail) log_e("cannot read from %s", sp);
        file.close();
        device->releaseMutex();
        return false;
    }
    log_i("read %d bytes from %s", read, sp);
    file.close();
    device->releaseMutex();
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
    if (!device->aquireMutex()) {
        log_e("could not aquire mutex");
        return nullptr;
    }
    File file = fs->open(basePath);
    if (!file) {
        if (!fs->mkdir(basePath)) {
            log_e("could not create %s", basePath);
            device->releaseMutex();
            return nullptr;
        }
        file = fs->open(basePath);
    }
    if (!file) {
        log_e("could not open %s", basePath);
        device->releaseMutex();
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
                        device->releaseMutex();
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
            log_i("system time has not been set");
            device->releaseMutex();
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
    device->releaseMutex();
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

// append stats extension to path
int Recorder::appendStatsExt(char *path, size_t size) {
    log_i("path: %s, size: %d", path, size);
    if (nullptr == path || !size) return 0;
    char sp[size] = "";
    int ret = snprintf(sp, size, "%s%s", path, statsExt);
    snprintf(path, size, "%s", sp);
    log_i("path: %s, ret: %d", path, ret);
    return ret;
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
    const char *cp = currentPath();
    uint8_t cpLen = 0;
    char recPath[nullptr == cp ? 1 : (strlen(cp) + 1)] = "";
    char gpxPath[sizeof(recPath) + 5] = "";
    if (nullptr == cp) {
        if (forgetLast) {
            log_i("currentPath() is null, not creating gpx");
        }
    } else {
        if (forgetLast) {
            cpLen = strlen(cp);
            strncpy(recPath, cp, sizeof(recPath));
            snprintf(gpxPath, sizeof(gpxPath), "%s.gpx", recPath);
            log_i("rec: %s gpx: %s", recPath, gpxPath);
        }
    }
    isRecording = false;
    resetBuffer();
    currentPath(true);       // reset
    currentStatsPath(true);  // reset
    if (forgetLast) {
        log_i("recording end");
        if (nullptr == fs)
            log_e("no fs");
        else if (!device->aquireMutex())
            log_e("could not aquire mutex");
        else {
            fs->remove(continuePath);
            device->releaseMutex();
        }
        stats = Stats();
        if (cpLen)
            rec2gpx(recPath, gpxPath);
        // some datapoints may have been created since we started writing the gpx file
        resetBuffer();
        currentPath(true);       // reset
        currentStatsPath(true);  // reset
        onDistanceChanged(stats.distance);
        onAltGainChanged(stats.altGain);
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
bool Recorder::rec2gpx(const char *recPath,
                       const char *gpxPathIn,
                       bool overwrite) {
    if (nullptr == instance) {
        log_e("instance is null");
        return false;
    }
    fs::FS *fs = instance->fs;
    if (nullptr == fs) {
        log_e("fs is null");
        return false;
    }
    if (!device->aquireMutex()) {
        log_e("could not aquire mutex");
        return false;
    }
    if (!fs->exists(recPath)) {
        log_e("%s does not exist", recPath);
        device->releaseMutex();
        return false;
    }

    char gpxPath[strlen(gpxPathIn) + 1] = "";
    strncat(gpxPath, gpxPathIn, sizeof(gpxPath));
    if (fs->exists(gpxPath)) {
        log_i("%s already exists", gpxPath);
        if (overwrite) {
            log_w("overwriting %s", gpxPath);
        } else {
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
                device->releaseMutex();
                return false;
            }
        }
    }
    File rec = fs->open(recPath);
    if (!rec) {
        log_e("could not open %s", recPath);
        device->releaseMutex();
        return false;
    }
    File gpx = fs->open(gpxPath, FILE_WRITE);
    if (!gpx) {
        log_e("could not open %s", gpxPath);
        rec.close();
        device->releaseMutex();
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

    // const char *pointFormat = R"====(
    //   <trkpt lat="%.7f" lon="%.7f">
    //     <time>%s</time>
    //     <ele>%d</ele>
    //     <extensions>
    //       <power>%d</power>
    //       <gpxtpx:TrackPointExtension>
    //         <gpxtpx:hr>%d</gpxtpx:hr>
    //         <gpxtpx:cad>%d</gpxtpx:cad>
    //       </gpxtpx:TrackPointExtension>
    //     </extensions>
    //   </trkpt>)====";

    const char *pointFormat = R"====(
      <trkpt%s>
        <time>%s</time>%s%s
      </trkpt>)====";

    const char *locationFormat = R"====( lat="%.7f" lon="%.7f")====";

    const char *altFormat = R"====(
        <ele>%d</ele>)====";

    const char *extFormat = R"====(
        <extensions>%s%s
        </extensions>)====";

    const char *powerFormat = R"====(
          <power>%d</power>)====";

    const char *tpxFormat = R"====(
          <gpxtpx:TrackPointExtension>%s%s
          </gpxtpx:TrackPointExtension>)====";

    const char *hrFormat = R"====(
            <gpxtpx:hr>%d</gpxtpx:hr>)====";

    const char *cadFormat = R"====(
            <gpxtpx:cad>%d</gpxtpx:cad>)====";

    const char *footer = R"====(
    </trkseg>
  </trk>
</gpx>)====";

    if (gpx.write((uint8_t *)header, strlen(header)) != strlen(header)) {
        log_e("could not write header to %s", gpxPath);
        rec.close();
        gpx.close();
        fs->remove(gpxPath);
        device->releaseMutex();
        return false;
    }

    device->releaseMutex();
    // Serial.print(header);
    DataPoint point;

    char timeBuf[21] = "";  // ISO 8601 (2022-03-25T12:58:13Z)
    struct tm tms;
    char locationBuf[           //
        strlen(locationFormat)  //
        + 10                    // lat (0...90).(7 decimals)
        + 11                    // lon (0...180).(7 decimals)
    ] = "";
    char altBuf[           //
        strlen(altFormat)  //
        + 5                // uint16 altitude
    ] = "";
    char powerBuf[           //
        strlen(powerFormat)  //
        + 5                  // uint16 power
    ] = "";
    char hrBuf[           //
        strlen(hrFormat)  //
        + 3               // uint8 heartrate
    ] = "";
    char cadBuf[           //
        strlen(cadFormat)  //
        + 3                // uint8 cadence
    ] = "";
    char tpxBuf[           //
        strlen(tpxFormat)  //
        + sizeof(hrBuf)    //
        + sizeof(cadBuf)   //
    ] = "";
    char extBuf[            //
        strlen(extFormat)   //
        + sizeof(powerBuf)  //
        + sizeof(tpxBuf)    //
    ] = "";
    char pointBuf[             //
        strlen(pointFormat)    //
        + sizeof(locationBuf)  //
        + sizeof(timeBuf)      //
        + sizeof(altBuf)       //
        + sizeof(extBuf)       //
    ] = "";
    uint16_t pointBufLen = 0;
    bool metaTrkAdded = false;
    uint32_t points = 0;
    time_t prevTime = 0;
    while (rec.read((uint8_t *)&point, sizeof(point)) == sizeof(point)) {
        if (!device->aquireMutex()) {
            log_e("could not aquire mutex");
            continue;
        }
        if (0 == point.time) {
            log_e("point %d time is zero", points);
            device->releaseMutex();
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
        snprintf(timeBuf, sizeof(timeBuf), "%04d-%02d-%02dT%02d:%02d:%02dZ",
                 tms.tm_year + 1900,
                 tms.tm_mon + 1,
                 tms.tm_mday,
                 tms.tm_hour,
                 tms.tm_min,
                 tms.tm_sec);
        if (!metaTrkAdded) {
            char metaTrk[strlen(metaTrkFormat) + sizeof(timeBuf)];
            snprintf(metaTrk, sizeof(metaTrk), metaTrkFormat, timeBuf);
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

        if (point.flags & Flags.location)
            snprintf(locationBuf, sizeof(locationBuf), locationFormat, point.lat, point.lon);
        else
            strncpy(locationBuf, "", sizeof(locationBuf));

        if (point.flags & Flags.altitude)
            snprintf(altBuf, sizeof(altBuf), altFormat, point.altitude);
        else
            strncpy(altBuf, "", sizeof(altBuf));

        if (point.flags & Flags.power)
            snprintf(powerBuf, sizeof(powerBuf), powerFormat, point.power);
        else
            strncpy(powerBuf, "", sizeof(powerBuf));

        if (point.flags & Flags.heartrate)
            snprintf(hrBuf, sizeof(hrBuf), hrFormat, point.heartrate);
        else
            strncpy(hrBuf, "", sizeof(hrBuf));

        if (point.flags & Flags.cadence)
            snprintf(cadBuf, sizeof(cadBuf), cadFormat, point.cadence);
        else
            strncpy(cadBuf, "", sizeof(cadBuf));

        if (0 < strlen(powerBuf) || 0 < strlen(hrBuf) || 0 < strlen(cadBuf)) {
            if (0 < strlen(hrBuf) || 0 < strlen(cadBuf)) {
                snprintf(tpxBuf, sizeof(tpxBuf), tpxFormat, hrBuf, cadBuf);
            } else
                strncpy(tpxBuf, "", sizeof(tpxBuf));
            snprintf(extBuf, sizeof(extBuf), extFormat, powerBuf, tpxBuf);
        } else
            strncpy(extBuf, "", sizeof(extBuf));

        snprintf(pointBuf, sizeof(pointBuf), pointFormat,
                 locationBuf,
                 timeBuf,
                 altBuf,
                 extBuf);
        pointBufLen = strlen(pointBuf);
        if (gpx.write((uint8_t *)&pointBuf, pointBufLen) != pointBufLen) {
            log_e("could not write point #%d to %s", points, gpxPath);
            device->releaseMutex();
            continue;
        }
        // Serial.print(pointBuf);
        points++;
        device->releaseMutex();
    }
    if (!device->aquireMutex()) {
        log_e("could not aquire mutex");
        return false;
    }
    if (gpx.write((uint8_t *)footer, strlen(footer)) != strlen(footer)) {
        log_e("could not write footer to %s", gpxPath);
        rec.close();
        gpx.close();
        fs->remove(gpxPath);
        device->releaseMutex();
        return false;
    }
    // Serial.print(footer);
    rec.close();
    gpx.close();  // need to reopen to get size
    gpx = fs->open(gpxPath);
    if (!gpx) {
        log_e("could not open %s", gpxPath);
        gpx.close();
        device->releaseMutex();
        return false;
    }
    log_i("%s created, size: %d bytes", gpxPath, gpx.size());
    gpx.close();
    device->releaseMutex();
    return true;
}

ApiResult *Recorder::recProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    Atoll::ApiResult *result = Api::success();
    if (0 < strlen(msg->arg)) {
        if (msg->argIs("start")) {
            if (!instance->start()) result = Api::error();
        } else if (msg->argIs("pause")) {
            if (!instance->pause()) result = Api::error();
        } else if (msg->argIs("end")) {
            if (!instance->end()) result = Api::error();
        } else if (msg->argStartsWith("files")) {
            const char *cPath = instance->currentPath();
            static const uint8_t modeRec = 1;
            static const uint8_t modeGpx = 2;
            uint8_t mode = 0;
            if (msg->argIs("files"))
                mode = modeRec | modeGpx;
            else if (msg->argIs("files:rec"))
                mode = modeRec;
            else if (msg->argIs("files:gpx"))
                mode = modeGpx;
            else
                return Api::argInvalid();
            // log_i("mode: %d, rec: %d, gpx: %d",
            //       mode, (mode & modeRec) ? 1 : 0, (mode & modeGpx) ? 1 : 0);
            if (!instance->device) {
                log_e("device error");
                return Api::internalError();
            }
            if (!instance->device->aquireMutex()) {
                log_e("mutex error");
                return Api::internalError();
            }
            if (!instance->fs) {
                instance->device->releaseMutex();
                log_e("fs error");
                return Api::internalError();
            }
            File dir = instance->fs->open(instance->basePath);
            if (!dir) {
                instance->device->releaseMutex();
                log_e("could not open %s", instance->basePath);
                return Api::internalError();
            }
            if (!dir.isDirectory()) {
                instance->device->releaseMutex();
                log_e("%s not dir", instance->basePath);
                return Api::internalError();
            }
            snprintf(msg->reply, sizeof(msg->reply), "files:");
            File f;
            size_t added = 0;
            bool add;
            while (f = dir.openNextFile()) {
                add = false;
                if (strlen(f.name()) < 5) {  // /rec/last
                    f.close();
                    continue;
                }
                // log_i("checking %s ext: %s", f.name(), f.name() + strlen(f.name()) - 4);
                if ((mode & modeGpx) &&
                    0 == strcmp(f.name() + strlen(f.name()) - 4, ".gpx"))
                    add = true;
                else if ((mode & modeRec) &&
                         8 == strlen(f.name()) &&
                         !strchr(f.name(), '.') &&
                         (!cPath || 0 != strcmp(cPath, f.path())))
                    add = true;
                if (!add) {
                    f.close();
                    continue;
                }
                // log_i("adding %s len: %d", f.name(), strlen(msg->reply));
                if (sizeof(msg->reply) < strlen(msg->reply) + strlen(f.name()) + 15) {
                    log_e("no more space in reply");
                    f.close();
                    break;
                }
                if (added) msg->replyAppend(";");
                msg->replyAppend(f.name());
                f.close();
                added++;
            }
            instance->device->releaseMutex();
            return Api::success();
        } else if (msg->argStartsWith("info:")) {
            char name[16] = "";
            if (!msg->argGetParam("info:", name, sizeof(name)) || strlen(name) < 2)
                return Api::argInvalid();
            log_i("name: %s", name);
            if (!instance->device) {
                log_e("device error");
                return Api::internalError();
            }
            if (!instance->device->aquireMutex()) {
                log_e("mutex error");
                return Api::internalError();
            }
            if (!instance->fs) {
                instance->device->releaseMutex();
                log_e("fs error");
                return Api::internalError();
            }
            char path[ATOLL_RECORDER_PATH_LENGTH] = "";
            snprintf(path, sizeof(path), "%s/%s", instance->basePath, name);
            log_i("path: %s", path);
            File f = instance->fs->open(path);
            if (!f) {
                instance->device->releaseMutex();
                log_e("could not open %s", path);
                return Api::internalError();
            }
            snprintf(msg->reply, sizeof(msg->reply), "info:%s;size:%d", f.name(), f.size());
            f.close();
            char extLess[strlen(name) + 1] = "";
            uint8_t i;
            for (i = 0; i < strlen(name); i++) {
                if (name[i] == '.') break;
                extLess[i] = name[i];
            }
            extLess[i + 1] = 0;
            log_i("extLess: %s", extLess);
            snprintf(path, sizeof(path), "%s/%s%s", instance->basePath, extLess, instance->statsExt);
            log_i("path: %s", path);
            f = instance->fs->open(path);
            if (f) {
                Stats tmpStats;
                size_t read = f.read((uint8_t *)&tmpStats, sizeof(tmpStats));
                f.close();
                if (read < sizeof(tmpStats))
                    log_e("cannot read from %s", path);
                else {
                    log_i("read %d bytes from %s", read, path);
                    char str[64];
                    snprintf(str, sizeof(str), ";distance:%.0f;altGain:%u",
                             tmpStats.distance, tmpStats.altGain);
                    msg->replyAppend(str);
                }
            }
            instance->device->releaseMutex();
            return Api::success();
        } else if (msg->argStartsWith("get:")) {
            char name[16] = "";
            if (!msg->argGetParam("get:", name, sizeof(name)) || strlen(name) < 2)
                return Api::argInvalid();
            // log_i("name: %s", name);
            if (!instance->device) {
                log_e("device error");
                return Api::internalError();
            }
            if (!instance->device->aquireMutex()) {
                log_e("mutex error");
                return Api::internalError();
            }
            if (!instance->fs) {
                instance->device->releaseMutex();
                log_e("fs error");
                return Api::internalError();
            }
            char path[ATOLL_RECORDER_PATH_LENGTH] = "";
            snprintf(path, sizeof(path), "%s/%s", instance->basePath, name);
            // log_i("path: %s", path);
            File f = instance->fs->open(path);
            if (!f) {
                instance->device->releaseMutex();
                log_e("could not open %s", path);
                return Api::internalError();
            }
            char offsetStr[sizeof(int) * 8 + 1];
            if (!msg->argGetParam("offset:", offsetStr, sizeof(offsetStr))) {
                f.close();
                instance->device->releaseMutex();
                log_e("missing offset");
                return Api::argInvalid();
            }
            int offset = atoi(offsetStr);
            // log_i("get: %s offset: %d", name, offset);
            char offsetCmp[sizeof(offsetStr)];
            snprintf(offsetCmp, sizeof(offsetCmp), "%d", offset);
            if (0 < offset) offset -= 1;
            if (offset < 0 || f.size() <= offset || 0 != strcmp(offsetStr, offsetCmp)) {
                f.close();
                instance->device->releaseMutex();
                log_e("invalid offset %s", offsetStr);
                return Api::argInvalid();
            }
            if (!f.seek((uint32_t)offset)) {
                f.close();
                instance->device->releaseMutex();
                log_e("could not seek to %d", offset);
                return Api::internalError();
            }
            snprintf(msg->reply, sizeof(msg->reply),
                     "get:%s:%s;", name, offsetStr);
            size_t replyTextLen = strlen(msg->reply);
            char buf[sizeof(msg->reply) - replyTextLen - 9];
            size_t read = f.readBytes(buf, sizeof(buf));
            f.close();
            instance->device->releaseMutex();
            memcpy(msg->reply + replyTextLen, &buf, read);
            msg->replyLength = replyTextLen + read;
            log_i("get %s:%s sent %d bytes", name, offsetStr, read);
            return Api::success();
        } else if (msg->argStartsWith("delete:")) {
            char name[16] = "";
            if (!msg->argGetParam("delete:", name, sizeof(name)) || strlen(name) < 2)
                return Api::argInvalid();
            // log_i("name: %s", name);
            if (!instance->device) {
                log_e("device error");
                return Api::internalError();
            }
            if (!instance->device->aquireMutex()) {
                log_e("mutex error");
                return Api::internalError();
            }
            if (!instance->fs) {
                instance->device->releaseMutex();
                log_e("fs error");
                return Api::internalError();
            }
            char path[ATOLL_RECORDER_PATH_LENGTH] = "";
            snprintf(path, sizeof(path),
                     "%s/%s", instance->basePath, name);
            // log_i("path: %s", path);
            if (!instance->fs->exists(path)) {
                instance->device->releaseMutex();
                log_e("%s not found", path);
                return Api::argInvalid();
            }
            bool success = instance->fs->remove(path);
            if (nullptr == strchr(name, '.') && 8 == strlen(name)) {
                snprintf(path, sizeof(path),
                         "%s/%s%s", instance->basePath, name, instance->statsExt);
                if (instance->fs->exists(path) && instance->fs->remove(path))
                    log_i("deleted %s", path);
            }
            instance->device->releaseMutex();
            snprintf(msg->reply, sizeof(msg->reply),
                     success ? "deleted: %s" : "failed to delete: %s", name);
            log_i("deleted %s", name);
            return Api::success();
        } else if (msg->argStartsWith("regen:")) {
            char gpxName[16] = "";
            if (!msg->argGetParam("regen:", gpxName, sizeof(gpxName)) || strlen(gpxName) < 2)
                return Api::argInvalid();
            char *gpx = strstr(gpxName, ".gpx");
            if (!gpx) return Api::argInvalid();
            char recName[sizeof(gpxName)] = "";
            strncpy(recName, gpxName, gpx - gpxName);
            log_i("recName: %s", recName);
            if (!instance->device) {
                log_e("device error");
                return Api::internalError();
            }
            if (!instance->device->aquireMutex()) {
                log_e("mutex error");
                return Api::internalError();
            }
            if (!instance->fs) {
                instance->device->releaseMutex();
                log_e("fs error");
                return Api::internalError();
            }
            char gpxPath[ATOLL_RECORDER_PATH_LENGTH] = "";
            snprintf(gpxPath, sizeof(gpxPath),
                     "%s/%s", instance->basePath, gpxName);
            char recPath[ATOLL_RECORDER_PATH_LENGTH] = "";
            snprintf(recPath, sizeof(recPath),
                     "%s/%s", instance->basePath, recName);
            log_i("recPath: %s, gpxPath: %s", recPath, gpxPath);
            if (!instance->fs->exists(recPath)) {
                instance->device->releaseMutex();
                log_e("%s not found", recPath);
                return Api::argInvalid();
            }
            if (!instance->fs->exists(gpxPath)) {
                instance->device->releaseMutex();
                log_e("%s not found", gpxPath);
                return Api::argInvalid();
            }
            instance->device->releaseMutex();
            bool success = instance->rec2gpx(recPath, gpxPath, true);  // overwrite
            snprintf(msg->reply, sizeof(msg->reply),
                     success ? "regen:%s" : "failed: %s", gpxName);
            log_i("regenerated %s", gpxName);
            return success ? Api::success() : Api::error();
        } else {
            snprintf(msg->reply, sizeof(msg->reply),
                     "start|pause|end|files[:rec|:gpx]|info:filename[.gpx]|"
                     "get:filename[.gpx];offset:1234|delete:filename.gpx|"
                     "regen:filename.gpx");
            return Api::result("argInvalid");
        }
    }
    snprintf(msg->reply, sizeof(msg->reply),
             "%d", instance->isRecording);
    return result;
}

#endif