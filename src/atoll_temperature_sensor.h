#if !defined(__atoll_temperature_sensor_h) && defined(FEATURE_TEMPERATURE)
#define __atoll_temperature_sensor_h

#include "atoll_preferences.h"

#ifdef FEATURE_BLE_SERVER
#include "atoll_ble_server.h"
#endif

namespace Atoll {

class TemperatureSensor : public Task, public Preferences
#ifdef FEATURE_BLE_SERVER
    ,
                          public BleCharacteristicCallbacks
#endif
{
   public:
    typedef std::function<void(TemperatureSensor *)> Callback;

    char label[8] = "";
    float value = 0.0f;       // ˚C
    float validMin = -50.0f;  // ˚C
    float validMax = 100.0f;  // ˚C
    ulong lastUpdate = 0;     // ms
    float offset = 0.0f;      // ˚C

    TemperatureSensor(
        const char *label,
        float updateFrequency = 1.0f,
        Callback onValueChange = nullptr);

    virtual ~TemperatureSensor();

    virtual void setup(::Preferences *p = nullptr);
    virtual void begin();
    virtual void loop();
    virtual bool update() = 0;
    virtual void setLabel(const char *label);

#ifdef FEATURE_BLE_SERVER
    // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.service.environmental_sensing.xml
    // https://github.com/oesmith/gatt-xml/blob/master/org.bluetooth.characteristic.temperature.xml
    BLEService *bleService = nullptr;
    BLECharacteristic *bleChar = nullptr;
    virtual void addBleService(
        Atoll::BleServer *bleServer,
        BLEUUID serviceUuid = BLEUUID(ENVIRONMENTAL_SENSING_SERVICE_UUID),
        BLEUUID charUuid = BLEUUID(TEMPERATURE_CHAR_UUID));
#endif

    virtual const char *taskName() override { return label; }

    void loadSettings();
    void saveSettings();
    void printSettings();

   protected:
    virtual void updateValue(float newValue);

    virtual void callOnValueChange();
    Callback onValueChange = nullptr;
};

}  // namespace Atoll

#endif