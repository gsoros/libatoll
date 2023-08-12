#include "atoll_vesc_settings.h"

using namespace Atoll;

void VescSettings::load(::Preferences *preferences) {
    uint32_t tmpUint = preferences->getUInt("vescBNS", battNumSeries);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    battNumSeries = tmpUint;

    battCapacityWh = preferences->getFloat("vescBC", battCapacityWh);
    if (battCapacityWh < 0.0f) battCapacityWh = 0.0f;

    tmpUint = preferences->getUInt("vescMP", maxPower);
    if (UINT16_MAX < tmpUint) tmpUint = UINT16_MAX;
    maxPower = tmpUint;

    minCurrent = preferences->getFloat("vescMiC", minCurrent);
    if (minCurrent < 0.0f) minCurrent = 0.0f;

    maxCurrent = preferences->getFloat("vescMaC", maxCurrent);
    if (maxCurrent < 0.0f) maxCurrent = 0.0f;

    rampUp = preferences->getBool("vescRU", rampUp);

    rampDown = preferences->getBool("vescRD", rampDown);

    rampMinCurrentDiff = preferences->getFloat("vescRMCD", rampMinCurrentDiff);
    if (rampMinCurrentDiff < 0.0f) rampMinCurrentDiff = 0.0f;

    tmpUint = preferences->getUInt("vescRNS", rampNumSteps);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    rampNumSteps = tmpUint;

    tmpUint = preferences->getUInt("vescRT", rampTime);
    if (UINT16_MAX < tmpUint) tmpUint = UINT16_MAX;
    rampTime = tmpUint;

    tmpUint = preferences->getUInt("vescTMW", tMotorWarn);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    tMotorWarn = tmpUint;

    tmpUint = preferences->getUInt("vescTML", tMotorLimit);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    tMotorLimit = tmpUint;

    tmpUint = preferences->getUInt("vescTEW", tEscWarn);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    tEscWarn = tmpUint;

    tmpUint = preferences->getUInt("vescTEL", tEscLimit);
    if (UINT8_MAX < tmpUint) tmpUint = UINT8_MAX;
    tEscLimit = tmpUint;
}

void VescSettings::save(::Preferences *preferences) {
    preferences->putUInt("vescBNS", battNumSeries);
    preferences->putFloat("vescBC", battCapacityWh);
    preferences->putUInt("vescMP", maxPower);
    preferences->putFloat("vescMiC", minCurrent);
    preferences->putFloat("vescMaC", maxCurrent);
    preferences->putBool("vescRU", rampUp);
    preferences->putBool("vescRD", rampDown);
    preferences->putFloat("vescRMCD", rampMinCurrentDiff);
    preferences->putUInt("vescRNS", rampNumSteps);
    preferences->putUInt("vescRT", rampTime);
    preferences->putUInt("vescTMW", tMotorWarn);
    preferences->putUInt("vescTML", tMotorLimit);
    preferences->putUInt("vescTEW", tEscWarn);
    preferences->putUInt("vescTEL", tEscLimit);
}