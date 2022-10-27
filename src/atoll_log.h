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

namespace Atoll {

class Log {
   public:
    static void write(uint8_t level, const char *format, ...);
    static void setLevel(uint8_t level);

   protected:
    static uint8_t level;
#if 0 != ATOLL_LOG_LEVEL
    static char buffer[ATOLL_LOG_BUFFER_SIZE];
    static SemaphoreHandle_t mutex;
#endif
};

}  // namespace Atoll

#endif