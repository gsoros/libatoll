#include "atoll_oled.h"

using namespace Atoll;

Oled::~Oled() {
    delete device;
}

void Oled::loop() {
    ulong t = millis();
    if (lastFieldUpdate < t - 3000 && systemTimeLastSet())
        showTime();
    // if (lastHeartrate < t - 3000)
    //     if (systemTimeLastSet())
    //         showDate();
    //     else
    //         showSomething();
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
    // printfFieldDigits(1, true, 0, 1, "%d%02lu",
    //             pad->index, (millis() - pad->start) / 100);

    assert(pad->index < sizeof(feedback) / sizeof(feedback[0]));
    Area *a = &feedback[pad->index];

    switch (event) {
        case Touch::Event::start: {
            return;
        }
        case Touch::Event::end: {
            log_i("pad %d end", pad->index);
            fill(a, C_FG);
            fill(a, C_BG, true, 1000);
            return;
        }
        case Touch::Event::doubleTouch: {
            log_i("pad %d double", pad->index);
            Area b;
            memcpy(&b, a, sizeof(b));
            b.h /= 3;  // divide area height by 3
            if (aquireMutex()) {
                fill(&b, C_FG, false);  // area 1 white
                b.y += b.h;             // move down
                fill(&b, C_BG, false);  // area 2 black
                b.y += b.h;             // move down
                fill(&b, C_FG, false);  // area 3 white
                device->sendBuffer();
                releaseMutex();
            }
            delay(Touch::touchTime * 3);  // TODO is delay() a good idea? maybe create a queue schedule?
            fill(a, C_BG, true, 1000);    // clear
            return;
        }
        case Touch::Event::longTouch: {
            log_i("pad %d long", pad->index);
            fill(a, C_FG);
            delay(Touch::touchTime * 3);
            fill(a, C_BG, true, 1000);
            return;
        }
        case Touch::Event::touching: {
            // log_i("pad %d touching", pad->index);
            if (pad->start + Touch::longTouchTime < millis()) {  // animation completed, still touching
                fill(a, C_FG);
                delay(Touch::touchTime * 3);
                fill(a, C_BG, true, 1000);
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
            fill(a, C_BG, false);
            fill(&b, C_FG);
            return;
        }
        default: {
            log_e("unhandled %d", event);
        }
    }
}
