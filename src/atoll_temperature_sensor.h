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
    typedef uint8_t Address[8];
    typedef std::function<void(TemperatureSensor *)> Callback;

    char label[8] = "";
    Address address;
    float value = 0.0f;       // ˚C
    float validMin = -50.0f;  // ˚C
    float validMax = 100.0f;  // ˚C
    ulong lastUpdate = 0;

    TemperatureSensor(
        DallasTemperature *dallas,
        const char *label,
        Address address,
        float updateFrequency = 1.0f,
        Callback onTempChange = nullptr,
        SemaphoreHandle_t *mutex = nullptr,
        uint32_t mutexTimeout = 1000);

    virtual ~TemperatureSensor() {}
    virtual const char *taskName() override { return label; }

    virtual void loop() override;

    virtual bool update();
    virtual bool setResolution(uint8_t resolution);

    static uint8_t getDeviceCount(DallasTemperature *dallas);
    static bool getAddressByIndex(DallasTemperature *dallas, uint8_t index, Address address);
    static void setResolution(DallasTemperature *dallas, uint8_t resolution);
    static void addressToStr(Address address, char *buf, size_t size);

#ifdef FEATURE_BLE_SERVER
    BLEService *bleService = nullptr;
    BLECharacteristic *bleChar = nullptr;
    virtual void addBleService(Atoll::BleServer *bleServer);
#endif

   protected:
    DallasTemperature *dallas;
    Callback onTempChange;
    SemaphoreHandle_t *mutex;
    uint32_t mutexTimeout;
};

}  // namespace Atoll

#endif