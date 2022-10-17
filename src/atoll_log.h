#ifndef __atoll_log_h
#define __atoll_log_h

#include <Arduino.h>

#include "atoll_serial.h"

#ifndef ATOLL_LOG_BUFFER_SIZE
#define ATOLL_LOG_BUFFER_SIZE 512
#endif

#ifdef log_i
#undef log_i
#endif
#define log_i(format, ...) Atoll::log(ARDUHAL_LOG_FORMAT(I, format), ##__VA_ARGS__)

#ifdef log_w
#undef log_w
#endif
#define log_w(format, ...) Atoll::log(ARDUHAL_LOG_FORMAT(W, format), ##__VA_ARGS__)

#ifdef log_e
#undef log_e
#endif
#define log_e(format, ...) Atoll::log(ARDUHAL_LOG_FORMAT(E, format), ##__VA_ARGS__)

namespace Atoll {

inline void log(const char *format, ...) {
#ifdef FEATURE_SERIAL
    char buf[ATOLL_LOG_BUFFER_SIZE];
    va_list arg;
    va_start(arg, format);
    vsnprintf(buf, sizeof(buf), format, arg);
    va_end(arg);
    Serial.print(buf);
#endif
}

}  // namespace Atoll

#endif