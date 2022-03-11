#ifndef __atoll_touch_h
#define __atoll_touch_h

#include <Arduino.h>

#include "atoll_task.h"

namespace Atoll {

struct TouchPad {
    int pin = -1;
    uint16_t threshold = 42;
    bool isTouched = false;
    bool wasTouched = false;
    ulong lastTouched = 0;
};

#ifndef ATOLL_TOUCH_NUM_PADS
#define ATOLL_TOUCH_NUM_PADS 4
#endif

#define ATOLL_TOUCH_START 0
#define ATOLL_TOUCH_TOUCHING 1
#define ATOLL_TOUCH_END 2

extern void IRAM_ATTR touchISR0();
extern void IRAM_ATTR touchISR1();
extern void IRAM_ATTR touchISR2();
extern void IRAM_ATTR touchISR3();

class Touch : public Task {
   public:
    static struct TouchPad pads[ATOLL_TOUCH_NUM_PADS];

    Touch(int pin0 = -1, int pin1 = -1, int pin2 = -1, int pin3 = -1) {
        pads[0].pin = pin0;
        pads[1].pin = pin1;
        pads[2].pin = pin2;
        pads[3].pin = pin3;
    }

    void setup() {
        attachInterrupts();
    }

   protected:
    void loop() {
        static ulong lastT = 0;
        ulong t = millis();
        for (int i = 0; i < ATOLL_TOUCH_NUM_PADS; i++) {
            if (!pads[i].isTouched) continue;
            if (pads[i].lastTouched < lastT) {
                pads[i].isTouched = false;
                pads[i].wasTouched = false;
            }
            if (pads[i].isTouched) {
                if (!pads[i].wasTouched) {
                    fireEvent(i, ATOLL_TOUCH_START);
                    pads[i].wasTouched = true;
                    continue;
                }
                fireEvent(i, ATOLL_TOUCH_TOUCHING);
                continue;
            }
            fireEvent(i, ATOLL_TOUCH_END);
        }
        lastT = t;
    }

    void fireEvent(uint8_t index, uint8_t event) {
        Serial.printf("[Touch %d] %s\n", index, eventName(event));
    }

    const char *eventName(uint8_t event) {
        switch (event) {
            case ATOLL_TOUCH_START:
                return "start";
            case ATOLL_TOUCH_TOUCHING:
                return "touching";
            case ATOLL_TOUCH_END:
                return "end";
        }
        return "unknown";
    }

    void attachInterrupts() {
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

    void attachInterrupt(uint8_t index, void (*ISR)()) {
        if (pads[index].pin < 0) return;
        touchAttachInterrupt(pads[index].pin, ISR, pads[index].threshold);
    }

    uint16_t read(uint8_t index) {
        return touchRead(pads[index].pin);
    }
};

}  // namespace Atoll

#endif