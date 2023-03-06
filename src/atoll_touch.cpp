#ifdef FEATURE_TOUCH

#include "atoll_touch.h"
#include "atoll_preferences.h"

using namespace Atoll;

struct Touch::Pad Touch::pads[ATOLL_TOUCH_NUM_PADS];
const unsigned char Touch::numPads = ATOLL_TOUCH_NUM_PADS;

::Preferences *Touch::preferences;
const char *Touch::preferencesNS;

void IRAM_ATTR Atoll::touchISR0() {
    Atoll::Touch::pads[0].last = millis();
}
void IRAM_ATTR Atoll::touchISR1() {
    Atoll::Touch::pads[1].last = millis();
}
void IRAM_ATTR Atoll::touchISR2() {
    Atoll::Touch::pads[2].last = millis();
}
void IRAM_ATTR Atoll::touchISR3() {
    Atoll::Touch::pads[3].last = millis();
}

#endif