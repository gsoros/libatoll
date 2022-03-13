#include "atoll_touch.h"

using namespace Atoll;

struct TouchPad Touch::pads[ATOLL_TOUCH_NUM_PADS];
const unsigned char Touch::numPads = ATOLL_TOUCH_NUM_PADS;

void IRAM_ATTR Atoll::touchISR0() {
    Atoll::Touch::pads[0].isTouched = true;
    Atoll::Touch::pads[0].lastTouched = millis();
}
void IRAM_ATTR Atoll::touchISR1() {
    Atoll::Touch::pads[1].isTouched = true;
    Atoll::Touch::pads[1].lastTouched = millis();
}
void IRAM_ATTR Atoll::touchISR2() {
    Atoll::Touch::pads[2].isTouched = true;
    Atoll::Touch::pads[2].lastTouched = millis();
}
void IRAM_ATTR Atoll::touchISR3() {
    Atoll::Touch::pads[3].isTouched = true;
    Atoll::Touch::pads[3].lastTouched = millis();
}