#if !defined(__atoll_temperature_h) && defined(FEATURE_TEMPERATURE)
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
        uint32_t mutexTimeout = 1000);

    virtual ~Temperature() {}
    virtual const char *taskName() override { return label; }

    virtual void loop() override;

    bool update();

    static uint8_t getDeviceCount(DallasTemperature *bus);
    static bool getAddressByIndex(DallasTemperature *bus, uint8_t index, Address address);
    static void addressToStr(Address address, char *buf, size_t size);

   protected:
    DallasTemperature *dallas;
    Callback onTempChange;
    SemaphoreHandle_t *mutex;
    uint32_t mutexTimeout;
};

}  // namespace Atoll

#endif