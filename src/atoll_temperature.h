#ifndef __atoll_temperature_h
#define __atoll_temperature_h

#include <OneWire.h>
#include <DallasTemperature.h>

#include "atoll_task.h"

namespace Atoll {

class Temperature : public Task {
   public:
    typedef uint8_t Address[8];
    typedef std::function<void(Temperature *)> Callback;

    char label[8] = "";
    Address address;
    float value = 0.0f;
    ulong lastUpdate = 0;

    Temperature(
        DallasTemperature *dallas,
        const char *label,
        Address address,
        float updateFrequency = 1.0f,
        Callback onTempChange = nullptr,
        SemaphoreHandle_t *mutex = nullptr,
        uint32_t mutexTimeout = 1000)
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

    virtual ~Temperature() {}
    const char *taskName() override { return label; }

    virtual void loop() override {
        float prevValue = value;
        if (update())
            lastUpdate = millis();
        if (onTempChange && prevValue != value) onTempChange(this);
    }

    bool update() {
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

    static uint8_t getDeviceCount(DallasTemperature *bus) {
        return bus->getDeviceCount();
    }

    static bool getAddressByIndex(DallasTemperature *bus, uint8_t index, Address address) {
        return bus->getAddress(address, index);
    }

    // e.g. "0x28 0x8a 0x85 0x0a 0x00 0x00 0x00 0x39"
    static void addressToStr(Address address, char *buf, size_t size) {
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

   protected:
    DallasTemperature *dallas;
    Callback onTempChange;
    SemaphoreHandle_t *mutex;
    uint32_t mutexTimeout;
};

}  // namespace Atoll

#endif