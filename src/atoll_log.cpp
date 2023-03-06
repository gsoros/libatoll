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

// 0: none, 1: error, 2: warning, 3, info, 4+: debug
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
    char *from = bootLog;
    char out[ATOLL_LOG_BUFFER_SIZE - 32] = "";
    char *to = out;
    int count = 0;

    while (from < bootLog + len) {
        strncpy(to, from, 1);
        if ('\n' == *to || from == bootLog + len - 1 || out + sizeof(out) - 1 <= to) {
            if ('\n' != *to) *to = '\n';
            *(to + 1) = '\0';
            if (count) delay(100);
            write(1, "[BOOT %d] %s", count, out);
            to = out;
            count++;
        } else to++;
        from++;
    }
#endif
}