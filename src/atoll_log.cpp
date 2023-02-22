#include "atoll_log.h"

using namespace Atoll;

uint8_t Log::level = ATOLL_LOG_LEVEL;

#if 0 != ATOLL_LOG_LEVEL
char Log::buffer[ATOLL_LOG_BUFFER_SIZE];
SemaphoreHandle_t Log::mutex = xSemaphoreCreateMutex();
Log::writeCallback_t Log::writeCallback = nullptr;
#if defined(ATOLL_BOOTLOG_SIZE) && 0 < ATOLL_BOOTLOG_SIZE
char Log::bootLog[ATOLL_BOOTLOG_SIZE] = "";
bool Log::bootLogIsFull = false;
#endif
#endif

void Log::write(uint8_t level, const char *format, ...) {
#if 0 != ATOLL_LOG_LEVEL
    if (Log::level < level) return;
    if (!xSemaphoreTake(mutex, (TickType_t)100) == pdTRUE) return;
    va_list arg;
    va_start(arg, format);
    vsnprintf(buffer, sizeof(buffer), format, arg);
    va_end(arg);
#if defined(ATOLL_BOOTLOG_SIZE) && 0 < ATOLL_BOOTLOG_SIZE
    if (!bootLogIsFull) {
        size_t len = strlen(bootLog);
        if (ATOLL_BOOTLOG_SIZE <= len + strlen(buffer)) {
            bootLogIsFull = true;
        }
        strncat(bootLog, buffer, ATOLL_BOOTLOG_SIZE - len);
    }
#endif
#ifdef FEATURE_SERIAL
    Serial.print(buffer);
#endif
    if (nullptr != writeCallback) {
        size_t len = strlen(buffer);
        // #ifdef FEATURE_SERIAL
        //    if (3 < len) Serial.printf("%d %d %d", buffer[len-3], buffer[len-2], buffer[len-1]);
        // #endif
        if (2 < len && '\r' == buffer[len-2] && '\n' == buffer[len-1]) len -= 2;  // strip last crnl
        writeCallback(buffer, len);
    }
    xSemaphoreGive(Log::mutex);
#endif
}

void Log::setLevel(uint8_t level) {
    Log::level = level;
}

void Log::setWriteCallback(Log::writeCallback_t callback) {
#if 0 != ATOLL_LOG_LEVEL
    writeCallback = callback;
#endif
}

void Log::dumpBootLog() {
#if 0 != ATOLL_LOG_LEVEL && defined(ATOLL_BOOTLOG_SIZE) && 0 < ATOLL_BOOTLOG_SIZE
    if (nullptr == bootLog) return;
    size_t len = strlen(bootLog);
    int count = 0;
    for (int i = 0; i < len; i += ATOLL_LOG_BUFFER_SIZE - 11) {
        if (0 < i) delay(1000);
        write(1, "\n[BOOT %d] %s\n\n", count, (char *)(bootLog + i));
        // log_d("\n[BOOT %d] %s\n\n", count, (char *)(bootLog + i));
        count++;
    }
#endif
}