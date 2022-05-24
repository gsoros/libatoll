#ifndef __atoll_fs_h
#define __atoll_fs_h

#include <Arduino.h>
#include "FS.h"

#include "atoll_serial.h"

namespace Atoll {

class Fs {
   public:
    bool mounted = false;
    SemaphoreHandle_t defaultMutex = xSemaphoreCreateMutex();
    SemaphoreHandle_t *mutex = &defaultMutex;

    virtual void setup() = 0;

    virtual fs::FS *pFs() = 0;

    virtual void unmount() { log_i("not implemented"); }

    virtual int format() {
        log_i("not implemented");
        return -1;
    }

    virtual bool aquireMutex(uint32_t timeout = 100) {
        // log_d("aquireMutex %d", (int)mutex);
        if (xSemaphoreTake(*mutex, (TickType_t)timeout) == pdTRUE)
            return true;
        log_i("Could not aquire mutex");
        return false;
    }

    virtual void releaseMutex() {
        // log_d("releaseMutex %d", (int)mutex);
        xSemaphoreGive(*mutex);
    }
};

}  // namespace Atoll

#endif