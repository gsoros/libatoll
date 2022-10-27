#include "atoll_log.h"

using namespace Atoll;

uint8_t Log::level = ATOLL_LOG_LEVEL;

#if 0 != ATOLL_LOG_LEVEL
char Log::buffer[ATOLL_LOG_BUFFER_SIZE];
SemaphoreHandle_t Log::mutex = xSemaphoreCreateMutex();
#endif

void Log::write(uint8_t level, const char *format, ...) {
#if defined(FEATURE_SERIAL) && 0 != ATOLL_LOG_LEVEL
    if (Log::level < level) return;
    if (!xSemaphoreTake(Log::mutex, portMAX_DELAY) == pdTRUE) return;
    va_list arg;
    va_start(arg, format);
    vsnprintf(Log::buffer, sizeof(Log::buffer), format, arg);
    va_end(arg);
    Serial.print(Log::buffer);
    xSemaphoreGive(Log::mutex);
#endif
}

void Log::setLevel(uint8_t level) {
    Log::level = level;
}