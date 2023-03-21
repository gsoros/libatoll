#if !defined(__atoll_ds18b20_h) && defined(FEATURE_TEMPERATURE)
#define __atoll_ds18b20_h

#include <OneWire.h>
#include <DallasTemperature.h>

#include "atoll_temperature_sensor.h"

namespace Atoll {

class DS18B20 : public TemperatureSensor {
   public:
    enum Mode {
        TSM_EXCLUSIVE,  // single sensor on the pin
        TSM_SHARED      // multiple sensors on the same pin
    };

    typedef std::function<void(DS18B20 *)> Callback;
    typedef uint8_t Address[8];

    Address address = {0, 0, 0, 0, 0, 0, 0, 0};  // 8 Bytes
    uint8_t resolution = 11;                     // bits

    // multiple sensors on the same pin
    DS18B20(
        DallasTemperature *dallas,
        const char *label,
        Address address,
        SemaphoreHandle_t *mutex,
        uint8_t resolution = 11,
        uint32_t mutexTimeout = 1000,
        float updateFrequency = 1.0f,
        Callback onValueChange = nullptr);

    // single sensor on the pin
    DS18B20(
        gpio_num_t pin,
        const char *label,
        uint8_t resolution = 11,
        float updateFrequency = 1.0f,
        Callback onValueChange = nullptr);

    virtual ~DS18B20() override;

    virtual void begin() override;

    virtual bool update();

    virtual void setAddress(const Address address);
    virtual bool setResolution(uint8_t resolution);
    static uint8_t getDeviceCount(DallasTemperature *dallas);
    static bool getAddressByIndex(DallasTemperature *dallas, uint8_t index, Address address);
    // set global resolution
    static void setResolution(DallasTemperature *dallas, uint8_t resolution);
    static void addressToStr(Address address, char *buf, size_t size);

   protected:
    virtual bool aquireMutex();
    virtual void releaseMutex();

    virtual void callOnValueChange() override;
    Callback onValueChange = nullptr;

    uint8_t mode = TSM_EXCLUSIVE;
    DallasTemperature *dallas = nullptr;  //
    gpio_num_t pin = GPIO_NUM_NC;         // used in exclusive mode only
    OneWire *bus = nullptr;               // used in exclusive mode only
    SemaphoreHandle_t *mutex = nullptr;   // used in shared mode only
    uint32_t mutexTimeout = 1000;         // used in shared mode only
};

}  // namespace Atoll

#endif