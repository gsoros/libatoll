#include "atoll_oled.h"

using namespace Atoll;

Oled::~Oled() {
    delete device;
}

void Oled::loop() {
    ulong t = millis();
    if ((lastPower < t - 3000) && (lastCadence < t - 3000))
        if (systemTimeLastSet())
            showTime();
    if (lastHeartrate < t - 3000)
        if (systemTimeLastSet())
            showDate();
    // else
    //    showSomething();
    /*
    static const uint8_t contrastMax = __UINT8_MAX__;
    static int16_t contrast = contrastMax;
    static int16_t newContrast = contrast;
    if (board.touch.anyPadIsTouched()) {
        static bool increase = true;
        newContrast += increase ? 50 : -50;
        if (newContrast < 0) {
            newContrast = 0;
            increase = true;
        } else if (contrastMax < newContrast) {
            newContrast = contrastMax;
            increase = false;
        }
    } else
        newContrast = contrastMax;
    if (contrast != newContrast) {
        contrast = newContrast;
        // bool success =
        setContrast((uint8_t)contrast);
        // log_i("Contrast: %d %s\n", contrast, success ? "succ" : "fail");
    }
    */
}

void Oled::onTouchEvent(Touch::Pad *pad, Touch::Event event) {
    // printfField(1, true, 0, 1, "%d%02lu",
    //             pad->index, (millis() - pad->start) / 100);

    assert(pad->index < sizeof(feedback) / sizeof(feedback[0]));
    Area *a = &feedback[pad->index];

    switch (event) {
        case Touch::Event::start: {
            return;
        }
        case Touch::Event::end: {
            log_i("pad %d end", pad->index);
            fill(a, 1);
            fill(a, 0);
            return;
        }
        case Touch::Event::doubleTouch: {
            log_i("pad %d double", pad->index);
            Area b;
            memcpy(&b, a, sizeof(b));
            b.h /= 3;                     // divide area height by 3
            fill(&b, 1, false);           // area 1 white
            b.y += b.h;                   // move down
            fill(&b, 0, false);           // area 2 black
            b.y += b.h;                   // move down
            fill(&b, 1);                  // area 3 white
            delay(Touch::touchTime * 3);  //
            fill(a, 0);                   // clear
            return;
        }
        case Touch::Event::longTouch: {
            log_i("pad %d long", pad->index);
            fill(a, 1);
            delay(Touch::touchTime * 3);
            fill(a, 0);
            return;
        }
        case Touch::Event::touching: {
            // log_i("pad %d touching", pad->index);
            if (pad->start + Touch::longTouchTime < millis()) {  // animation completed, still touching
                fill(a, 1);
                delay(Touch::touchTime * 3);
                fill(a, 0);
                return;
            }
            // log_i("pad %d animating", pad->index);
            Area b;
            memcpy(&b, a, sizeof(b));
            b.h = map(
                millis() - pad->start,
                0,
                Touch::longTouchTime,
                0,
                b.h);                 // scale down area height
            if (a->h < b.h) return;   // overflow
            b.y += (a->h - b.h) / 2;  // move area to vertical middle
            fill(a, 0, false);
            fill(&b, 1);
            return;
        }
        default: {
            log_e("unhandled %d", event);
        }
    }
}
