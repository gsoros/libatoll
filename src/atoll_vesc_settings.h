#ifndef __atoll_vesc_settings_h
#define __atoll_vesc_settings_h

#include <Arduino.h>

#include "atoll_preferences.h"

namespace Atoll {
class VescSettings {
   public:
    uint8_t battNumSeries = 13;       // number of cells in series
    float battCapacityWh = 740;       // 3.7V * 20Ah
    uint16_t maxPower = 2500;         // power limit, W
    float minCurrent = 1.1f;          // minimum current for startup, A
    float maxCurrent = 50.0f;         // current limit, A
    bool rampUp = true;               // ramp up active
    bool rampDown = true;             // ramp down active
    float rampMinCurrentDiff = 1.0f;  // minimum current difference for ramping, A
    uint8_t rampNumSteps = 3;         // number of ramp steps
    uint16_t rampTime = 500;          // ramp time, ms
    uint8_t tMotorWarn = 60;          // tMotor warning, ˚C
    uint8_t tMotorLimit = 90;         // tMotor limit, ˚C
    uint8_t tEscWarn = 50;            // tESC warning, ˚C
    uint8_t tEscLimit = 90;           // tESC limit, ˚C

    void load(::Preferences *p);
    void save(::Preferences *p);
};
}  // namespace Atoll

#endif