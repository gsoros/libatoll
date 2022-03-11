#include "atoll_touch.h"

using namespace Atoll;

// struct TouchPad Atoll::touchPads[ATOLL_TOUCH_NUM_PADS];

struct TouchPad Atoll::Touch::pads[ATOLL_TOUCH_NUM_PADS];

void IRAM_ATTR Atoll::touchISR0() {
    Atoll::Touch::pads[0].isTouched = true;
    Atoll::Touch::pads[0].lastTouched = millis();
}