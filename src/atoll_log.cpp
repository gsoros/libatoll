#include "atoll_log.h"

using namespace Atoll;

uint8_t Log::level = ATOLL_LOG_LEVEL;
Log::writeCallback_t Log::writeCallback = nullptr;

#if 0 != ATOLL_LOG_LEVEL
char Log::buffer[ATOLL_LOG_BUFFER_SIZE];
SemaphoreHandle_t Log::mutex = xSemaphoreCreateMutex();
#endif

void Log::write(uint8_t level, const char *format, ...) {
#if 0 != ATOLL_LOG_LEVEL
    if (Log::level < level) return;
    if (!xSemaphoreTake(Log::mutex, portMAX_DELAY) == pdTRUE) return;
    va_list arg;
    va_start(arg, format);
    vsnprintf(Log::buffer, sizeof(Log::buffer), format, arg);
    va_end(arg);
#if defined(FEATURE_SERIAL)
    Serial.print(Log::buffer);
#endif
    if (nullptr != Log::writeCallback) {
        size_t len = strlen(Log::buffer);
        if (2 < len && '\n' == Log::buffer[len-2]) len -= 2;  // strip last newline
        Log::writeCallback(Log::buffer, len);
    }
    xSemaphoreGive(Log::mutex);
#endif
}

void Log::setLevel(uint8_t level) {
    Log::level = level;
}

void Log::setWriteCallback(Log::writeCallback_t callback) {
    Log::writeCallback = callback;
}