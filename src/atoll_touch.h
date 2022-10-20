#ifndef __atoll_touch_h
#define __atoll_touch_h

#include <Arduino.h>

#include "atoll_task.h"
#include "atoll_preferences.h"

#ifndef ATOLL_TOUCH_NUM_PADS
#define ATOLL_TOUCH_NUM_PADS 4
#endif

#if ATOLL_TOUCH_NUM_PADS < 1
#error at least one pad is required
#endif

#if 4 < ATOLL_TOUCH_NUM_PADS
#error maximum 4 pads are supported
#endif

namespace Atoll {

extern void IRAM_ATTR touchISR0();
extern void IRAM_ATTR touchISR1();
extern void IRAM_ATTR touchISR2();
extern void IRAM_ATTR touchISR3();

class Touch : public Task, public Preferences {
   public:
    struct Pad {
        int pin = -1;
        uint8_t index = 0;
        uint16_t threshold = 85;  // touch sensitivity
        volatile ulong last = 0;  // time of last touch
        ulong start = 0;          // time of start of touch
        ulong end = 0;            // time of end of touch
        uint8_t count = 0;        // number of touches

        void dump() {
            log_i("pad #%d last: %d, start: %d, end: %d, count: %d",
                  index, last, start, end, count);
        }
    };

    const char *taskName() { return "Touch"; }
    static struct Pad pads[ATOLL_TOUCH_NUM_PADS];
    static const unsigned char numPads;

    enum Event {
        invalid,
        start,
        touching,
        end,
        singleTouch,
        doubleTouch,
        tripleTouch,
        quadrupleTouch,
        quintupleTouch,
        // sextupleTouch,
        // septupleTouch,
        longTouch,
        EVENT_MAX
    };

    // ms, max time between two touches to register a multi-touch
    static const uint16_t multiTouchTime = 200;
    // ms, min time to register a long touch
    static const uint16_t longTouchTime = 800;

    bool enabled = true;  // enable touch events
    ulong enableAfter = 0;
    ulong lastLoopTime = 0;

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

    virtual void setup(::Preferences *preferences, const char *preferencesNS) {
        this->preferences = preferences;
        this->preferencesNS = preferencesNS;
        preferencesSetup(preferences, preferencesNS);
        loadSettings();
        attachInterrupts();
    }

    bool padIsTouched(Pad *pad) {
        return enabled && pad->last && lastLoopTime <= pad->last;
    }

    bool anyPadIsTouched() {
        if (!enabled) return false;
        for (uint8_t i = 0; i < numPads; i++)
            if (pads[i].last && lastLoopTime <= pads[i].last)
                return true;
        return false;
    }

    static const char *eventName(Event event) {
        switch (event) {
            case start:
                return "start";
            case touching:
                return "touching";
            case singleTouch:
                return "singleTouch";
            case doubleTouch:
                return "doubleTouch";
            case tripleTouch:
                return "tripleTouch";
            case quadrupleTouch:
                return "quadrupleTouch";
            case quintupleTouch:
                return "quintupleTouch";
            case longTouch:
                return "longTouch";
            case end:
                return "end";
            default:
                log_e("%d not handled", event);
                return "invalid";
        }
    }

    static Event eventType(uint8_t numTouches) {
        switch (numTouches) {
            case 1:
                return Event::singleTouch;
            case 2:
                return Event::doubleTouch;
            case 3:
                return Event::tripleTouch;
            case 4:
                return Event::quadrupleTouch;
            case 5:
                return Event::quintupleTouch;
            default:
                log_e("%d not handled", numTouches);
                return Event::invalid;
        }
    }

    bool loadSettings() {
        if (!preferencesStartLoad()) return false;
        for (int i = 0; i < numPads; i++) {
            char key[8];
            snprintf(key, sizeof key, "thres%d", i);
            uint8_t threshold = preferences->getUChar(key, pads[i].threshold);
            // log_i("pad %d threshold %d", i, threshold);
            pads[i].threshold = threshold;
        }
        preferencesEnd();
        return true;
    }

    bool saveSettings() {
        if (!preferencesStartSave()) return false;
        uint8_t result = 0;
        for (int i = 0; i < numPads; i++) {
            char key[8];
            snprintf(key, sizeof key, "thres%d", i);
            if (preferences->putUChar(key, pads[i].threshold)) {
                log_i("pad %d saved threshold %d", i, pads[i].threshold);
                result++;
            }
        }
        preferencesEnd();
        return result == numPads;
    }

    void setPadThreshold(uint8_t index, uint8_t threshold) {
        if (numPads <= index) {
            log_e("index out of range: %d", index);
            return;
        }
        if (threshold < 1 || 100 < threshold) {
            log_e("threshold out of range: %d", threshold);
            return;
        }
        pads[index].threshold = threshold;
        switch (index) {
            case 0:
                attachInterrupt(index, touchISR0);
                break;
            case 1:
                attachInterrupt(index, touchISR1);
                break;
            case 2:
                attachInterrupt(index, touchISR2);
                break;
            case 3:
                attachInterrupt(index, touchISR3);
                break;
            default:
                break;
        }
    }

    virtual uint16_t read(uint8_t index) {
        return touchRead(pads[index].pin);
    }

    virtual void onEnabledChanged() {
        log_i("touch %sabled", enabled ? "en" : "dis");
    }

   protected:
    static ::Preferences *preferences;
    static const char *preferencesNS;

    virtual void loop() {
        ulong t = millis();
        if (!enabled) {
            if (enableAfter <= t && enableAfter != 0) {
                enabled = true;
                enableAfter = 0;
                attachInterrupts();  // interrupt is disabled by read()
                onEnabledChanged();
            } else
                return;
        }
        static Pad *p;
        for (uint8_t i = 0; i < numPads; i++) {
            p = &pads[i];
            if (!p->last) continue;                                 // not touched yet
            if (lastLoopTime <= p->last) {                          // touched since last loop
                if (!p->start) {                                    // start not set
                    p->start = t;                                   // set start
                    p->end = 0;                                     // reset end
                    p->count++;                                     // increase count
                    fireEvent(i, Event::start);                     // fire start event
                } else {                                            // start is set
                    fireEvent(i, Event::touching);                  // fire touching event
                }                                                   //
            } else {                                                // not touched since last loop
                if (!p->end) {                                      // end not set
                    if (p->start) {                                 // start is set
                        p->end = t;                                 // set end
                        fireEvent(i, Event::end);                   // fire end event
                        if (p->start <= t - longTouchTime) {        // long touch
                            p->end = 0;                             // reset end
                            p->count = 0;                           // reset count
                            fireEvent(i, Event::longTouch);         // fire long event
                        }                                           //
                        p->start = 0;                               // reset start
                    }                                               //
                }                                                   //
                else {                                              // end is set
                    if (p->end < t - multiTouchTime && p->count) {  // multitouch time passed
                        p->end = 0;                                 // reset end
                        fireEvent(i, eventType(p->count));          // fire (multi)touch event
                        p->count = 0;                               // reset count
                    }
                }
            }
        }
        lastLoopTime = t;
    }

    virtual void fireEvent(uint8_t index, Event event) {
        log_i("%d %s", index, eventName(event));
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
        log_i("%d %2d %2d", index, pads[index].pin, pads[index].threshold);
        touchAttachInterrupt(pads[index].pin, ISR, pads[index].threshold);
    }
};

}  // namespace Atoll

#endif