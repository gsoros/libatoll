#ifndef __atoll_log_h
#define __atoll_log_h

#include <Arduino.h>

#include "atoll_serial.h"

// 0: none, 1: error, 2: warning, 3, info, 4+: debug
#ifndef ATOLL_LOG_LEVEL
#define ATOLL_LOG_LEVEL 1
#endif

#if 0 != ATOLL_LOG_LEVEL
#ifndef ATOLL_LOG_BUFFER_SIZE
#define ATOLL_LOG_BUFFER_SIZE 512
#endif
#endif

#ifndef ATOLL_BOOTLOG_SIZE
#define ATOLL_BOOTLOG_SIZE 1024
#endif

#ifdef log_e
#undef log_e
#endif
#if 0 < ATOLL_LOG_LEVEL
#define log_e(format, ...) Atoll::Log::write(ARDUHAL_LOG_LEVEL_ERROR, ARDUHAL_LOG_FORMAT(E, format), ##__VA_ARGS__)
#else
#define log_e(format, ...) ((void)0)
#endif

#ifdef log_w
#undef log_w
#endif
#if 1 < ATOLL_LOG_LEVEL
#define log_w(format, ...) Atoll::Log::write(ARDUHAL_LOG_LEVEL_WARN, ARDUHAL_LOG_FORMAT(W, format), ##__VA_ARGS__)
#else
#define log_w(format, ...) ((void)0)
#endif

#ifdef log_i
#undef log_i
#endif
#if 2 < ATOLL_LOG_LEVEL
#define log_i(format, ...) Atoll::Log::write(ARDUHAL_LOG_LEVEL_INFO, ARDUHAL_LOG_FORMAT(I, format), ##__VA_ARGS__)
#else
#define log_i(format, ...) ((void)0)
#endif

#ifdef log_d
#undef log_d
#endif
#if 3 < ATOLL_LOG_LEVEL
#define log_d(format, ...) Atoll::Log::write(ARDUHAL_LOG_LEVEL_DEBUG, ARDUHAL_LOG_FORMAT(D, format), ##__VA_ARGS__)
#else
#define log_d(format, ...) ((void)0)
#endif

#ifdef log_s
#undef log_s
#endif
/* naked info log */
#define log_s(format, ...) Atoll::Log::write(ARDUHAL_LOG_LEVEL_INFO, format "\r\n", ##__VA_ARGS__)

#ifdef log_sn
#undef log_sn
#endif
/* naked info log without newline */
#define log_sn(format, ...) Atoll::Log::write(ARDUHAL_LOG_LEVEL_INFO, format, ##__VA_ARGS__)

namespace Atoll {

class Log {
    // typedef void (*writeCallback_t)(const char *buf, size_t size);
    typedef std::function<void(const char *buf, size_t size)> writeCallback_t;

   public:
    static void write(uint8_t level, const char *format, ...);

    // 0: none, 1: error, 2: warning, 3, info, 4+: debug
    static void setLevel(uint8_t level);

    static void setWriteCallback(writeCallback_t callback);
    static void dumpBootLog();

   protected:
    static uint8_t level;  // 0: none, 1: error, 2: warning, 3, info, 4+: debug
#if 0 != ATOLL_LOG_LEVEL
    static char buffer[ATOLL_LOG_BUFFER_SIZE];
    static SemaphoreHandle_t mutex;
    static writeCallback_t writeCallback;
#if defined(ATOLL_BOOTLOG_SIZE) && 0 < ATOLL_BOOTLOG_SIZE
    static char bootLog[ATOLL_BOOTLOG_SIZE];
    static bool bootLogIsFull;
#endif
#endif
};

}  // namespace Atoll

#endif