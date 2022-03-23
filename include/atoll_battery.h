#ifndef __atoll_battery_h
#define __atoll_battery_h

#include <Arduino.h>
#include <Preferences.h>
#include <CircularBuffer.h>

#include "atoll_preferences.h"
#include "atoll_task.h"
#include "atoll_api.h"
#include "atoll_ble_server.h"

#ifndef BATTERY_PIN
#define BATTERY_PIN 35
#endif

#ifndef BATTERY_RINGBUF_SIZE
#define BATTERY_RINGBUF_SIZE 10  // circular buffer size
#endif

#ifndef BATTERY_EMPTY
#define BATTERY_EMPTY 3.2F
#endif

#ifndef BATTERY_FULL
#define BATTERY_FULL 4.2F
#endif

namespace Atoll {

class Battery : public Task, public Preferences {
   public:
    float corrF = 1.0;
    float voltage = 0.0;
    float pinVoltage = 0.0;
    uint8_t level = 0;
    static Battery *instance;
    BleServer *bleServer = nullptr;

    void setup(
        ::Preferences *p,
        Battery *instance = nullptr,
        Api *api = nullptr,
        BleServer *bleServer = nullptr);
    bool addBleService();
    void notifyChar(uint8_t *value);
    void output();
    void loop();
    int calculateLevel();
    float voltageAvg();
    float measureVoltage(bool useCorrection = true);
    void loadSettings();
    void calibrateTo(float realVoltage);
    void saveSettings();
    void printSettings();

    static ApiResult *batteryProcessor(ApiReply *reply);

   private:
    CircularBuffer<float, BATTERY_RINGBUF_SIZE> _voltageBuf;
};

}  // namespace Atoll

#endif