#ifndef __atoll_battery_h
#define __atoll_battery_h

#include <Arduino.h>
#include <Preferences.h>
#include <CircularBuffer.h>

#include "atoll_preferences.h"
#include "atoll_task.h"
#include "atoll_api.h"
#include "atoll_ble_server.h"

#ifndef ATOLL_BATTERY_PIN
#define ATOLL_BATTERY_PIN 35
#endif

#ifndef ATOLL_BATTERY_RINGBUF_SIZE
#define ATOLL_BATTERY_RINGBUF_SIZE 10  // circular buffer size
#endif

#ifndef ATOLL_BATTERY_EMPTY
#define ATOLL_BATTERY_EMPTY 3.2f
#endif

#ifndef ATOLL_BATTERY_FULL
#define ATOLL_BATTERY_FULL 4.2f
#endif

#ifndef ATOLL_BATTERY_CHARGE_START_VOLTAGE_RISE
#define ATOLL_BATTERY_CHARGE_START_VOLTAGE_RISE 0.075f
#endif

#ifndef ATOLL_BATTERY_CHARGE_END_VOLTAGE_DROP
#define ATOLL_BATTERY_CHARGE_END_VOLTAGE_DROP 0.075f
#endif

namespace Atoll {

class Battery : public Task, public Preferences {
   public:
    const char *taskName() { return "Battery"; }
    uint8_t pin = ATOLL_BATTERY_PIN;
    float corrF = 1.0;
    float voltage = 0.0;
    float pinVoltage = 0.0;
    uint8_t level = 0;

    virtual ~Battery();

    void setup(
        ::Preferences *p,
        int16_t pin = -1,
        Battery *instance = nullptr,
        Api *api = nullptr,
        BleServer *bleServer = nullptr);
    bool addBleService();
    void notifyChar(uint8_t *value);
    virtual bool report();
    void loop();
    void detectChargingState();
    uint8_t calculateLevel();
    static uint8_t calculateLevel(
        float voltage,
        uint8_t cellCount = 1,
        float cellEmpty = ATOLL_BATTERY_EMPTY,
        float cellFull = ATOLL_BATTERY_FULL);
    float voltageAvg();
    float measureVoltage(bool useCorrection = true);
    void loadSettings();
    void calibrateTo(float measuredVoltage);
    void saveSettings();
    void printSettings();

    static ApiResult *batteryProcessor(ApiMessage *reply);

    enum ChargingState {
        csUnknown,
        csDischarging,
        csCharging
    };
    ChargingState getChargingState();
    bool isCharging();

   protected:
    static Battery *instance;
    CircularBuffer<float, ATOLL_BATTERY_RINGBUF_SIZE> _voltageBuf;
    Api *api = nullptr;
    BleServer *bleServer = nullptr;
    ChargingState chargingState = csUnknown;
};

}  // namespace Atoll

#endif