#ifdef FEATURE_TEMPERATURE
#include "atoll_temperature_sensor.h"
#include "atoll_log.h"

using namespace Atoll;

TemperatureSensor::TemperatureSensor(
    DallasTemperature *dallas,
    const char *label,
    Address address,
    float updateFrequency,
    Callback onTempChange,
    SemaphoreHandle_t *mutex,
    uint32_t mutexTimeout)
    : dallas(dallas),
      onTempChange(onTempChange),
      mutex(mutex),
      mutexTimeout(mutexTimeout) {
    strncpy(this->label, label, sizeof(this->label));
    this->label[sizeof(this->label) - 1] = '\0';
    if (!dallas) {
        log_e("%s no dallas", label);
    }
    if (!dallas->validAddress(address)) {
        log_e("%s invalid address", label);
    }
    for (uint8_t i = 0; i < sizeof(Address); i++) this->address[i] = address[i];
    taskSetFreq(updateFrequency);
    //_taskDebug();
}

void TemperatureSensor::loop() {
    float prevValue = value;
    if (update())
        lastUpdate = millis();
    if (onTempChange && prevValue != value) onTempChange(this);
}

bool TemperatureSensor::update() {
    if (mutex && !xSemaphoreTake(*mutex, (TickType_t)mutexTimeout) == pdTRUE) {
        log_e("%s could not aquire mutex in %dms", label, mutexTimeout);
        return false;
    }
    DallasTemperature::request_t req = dallas->requestTemperaturesByAddress(address);
    if (!req.result) {
        if (mutex) xSemaphoreGive(*mutex);
        log_d("%s request failed", label);
        return false;
    }
    float newValue = dallas->getTempC(address);
    if (mutex) xSemaphoreGive(*mutex);
    if (newValue < validMin || validMax < newValue) {
        log_e("%s out of range: %d", label, newValue);
        return false;
    }
    value = newValue;
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
    return true;
}

bool TemperatureSensor::setResolution(uint8_t resolution) {
    return dallas->setResolution(address, resolution);
}

uint8_t TemperatureSensor::getDeviceCount(DallasTemperature *dallas) {
    return dallas->getDeviceCount();
}

void TemperatureSensor::setResolution(DallasTemperature *dallas, uint8_t resolution) {
    dallas->setResolution(resolution);
}

bool TemperatureSensor::getAddressByIndex(DallasTemperature *dallas, uint8_t index, Address address) {
    return dallas->getAddress(address, index);
}

// e.g. "0x28 0x8a 0x85 0x0a 0x00 0x00 0x00 0x39"
void TemperatureSensor::addressToStr(Address address, char *buf, size_t size) {
    if (1 < size) snprintf(buf, size, "");
    if (size < sizeof(Address) * 5) {
        log_e("buffer too small");
        return;
    }
    char hex[4];
    for (uint8_t i = 0; i < sizeof(Address); i++) {
        strncat(buf, "0x\0", size - strlen(buf) - 1);
        if (address[i] < 16) strncat(buf, "0\0", size - strlen(buf) - 1);
        snprintf(hex, sizeof(hex), "%x ", address[i]);
        strncat(buf, hex, size - strlen(buf) - 1);
    }
    strncat(buf, "\0", size - strlen(buf) - 1);
}

#ifdef FEATURE_BLE_SERVER
void TemperatureSensor::addBleService(Atoll::BleServer *bleServer) {
    if (!bleServer) {
        log_e("bleServer is null");
        return;
    }
    // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.service.environmental_sensing.xml
    BLEUUID serviceUuid = BLEUUID(ENVIRONMENTAL_SENSING_SERVICE_UUID);
    bleService = bleServer->getService(serviceUuid);
    if (!bleService) bleService = bleServer->createService(serviceUuid);
    if (!bleService) {
        log_e("could not create service");
        return;
    }
    // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.temperature.xml
    BLEUUID charUuid = BLEUUID(TEMPERATURE_CHAR_UUID);
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