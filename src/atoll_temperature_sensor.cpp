#ifdef FEATURE_TEMPERATURE
#include "atoll_temperature_sensor.h"
#include "atoll_log.h"

using namespace Atoll;

TemperatureSensor::TemperatureSensor(
    const char *label,
    float updateFrequency,
    Callback onValueChange)
    : onValueChange(onValueChange) {
    setLabel(label);
    taskSetFreq(updateFrequency);
}

TemperatureSensor::~TemperatureSensor() {}

void TemperatureSensor::begin() {
    taskStart();
}

void TemperatureSensor::loop() {
    float prevValue = value;
    if (update()) lastUpdate = millis();
    if (prevValue == value) return;
#ifdef FEATURE_BLE_SERVER
    if (bleChar) {
        uint16_t bleVal = (uint16_t)(value * 100);
        unsigned char buf[2];
        buf[0] = bleVal & 0xff;
        buf[1] = (bleVal >> 8) & 0xff;
        bleChar->setValue((uint8_t *)buf, sizeof(buf));
        bleChar->notify();
    }
#endif
    callOnValueChange();
}

void TemperatureSensor::setLabel(const char *label) {
    strncpy(this->label, label, sizeof(this->label));
    this->label[sizeof(this->label) - 1] = '\0';
}

void TemperatureSensor::updateValue(float newValue) {
    value = newValue;
}

void TemperatureSensor::callOnValueChange() {
    if (onValueChange) onValueChange(this);
}

#ifdef FEATURE_BLE_SERVER
void TemperatureSensor::addBleService(
    Atoll::BleServer *bleServer,
    BLEUUID serviceUuid,
    BLEUUID charUuid) {
    if (!bleServer) {
        log_e("bleServer is null");
        return;
    }

    bleService = bleServer->getService(serviceUuid);
    if (!bleService) bleService = bleServer->createService(serviceUuid);
    if (!bleService) {
        log_e("could not create service");
        return;
    }
    bleChar = bleService->getCharacteristic(charUuid);
    if (bleChar) {
        bleChar = nullptr;
        log_e("char already exists");
        return;
    }
    bleChar = bleService->createCharacteristic(
        charUuid,
        BLE_PROP::READ | BLE_PROP::NOTIFY,
        2);  // sint16 max_len
    if (!bleChar) {
        log_e("could not create char");
        return;
    }
    bleChar->setCallbacks(this);
    bleService->start();
    bleServer->advertiseService(serviceUuid);
}
#endif  // FEATURE_BLE_SERVER

#endif  // FEATURE_TEMPERATURE