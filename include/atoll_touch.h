#ifndef __atoll_touch_h
#define __atoll_touch_h

#include <Arduino.h>

#include "atoll_task.h"

namespace Atoll {
struct TouchPad : public Task {
    int pin = -1;
    uint16_t threshold = 42;
    bool isTouched = false;
    bool wasTouched = false;
    ulong lastTouched = 0;
};

#ifndef ATOLL_TOUCH_NUM_PADS
#define ATOLL_TOUCH_NUM_PADS 4
#endif

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
                    Serial.printf("[Touch %d] start\n", i);
                    pads[i].wasTouched = true;
                    continue;
                }
                Serial.printf("[Touch %d] touching\n", i);
                continue;
            }
            Serial.printf("[Touch %d] end\n", i);
        }
        lastT = t;
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