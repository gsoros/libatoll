#include "atoll_oled.h"

using namespace Atoll;

Oled::~Oled() {
    delete device;
}
