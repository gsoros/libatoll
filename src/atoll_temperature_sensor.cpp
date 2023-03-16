#ifdef FEATURE_TEMPERATURE
#include "atoll_temperature_sensor.h"
#include "atoll_log.h"

using namespace Atoll;

/// @brief constructor for multiple sensors on the same pin
/// @param dallas
/// @param label
/// @param address
/// @param mutex
/// @param resolution
/// @param mutexTimeout
/// @param updateFrequency
/// @param onTempChange
TemperatureSensor::TemperatureSensor(
    DallasTemperature *dallas,
    const char *label,
    Address address,
    SemaphoreHandle_t *mutex,
    uint8_t resolution,
    uint32_t mutexTimeout,
    float updateFrequency,
    Callback onTempChange)
    : dallas(dallas),
      mutex(mutex),
      resolution(resolution),
      mutexTimeout(mutexTimeout),
      onValueChange(onTempChange) {
    mode = TSM_SHARED;
    setLabel(label);
    if (!dallas) {
        log_e("%s no dallas", label);
    } else if (!dallas->validAddress(address)) {
        log_e("%s invalid address", label);
    } else
        setAddress(address);
    taskSetFreq(updateFrequency);
}

/// @brief constructor for single sensor on the pin
/// @param pin
/// @param label
/// @param resolution
/// @param updateFrequency
/// @param onValueChange
TemperatureSensor::TemperatureSensor(
    gpio_num_t pin,
    const char *label,
    uint8_t resolution,
    float updateFrequency,
    Callback onValueChange)
    : resolution(resolution),
      onValueChange(onValueChange) {
    mode = TSM_EXCLUSIVE;
    setLabel(label);
    this->pin = pin;
    bus = new OneWire();
    dallas = new DallasTemperature(bus);
    taskSetFreq(updateFrequency);
}

TemperatureSensor::~TemperatureSensor() {
    if (TSM_EXCLUSIVE == mode) {
        if (dallas) delete dallas;
        if (bus) delete bus;
    }
}

void TemperatureSensor::begin() {
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
    taskStart();
}

void TemperatureSensor::loop() {
    float prevValue = value;
    if (update()) lastUpdate = millis();
    if (prevValue != value) {
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
        if (onValueChange) onValueChange(this);
    }
}

bool TemperatureSensor::update() {
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

void TemperatureSensor::setAddress(const Address address) {
    for (uint8_t i = 0; i < sizeof(Address); i++) this->address[i] = address[i];
}

void TemperatureSensor::setLabel(const char *label) {
    strncpy(this->label, label, sizeof(this->label));
    this->label[sizeof(this->label) - 1] = '\0';
}

bool TemperatureSensor::setResolution(uint8_t resolution) {
    this->resolution = resolution;
    if (!dallas->setResolution(address, resolution)) {
        log_e("%s could not set resolution %d", label, resolution);
        return false;
    }
    return true;
}

uint8_t TemperatureSensor::getDeviceCount(DallasTemperature *dallas) {
    return dallas->getDeviceCount();
}

// set global resolution
void TemperatureSensor::setResolution(DallasTemperature *dallas, uint8_t resolution) {
    dallas->setResolution(resolution);
}

bool TemperatureSensor::getAddressByIndex(DallasTemperature *dallas, uint8_t index, Address address) {
    return dallas->getAddress(address, index);
}

/// @brief Put the string representation of address into buf, e.g. "0x28 0x8a 0x85 0x0a 0x00 0x00 0x00 0x39"
/// @param address
/// @param buf
/// @param size
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

void TemperatureSensor::updateValue(float newValue) {
    value = newValue;
}

bool TemperatureSensor::aquireMutex() {
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

void TemperatureSensor::releaseMutex() {
    if (TSM_EXCLUSIVE == mode) return;
    if (mutex) xSemaphoreGive(*mutex);
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