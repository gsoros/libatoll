#include "atoll_temperature.h"
#include "atoll_log.h"

using namespace Atoll;

Temperature::Temperature(
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

void Temperature::loop() {
    float prevValue = value;
    if (update())
        lastUpdate = millis();
    if (onTempChange && prevValue != value) onTempChange(this);
}

bool Temperature::update() {
    if (mutex && !xSemaphoreTake(*mutex, (TickType_t)mutexTimeout) == pdTRUE) {
        log_e("%s could not aquire mutex in %dms", label, mutexTimeout);
        return false;
    }
    DallasTemperature::request_t req = dallas->requestTemperaturesByAddress(address);
    if (!req.result) {
        if (mutex) xSemaphoreGive(*mutex);
        log_e("%s request failed", label);
        return false;
    }
    value = dallas->getTempC(address);
    if (mutex) xSemaphoreGive(*mutex);
    return true;
}

uint8_t Temperature::getDeviceCount(DallasTemperature *bus) {
    return bus->getDeviceCount();
}

bool Temperature::getAddressByIndex(DallasTemperature *bus, uint8_t index, Address address) {
    return bus->getAddress(address, index);
}

// e.g. "0x28 0x8a 0x85 0x0a 0x00 0x00 0x00 0x39"
void Temperature::addressToStr(Address address, char *buf, size_t size) {
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