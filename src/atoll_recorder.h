#if !defined(__atoll_recorder_h) && defined(FEATURE_RECORDER)
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
#define ATOLL_RECORDER_INTERVAL 200
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

// #define ATOLL_RECORDER_DATAPOINT_SIZE 29

namespace Atoll {

class Recorder : public Task {
   public:
    struct __attribute__((packed)) DataPoint {
        byte flags = 0;           // length: 1;
        time_t time = 0;          // length: 4; unit: seconds; UTS
        double lat = 0.0;         // length: 8; GCS latitude 0°... 90˚
        double lon = 0.0;         // length: 8; GCS longitude 0°... 180˚
        int16_t altitude = 0;     // length: 2; unit: m
        uint16_t power = 0;       // length: 2; unit: W
        uint8_t cadence = 0;      // length: 1; unit: rpm
        uint8_t heartrate = 0;    // length: 1; unit: bpm
        int16_t temperature = 0;  // length: 2; unit: ˚C / 10; unused
        // TODO uint32_t crc = 0; // crc32 checksum
        // https://stackoverflow.com/questions/21001659/crc32-algorithm-implementation-in-c-without-a-look-up-table-and-with-a-public-li

        /*
        void write(uint8_t *buf, size_t size) {
            if (size < ATOLL_RECORDER_DATAPOINT_SIZE) {
                log_e("buffer too small");
                return;
            }
            uint8_t cursor = 0;
            uint8_t i;
            buf[cursor] = flags;
            cursor++;
            for (i = 0; i < 4; i++)
                buf[cursor + i] = (time >> (i * 8)) & 0xff;
            cursor += i + 1;
            for (i = 0; i < 8; i++)
                buf[cursor + i] = (lat >> (i * 8)) & 0xff;
            cursor += i + 1;
            for (i = 0; i < 8; i++)
                buf[cursor + i] = (lon >> (i * 8)) & 0xff;
            cursor += i + 1;
        }

        void read(uint8_t *buf, size_t size) {
            if (size < ATOLL_RECORDER_DATAPOINT_SIZE) {
                log_e("buffer too small");
                return;
            }
            flags = buf[0];
            time = buf[1] | (buf[2] << 8) | (buf[3] << 16) | (buf[4] << 24);
            lat = buf[5] | (buf[6] << 8) | (buf[7] << 16) | (buf[8] << 24) |  //
                  (buf[9] << 32) | (buf[10] << 40) | (buf[11] << 48) | (buf[12] << 56);
            lon = buf[13] | (buf[14] << 8) | (buf[15] << 16) | (buf[16] << 24) |  //
                  (buf[17] << 32) | (buf[18] << 40) | (buf[19] << 48) | (buf[20] << 56);
        }
        */
    };

    struct Flags {
        const byte location = 1;
        const byte altitude = 2;
        const byte power = 4;
        const byte cadence = 8;
        const byte heartrate = 16;
        const byte temperature = 32;  // unused
        const byte lap = 64;          // unused
        // const byte isChecksum = 128; // TODO if set, the first four bytes after the flags contain the crc32 checksum of the previous datapoints
    } const Flags;

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
    Atoll::Fs *device = nullptr;                              // the recording device
    FS *fs = nullptr;                                         // the filesystem on the recording device
    Api *api = nullptr;                                       //
    static Recorder *instance;                                // instance pointer for static access
    const char *basePath = ATOLL_RECORDER_BASE_PATH;          // base path to the recordings
    const char *statsExt = ATOLL_RECORDER_EXT_STATS;          // stats file extension
    const char *continuePath = ATOLL_RECORDER_CONTINUE_PATH;  // full path to the file containing the path to the recording to be continued after an interruption

    virtual void setup(GPS *gps,
                       Fs *fs,
                       Api *api = nullptr,
                       Recorder *instance = nullptr);
    virtual void loop();
    virtual void addDataPoint();
    virtual bool saveBuffer();
    virtual bool saveStats();
    virtual bool loadStats(bool reportFail = true);
    virtual const char *currentPath(bool reset = false);
    virtual const char *currentStatsPath(bool reset = false);
    virtual int appendStatsExt(char *path, size_t size);
    virtual void resetBuffer(bool clearPoints = false);
    virtual bool resume();
    virtual bool start();
    virtual bool pause();
    virtual bool end();
    virtual bool stop(bool forgetLast = false);

    virtual void onDistanceChanged(double value) {}
    virtual void onAltGainChanged(uint16_t value) {}

    CircularBuffer<uint16_t, ATOLL_RECORDER_POWER_RINGBUF_SIZE> powerBuf;
    SemaphoreHandle_t powerMutex = xSemaphoreCreateMutex();
    virtual void onPower(uint16_t value);
    int16_t avgPower(bool clearBuffer = false);

    CircularBuffer<uint8_t, ATOLL_RECORDER_CADENCE_RINGBUF_SIZE> cadenceBuf;
    SemaphoreHandle_t cadenceMutex = xSemaphoreCreateMutex();
    virtual void onCadence(uint8_t value);
    int16_t avgCadence(bool clearBuffer = false);

    CircularBuffer<uint8_t, ATOLL_RECORDER_HR_RINGBUF_SIZE> heartrateBuf;
    SemaphoreHandle_t heartrateMutex = xSemaphoreCreateMutex();
    virtual void onHeartrate(uint8_t value);
    int16_t avgHeartrate(bool clearBuffer = false);

    bool aquireMutex(SemaphoreHandle_t mutex, uint32_t timeout = 100);
    void releaseMutex(SemaphoreHandle_t mutex);

    virtual bool rec2gpx(const char *in, const char *out, bool overwrite = false);

    static ApiResult *recProcessor(ApiMessage *reply);
};

}  // namespace Atoll

#endif