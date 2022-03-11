#ifndef __atoll_touch_h
#define __atoll_touch_h

#include <Arduino.h>

#include "atoll_task.h"

namespace Atoll {
struct TouchPad : public Task {
    int pin = -1;
    uint16_t threshold = 40;
    bool isTouched = false;
    bool wasTouched = false;
    ulong lastTouched = 0;
};

#define ATOLL_TOUCH_NUM_PADS 1

extern void IRAM_ATTR touchISR0();

class Touch : public Task {
   public:
    static struct TouchPad pads[ATOLL_TOUCH_NUM_PADS];

    Touch(int pin0) {
        pads[0].pin = pin0;
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
        attachInterrupt(0, touchISR0);
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