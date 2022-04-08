#ifndef __atoll_oled_h
#define __atoll_oled_h

#include <U8g2lib.h>

#include "atoll_task.h"
#include "atoll_time.h"
#include "atoll_log.h"

namespace Atoll {

class Oled : public Task {
   public:
    enum Color {
        C_BG,
        C_FG,
        C_XOR
    };

    const char *taskName() { return "Oled"; }
    uint8_t width;   // screen width
    uint8_t height;  // screen height

    Oled(U8G2 *device,
         uint8_t width = 64,
         uint8_t height = 128) {
        this->device = device;
        this->width = width;
        this->height = height;
    }

    virtual ~Oled();

    virtual void setup() {
        device->begin();
    }

    virtual void loop() { log_e("not implemented"); }

    virtual bool setContrast(uint8_t contrast) {
        if (!aquireMutex()) return false;
        device->setContrast(contrast);
        releaseMutex();
        return true;
    }

   protected:
    SemaphoreHandle_t mutex = xSemaphoreCreateMutex();
    U8G2 *device;
    ulong lastTime = 0;
    ulong lastFieldUpdate = 0;

    virtual bool aquireMutex(uint32_t timeout = 100) {
        // log_i("aquireMutex %d", (int)mutex);
        if (xSemaphoreTake(mutex, (TickType_t)timeout) == pdTRUE)
            return true;
        log_e("Could not aquire mutex");
        return false;
    }

    virtual void releaseMutex() {
        // log_i("releaseMutex %d", (int)mutex);
        xSemaphoreGive(mutex);
    }
};

}  // namespace Atoll

#endif