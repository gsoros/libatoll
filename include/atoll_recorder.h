#ifndef __atoll_recorder_h
#define __atoll_recorder_h

#include <Arduino.h>
#include <CircularBuffer.h>

#include "atoll_task.h"
#include "atoll_gps.h"
#include "atoll_fs.h"
#include "atoll_api.h"
#include "atoll_log.h"

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
#define ATOLL_RECORDER_CONTINUE_PATH "/rec/last"
#endif

#ifndef ATOLL_RECORDER_PATH_LENGTH
#define ATOLL_RECORDER_PATH_LENGTH 32
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
    Api *api = nullptr;                                       //
    static Recorder *instance;                                // instance pointer for static access
    const char *basePath = ATOLL_RECORDER_BASE_PATH;          // base path to the recordings
    const char *statsExt = ATOLL_RECORDER_EXT_STATS;          // stats file extension
    const char *continuePath = ATOLL_RECORDER_CONTINUE_PATH;  // full path to the file containing the path to the recording to be continued after an interruption

    void setup(GPS *gps,
               Fs *fs,
               Api *api = nullptr,
               Recorder *instance = nullptr);
    void loop();
    void addDataPoint();
    bool saveBuffer();
    bool saveStats();
    bool loadStats(bool reportFail = true);
    const char *currentPath(bool reset = false);
    const char *currentStatsPath(bool reset = false);
    void resetBuffer(bool clearPoints = false);
    bool resume();
    bool start();
    bool pause();
    bool end();
    bool stop(bool forgetLast = false);

    CircularBuffer<uint16_t, ATOLL_RECORDER_POWER_RINGBUF_SIZE> powerBuf;
    SemaphoreHandle_t powerMutex = xSemaphoreCreateMutex();
    void onPower(uint16_t value);
    uint16_t avgPower(bool clearBuffer = false);

    CircularBuffer<uint8_t, ATOLL_RECORDER_CADENCE_RINGBUF_SIZE> cadenceBuf;
    SemaphoreHandle_t cadenceMutex = xSemaphoreCreateMutex();
    void onCadence(uint8_t value);
    uint8_t avgCadence(bool clearBuffer = false);

    CircularBuffer<uint8_t, ATOLL_RECORDER_HR_RINGBUF_SIZE> heartrateBuf;
    SemaphoreHandle_t heartrateMutex = xSemaphoreCreateMutex();
    void onHeartrate(uint8_t value);
    uint8_t avgHeartrate(bool clearBuffer = false);

    bool aquireMutex(SemaphoreHandle_t mutex, uint32_t timeout = 100);
    void releaseMutex(SemaphoreHandle_t mutex);

    virtual bool rec2gpx(const char *in, const char *out);

    static ApiResult *recProcessor(ApiReply *reply);
};

}  // namespace Atoll

#endif