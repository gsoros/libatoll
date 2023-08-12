#if defined(FEATURE_TEMPERATURE) && defined(FEATURE_DS18B20)
#include "atoll_ds18b20.h"
#include "atoll_log.h"

using namespace Atoll;

DS18B20::DS18B20(
    DallasTemperature *dallas,
    const char *label,
    Address address,
    SemaphoreHandle_t *mutex,
    uint8_t resolution,
    uint32_t mutexTimeout,
    float updateFrequency,
    Callback onValueChange)
    : TemperatureSensor(label, updateFrequency),
      onValueChange(onValueChange),
      dallas(dallas),
      mutex(mutex),
      resolution(resolution),
      mutexTimeout(mutexTimeout) {
    mode = TSM_SHARED;
    if (!dallas) {
        log_e("%s no dallas", label);
    } else if (!dallas->validAddress(address)) {
        log_e("%s invalid address", label);
    } else
        setAddress(address);
}

DS18B20::DS18B20(
    gpio_num_t pin,
    const char *label,
    uint8_t resolution,
    float updateFrequency,
    Callback onValueChange)
    : TemperatureSensor(label, updateFrequency),
      onValueChange(onValueChange),
      pin(pin) {
    mode = TSM_EXCLUSIVE;
    bus = new OneWire();
    dallas = new DallasTemperature(bus);
}

DS18B20::~DS18B20() {
    if (TSM_EXCLUSIVE == mode) {
        if (dallas) delete dallas;
        if (bus) delete bus;
    }
}

void DS18B20::begin() {
    if (TSM_EXCLUSIVE == mode) {
        bus->begin(pin);
        dallas->begin();
        log_d("%d sensor(s), %sparasitic",
              getDeviceCount(dallas),
              dallas->isParasitePowerMode() ? "" : "not ");
        Address address;
        if (!getAddressByIndex(dallas, 0, address)) {
            log_e("%s could not get address", label);
            return;
        }
        char buf[40];
        addressToStr(address, buf, sizeof(buf));
        log_d("%s found at %s", label, buf);
        setAddress(address);
    }
    setResolution(resolution);
    TemperatureSensor::begin();
}

bool DS18B20::update() {
    if (!dallas || !aquireMutex()) return false;
    DallasTemperature::request_t req = dallas->requestTemperaturesByAddress(address);
    if (!req.result) {
        releaseMutex();
        log_d("%s request failed", label);
        return false;
    }
    float newValue = dallas->getTempC(address);
    releaseMutex();
    if (DEVICE_DISCONNECTED_C == newValue) {
        log_e("%s disconnected", label);
        return false;
    }
    if (newValue < validMin || validMax < newValue) {
        log_e("%s out of range: %.2f", label, newValue);
        return false;
    }
    updateValue(newValue);
    return true;
}

void DS18B20::setAddress(const Address address) {
    for (uint8_t i = 0; i < sizeof(Address); i++) this->address[i] = address[i];
}

bool DS18B20::setResolution(uint8_t resolution) {
    this->resolution = resolution;
    if (!dallas->setResolution(address, resolution)) {
        log_e("%s could not set resolution %d", label, resolution);
        return false;
    }
    return true;
}

uint8_t DS18B20::getDeviceCount(DallasTemperature *dallas) {
    return dallas->getDeviceCount();
}

// set global resolution
void DS18B20::setResolution(DallasTemperature *dallas, uint8_t resolution) {
    dallas->setResolution(resolution);
}

bool DS18B20::getAddressByIndex(DallasTemperature *dallas, uint8_t index, Address address) {
    return dallas->getAddress(address, index);
}

/// @brief Put the string representation of address into buf, e.g. "0x28 0x8a 0x85 0x0a 0x00 0x00 0x00 0x39"
/// @param address
/// @param buf
/// @param size
void DS18B20::addressToStr(Address address, char *buf, size_t size) {
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

bool DS18B20::aquireMutex() {
    if (TSM_EXCLUSIVE == mode) return true;
    if (!mutex) {
        log_e("%s no mutex", label);
        return false;
    }
    if (!xSemaphoreTake(*mutex, (TickType_t)mutexTimeout) == pdTRUE) {
        log_e("%s could not aquire mutex in %dms", label, mutexTimeout);
        return false;
    }
    return true;
}

void DS18B20::releaseMutex() {
    if (TSM_EXCLUSIVE == mode) return;
    if (mutex) xSemaphoreGive(*mutex);
}

void DS18B20::callOnValueChange() {
    if (onValueChange) onValueChange(this);
}

#endif  // FEATURE_TEMPERATURE