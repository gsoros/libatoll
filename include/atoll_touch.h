#ifndef __atoll_touch_h
#define __atoll_touch_h

#include <Arduino.h>

#include "atoll_task.h"

namespace Atoll {

struct TouchPad {
    int pin = -1;
    uint8_t index = 0;
    uint16_t threshold = 85;
    volatile bool isTouched = false;
    bool wasTouched = false;
    volatile ulong lastTouched = 0;
    ulong touchStart = 0;
};

#ifndef ATOLL_TOUCH_NUM_PADS
#define ATOLL_TOUCH_NUM_PADS 4
#endif

enum TouchEvent {
    start,
    touching,
    end
};

extern void IRAM_ATTR touchISR0();
extern void IRAM_ATTR touchISR1();
extern void IRAM_ATTR touchISR2();
extern void IRAM_ATTR touchISR3();

class Touch : public Task {
   public:
    static struct TouchPad pads[ATOLL_TOUCH_NUM_PADS];
    static const unsigned char numPads;

    Touch(int pin0 = -1,
          int pin1 = -1,
          int pin2 = -1,
          int pin3 = -1) {
        pads[0].index = 0;
        pads[0].pin = pin0;
#if 1 < ATOLL_TOUCH_NUM_PADS
        pads[1].index = 1;
        pads[1].pin = pin1;
#endif
#if 2 < ATOLL_TOUCH_NUM_PADS
        pads[2].index = 2;
        pads[2].pin = pin2;
#endif
#if 3 < ATOLL_TOUCH_NUM_PADS
        pads[3].index = 3;
        pads[3].pin = pin3;
#endif
    }

    virtual void setup() {
        attachInterrupts();
    }

    bool anyPadIsTouched() {
        for (int i = 0; i < numPads; i++)
            if (pads[i].isTouched)
                return true;
        return false;
    }

    static const char *eventName(TouchEvent event) {
        switch (event) {
            case start:
                return "start";
            case touching:
                return "touching";
            case end:
                return "end";
        }
        return "unknown";
    }

   protected:
    virtual void loop() {
        // log_i("%d", read(0));
        static ulong lastT = 0;
        ulong t = millis();
        for (int i = 0; i < numPads; i++) {
            if (!pads[i].isTouched) continue;
            // log_i("touched %d", i);
            if (pads[i].lastTouched < lastT) {
                pads[i].isTouched = false;
                pads[i].wasTouched = false;
                pads[i].touchStart = 0;
            }
            if (pads[i].isTouched) {
                if (!pads[i].wasTouched) {
                    pads[i].touchStart = t;
                    onEvent(i, TouchEvent::start);
                    pads[i].wasTouched = true;
                    continue;
                }
                onEvent(i, TouchEvent::touching);
                continue;
            }
            onEvent(i, TouchEvent::end);
        }
        lastT = t;
    }

    virtual void onEvent(uint8_t index, TouchEvent event) {
        // log_i("[Touch %d] %s", index, eventName(event));
    }

    virtual void attachInterrupts() {
#if 0 < ATOLL_TOUCH_NUM_PADS
        attachInterrupt(0, touchISR0);
#endif
#if 1 < ATOLL_TOUCH_NUM_PADS
        attachInterrupt(1, touchISR1);
#endif
#if 2 < ATOLL_TOUCH_NUM_PADS
        attachInterrupt(2, touchISR2);
#endif
#if 3 < ATOLL_TOUCH_NUM_PADS
        attachInterrupt(3, touchISR3);
#endif
    }

    virtual void attachInterrupt(uint8_t index, void (*ISR)()) {
        if (pads[index].pin < 0) return;
        // log_i("%d %d", index, pads[index].pin);
        touchAttachInterrupt(pads[index].pin, ISR, pads[index].threshold);
    }

    virtual uint16_t read(uint8_t index) {
        return touchRead(pads[index].pin);
    }
};

}  // namespace Atoll

#endif