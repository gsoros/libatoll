#ifndef __atoll_oled_h
#define __atoll_oled_h

#include <U8g2lib.h>
#include <TinyGPS++.h>

#include "atoll_task.h"
#include "atoll_touch.h"
#include "atoll_time.h"

namespace Atoll {

class Oled : public Task {
    struct Area {
        uint8_t x;  // top left x
        uint8_t y;  // top left y
        uint8_t w;  // width
        uint8_t h;  // height
        bool invert = false;
    };

   public:
    const char *taskName() { return "Oled"; }
    uint8_t width;
    uint8_t height;
    uint8_t feedbackWidth;
    uint8_t fieldHeight;
    Area fields[3];
    Area feedback[4];

    Oled(
        U8G2 *device,
        uint8_t width = 64,
        uint8_t height = 128,
        uint8_t feedbackWidth = 3,
        uint8_t fieldHeight = 32) {
        this->device = device;
        this->width = width;
        this->height = height;
        this->feedbackWidth = feedbackWidth;
        this->fieldHeight = fieldHeight;

        const uint8_t fieldWidth = width - 2 * feedbackWidth;
        const uint8_t fieldVSeparation = (height - 3 * fieldHeight) / 2;

        fields[0].x = feedbackWidth;
        fields[0].y = 0;
        fields[0].w = fieldWidth;
        fields[0].h = fieldHeight;

        fields[1].x = feedbackWidth;
        fields[1].y = fieldHeight + fieldVSeparation;
        fields[1].w = fieldWidth;
        fields[1].h = fieldHeight;

        fields[2].x = feedbackWidth;
        fields[2].y = height - fieldHeight;
        fields[2].w = fieldWidth;
        fields[2].h = fieldHeight;

        feedback[0].x = 0;
        feedback[0].y = 0;
        feedback[0].w = feedbackWidth;
        feedback[0].h = height / 2;
        feedback[0].invert = true;

        feedback[1].x = 0;
        feedback[1].y = height / 2;
        feedback[1].w = feedbackWidth;
        feedback[1].h = height / 2;

        feedback[2].x = width - feedbackWidth;
        feedback[2].y = 0;
        feedback[2].w = feedbackWidth;
        feedback[2].h = height / 2;
        feedback[2].invert = true;

        feedback[3].x = width - feedbackWidth;
        feedback[3].y = height / 2;
        feedback[3].w = feedbackWidth;
        feedback[3].h = height / 2;
    }

    virtual ~Oled();

    virtual void setup() {
        device->begin();
        device->setFont(u8g2_font_logisoso32_tn);
    }

    virtual void loop();

    virtual void printfField(
        uint8_t fieldIndex,
        bool send,
        uint8_t color,
        uint8_t bgColor,
        const char *format,
        ...) {
        assert(fieldIndex < sizeof(fields) / sizeof(fields[0]));
        if (send && !aquireMutex()) return;
        char out[4];
        va_list argp;
        va_start(argp, format);
        vsnprintf(out, 4, format, argp);
        va_end(argp);
        Area *a = &fields[fieldIndex];
        // log_i("%d(%d, %d, %d, %d) %s", fieldIndex, a->x, a->y, a->w, a->h, out);
        device->setClipWindow(a->x, a->y, a->x + a->w, a->y + a->h);
        fill(a, bgColor, false, false);
        device->setCursor(a->x, a->y + a->h);
        uint8_t oldColor = device->getDrawColor();
        device->setDrawColor(color);
        device->print(out);
        device->setDrawColor(oldColor);
        device->setMaxClipWindow();
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    virtual void fill(Area *a, uint8_t color, bool send = true, bool setClip = true) {
        if (send && !aquireMutex()) return;
        uint8_t oldColor = device->getDrawColor();
        if (setClip) device->setClipWindow(a->x, a->y, a->x + a->w, a->y + a->h);
        device->setDrawColor(color);
        device->drawBox(a->x, a->y, a->w, a->h);
        device->setDrawColor(oldColor);
        if (setClip) device->setMaxClipWindow();
        if (!send) return;
        device->sendBuffer();
        releaseMutex();
    }

    virtual void showTime() {
        tm time = localTm();
        if (!aquireMutex()) return;
        printfField(0, false, 1, 0, "%02d%d",
                    time.tm_hour, time.tm_min / 10);
        printfField(1, false, 1, 0, "%d%02d",
                    time.tm_min % 10, time.tm_sec);
        device->sendBuffer();
        releaseMutex();
    }

    virtual void showDate() {
        tm time = localTm();
        if (!aquireMutex()) return;
        printfField(2, false, 1, 0, "%d%02d",
                    (time.tm_mon + 1) % 10, time.tm_mday);
        device->sendBuffer();
        releaseMutex();
    }

    virtual void onPower(uint16_t value) {
        static uint16_t lastValue = 0;
        if (lastValue != value) {
            printfField(0, true, 1, 0, "%3d", value);
            lastValue = value;
            lastPower = millis();
        }
    }

    virtual void onCadence(uint16_t value) {
        static uint16_t lastValue = 0;
        if (lastValue != value) {
            printfField(1, true, 1, 0, "%3d", value);
            lastValue = value;
            lastCadence = millis();
        }
    }

    virtual void onHeartrate(uint16_t value) {
        static uint16_t lastValue = 0;
        if (lastValue != value) {
            printfField(2, true, 1, 0, "%3d", value);
            lastValue = value;
            lastHeartrate = millis();
        }
    }

    virtual void onTouchEvent(Touch::Pad *pad, Touch::Event event);

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
    ulong lastPower = 0;
    ulong lastCadence = 0;
    ulong lastHeartrate = 0;
    uint32_t satellites = 0;

    virtual bool aquireMutex(uint32_t timeout = 100) {
        // log_i("aquireMutex");
        if (xSemaphoreTake(mutex, (TickType_t)timeout) == pdTRUE)
            return true;
        log_e("Could not aquire mutex");
        return false;
    }

    virtual void releaseMutex() {
        // log_i("releaseMutex");
        xSemaphoreGive(mutex);
    }
};

}  // namespace Atoll

#endif