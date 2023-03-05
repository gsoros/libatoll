#ifndef __atoll_temperature_h
#define __atoll_temperature_h

#include <OneWire.h>
#include <DallasTemperature.h>

#include "atoll_task.h"

#ifndef ONE_WIRE_PIN
// #define ONE_WIRE_PIN 25
#error ONE_WIRE_PIN not defined
#endif

namespace Atoll {

class Temperature : public Task {
   public:
    typedef uint8_t Address[8];

    Temperature(const char *label, Address address);
    virtual ~Temperature() {}

    char label[8] = "";
    float value = 0.0f;

   protected:
    virtual void setup();
    virtual void loop();
    const char *taskName() { return label; }

    Address address;
    std::function<void(void *)> onValueChange = nullptr;
    ulong lastUpdate = 0;
};

}  // namespace Atoll

#endif