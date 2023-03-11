#if !defined(__atoll_temperature_sensor_h) && defined(FEATURE_TEMPERATURE)
#define __atoll_temperature_sensor_h

#include <OneWire.h>
#include <DallasTemperature.h>

#include "atoll_task.h"

#ifdef FEATURE_BLE_SERVER
#include "atoll_ble_server.h"
#endif

namespace Atoll {

class TemperatureSensor : public Task
#ifdef FEATURE_BLE_SERVER
    ,
                          public BleCharacteristicCallbacks
#endif
{
   public:
    enum Mode {
        TSM_EXCLUSIVE,  // single sensor on the pin
        TSM_SHARED      // multiple sensors on the same pin
    };

    typedef uint8_t Address[8];
    typedef std::function<void(TemperatureSensor *)> Callback;

    char label[8] = "";
    Address address = {0, 0, 0, 0, 0, 0, 0, 0};
    float value = 0.0f;       // ˚C
    float validMin = -50.0f;  // ˚C
    float validMax = 100.0f;  // ˚C
    ulong lastUpdate = 0;
    uint8_t resolution = 11;

    // multiple sensors on the same pin
    TemperatureSensor(
        DallasTemperature *dallas,
        const char *label,
        Address address,
        SemaphoreHandle_t *mutex,
        uint8_t resolution = 11,
        uint32_t mutexTimeout = 1000,
        float updateFrequency = 1.0f,
        Callback onValueChange = nullptr);

    // single sensor on the pin
    TemperatureSensor(
        gpio_num_t pin,
        const char *label,
        uint8_t resolution = 11,
        float updateFrequency = 1.0f,
        Callback onValueChange = nullptr);

    virtual ~TemperatureSensor();
    virtual const char *taskName() override { return label; }

    virtual void begin();
    virtual void loop() override;

    virtual bool update();
    virtual void setAddress(const Address address);
    virtual void setLabel(const char *label);
    virtual bool setResolution(uint8_t resolution);

    static uint8_t getDeviceCount(DallasTemperature *dallas);
    static bool getAddressByIndex(DallasTemperature *dallas, uint8_t index, Address address);
    static void setResolution(DallasTemperature *dallas, uint8_t resolution);  // set global resolution
    static void addressToStr(Address address, char *buf, size_t size);

#ifdef FEATURE_BLE_SERVER
    BLEService *bleService = nullptr;
    BLECharacteristic *bleChar = nullptr;
    virtual void addBleService(Atoll::BleServer *bleServer);
#endif

   protected:
    virtual bool aquireMutex();
    virtual void releaseMutex();

    uint8_t mode = TSM_EXCLUSIVE;
    DallasTemperature *dallas = nullptr;  //
    Callback onValueChange = nullptr;     //
    gpio_num_t pin = GPIO_NUM_NC;         // used in exclusive mode only
    OneWire *bus = nullptr;               // used in exclusive mode only
    SemaphoreHandle_t *mutex = nullptr;   // used in shared mode only
    uint32_t mutexTimeout = 1000;         // used in shared mode only
};

}  // namespace Atoll

#endif