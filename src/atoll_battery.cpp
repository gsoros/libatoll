#ifdef FEATURE_BATTERY

#include "atoll_battery.h"

using namespace Atoll;

Battery *Battery::instance = nullptr;

Battery::~Battery() {}

void Battery::setup(
    ::Preferences *p,
    int16_t pin,
    Battery *instance,
#ifdef FEATURE_API
    Api *api
#endif
#ifdef FEATURE_BLE_SERVER
    ,
    BleServer *bleServer
#endif
) {
    this->pin = (pin < 0 || UINT8_MAX < pin) ? ATOLL_BATTERY_PIN : (uint8_t)pin;
    this->instance = instance;

#ifdef FEATURE_BLE_SERVER
    this->bleServer = bleServer;
#endif
#ifdef FEATURE_API
    this->api = api;
#endif
    preferencesSetup(p, "Battery");
    loadSettings();
    // printSettings();
#ifdef FEATURE_BLE_SERVER
    addBleService();
#endif
    _voltageBuf.clear();

    if (nullptr == instance) return;
#ifdef FEATURE_API
    if (nullptr != api)
        api->addCommand(Api::Command("bat", batteryProcessor));
#endif
}

#ifdef FEATURE_BLE_SERVER
bool Battery::addBleService() {
    if (nullptr == bleServer) {
        log_i("bleServer is null, not adding battery service");
        return false;
    }
    BLEUUID serviceUuid = BLEUUID(BATTERY_SERVICE_UUID);
    BLEService *s = bleServer->createService(serviceUuid);
    if (nullptr == s) {
        log_e("could not create service");
        return false;
    }
    BLECharacteristic *c = s->createCharacteristic(
        BLEUUID(BATTERY_LEVEL_CHAR_UUID),
        BLE_PROP::READ | BLE_PROP::NOTIFY);
    if (nullptr == c) {
        log_e("could not create char");
        return false;
    }
    c->setValue(&level, 1);
    c->setCallbacks(bleServer);
    // BLEDescriptor *d = c->createDescriptor(
    //     BLEUUID(BATTERY_LEVEL_DESC_UUID),
    //     BLE_PROP::READ);
    // if (nullptr == d) {
    //     log_e("could not create descriptor");
    //     return false;
    // }
    // char perc[11] = "Percentage";
    // d->setValue((uint8_t *)perc, strlen(perc));
    if (!s->start()) {
        log_e("could not start service");
        return false;
    }
    bleServer->advertiseService(serviceUuid);
    // log_d("added ble service");
    return true;
}

void Battery::notifyChar(uint8_t *value) {
    if (nullptr == bleServer) return;
    BLECharacteristic *c = bleServer->getChar(
        BLEUUID(BATTERY_SERVICE_UUID),
        BLEUUID(BATTERY_LEVEL_CHAR_UUID));
    if (nullptr == c) {
        log_e("cannot find char");
        return;
    }
    c->setValue(value, 1);
    c->notify();
}
#endif  // FEATURE_BLE_SERVER

// returns true if notification was sent
bool Battery::report() {
    static ulong lastNotification = 0;
    static int16_t lastLevel = -1;
    const ulong delay = 10000;  // min. 10 secs between messages
    if ((uint8_t)lastLevel == level) return false;
    ulong t = millis();
    if (((t < delay) && (0 == lastNotification)) ||
        ((delay < t) && (lastNotification < t - delay))) {
        lastNotification = t;
        lastLevel = (int16_t)level;
        // log_i("%fV, %fVavg, %d%%", voltage, voltageAvg(), level);
#ifdef FEATURE_BLE_SERVER
        notifyChar(&level);
#else
        log_i("no bleServer");
#endif
        return true;
    }
    return false;
}

void Battery::loop() {
    float oldVoltage = voltage;
    measureVoltage();
    detectChargingState();
    calculateLevel();
    report();
}

void Battery::detectChargingState() {
    static ChargingState prevState = csUnknown;

    if (voltage < ATOLL_BATTERY_EMPTY) goto exit;

    {
        float avg = voltageAvg();

        if (avg < ATOLL_BATTERY_EMPTY) goto exit;

        if (ATOLL_BATTERY_CHARGE_START_VOLTAGE_RISE <= (voltage - avg)) {
            // log_d("charging %.2f => %.2f", avg, voltage);
            chargingState = csCharging;
        } else if (voltage < avg && voltage <= ATOLL_BATTERY_FULL - ATOLL_BATTERY_CHARGE_END_VOLTAGE_DROP) {
            // log_d("discharging %.2f => %.2f", avg, voltage);
            chargingState = csDischarging;
        }
        if (csUnknown != chargingState && prevState != chargingState) {
            log_i("%scharging %.2f => %.2f", csCharging == chargingState ? "" : "dis", avg, voltage);
#ifdef FEATURE_API
#ifdef FEATURE_BLE_SERVER
            if (nullptr == api || nullptr == bleServer) {
                log_e("api or bleServer is null");
                goto exit;
            }
            BLECharacteristic *c = bleServer->getChar(
                api->serviceUuid,
                BLEUUID(API_TX_CHAR_UUID));
            if (nullptr == c) goto exit;
            Api::Message msg = api->process("bat");
            char buf[32];
            snprintf(buf, sizeof(buf), "%d;%d=%s", msg.result->code, msg.commandCode, msg.reply);
            c->setValue((uint8_t *)buf, strlen(buf));
            c->notify();
#endif
#endif
        }
    }
exit:
    prevState = chargingState;
}

float Battery::voltageAvg() {
    float v = 0.0;
    for (decltype(_voltageBuf)::index_t i = 0; i < _voltageBuf.size(); i++)
        v += _voltageBuf[i] / _voltageBuf.size();
    if (v < ATOLL_BATTERY_EMPTY)
        v = ATOLL_BATTERY_EMPTY;
    else if (ATOLL_BATTERY_FULL < v)
        v = ATOLL_BATTERY_FULL;
    return v;
}

uint8_t Battery::calculateLevel() {
    level = calculateLevel(voltageAvg());
    return level;
}

uint8_t Battery::calculateLevel(float voltage, uint8_t cellCount, float cellEmpty, float cellFull) {
    if (cellCount < 1) cellCount = 1;
    if (1 < cellCount) voltage = voltage / cellCount;
    if (voltage < cellEmpty)
        voltage = cellEmpty;
    else if (cellFull < voltage)
        voltage = cellFull;

    // based on https://lygte-info.dk/review/batteries2012/Samsung%20INR18650-29E%202900mAh%20%28Blue%29%20UK.html
    // index: 3.2 + i / 100
    const uint8_t lookupTable[] = {
        /* 3.20 ... 3.29 */ 0, 1, 1, 2, 2, 3, 3, 4, 4, 5,
        /* 3.30 ... 3.39 */ 5, 6, 6, 7, 8, 9, 10, 11, 12, 13,
        /* 3.40 ... 3.49 */ 14, 15, 16, 17, 18, 20, 22, 24, 26, 28,
        /* 3.50 ... 3.59 */ 29, 30, 31, 33, 35, 37, 39, 41, 43, 45,
        /* 3.60 ... 3.69 */ 47, 48, 50, 51, 52, 54, 55, 56, 57, 58,
        /* 3.70 ... 3.79 */ 59, 61, 62, 64, 65, 67, 68, 69, 70, 71,
        /* 3.80 ... 3.89 */ 72, 73, 74, 75, 76, 77, 78, 79, 80, 81,
        /* 3.90 ... 3.99 */ 82, 83, 84, 85, 86, 87, 88, 89, 90, 91,
        /* 4.00 ... 4.09 */ 92, 93, 94, 95, 96, 97, 98, 99, 99, 99,
        /* 4.10 ... 4.19 */ 99, 99, 99, 100, 100, 100, 100, 100, 100, 100};
    int i = (int)((voltage - cellEmpty) * 100);
    if (i < 0)
        i = 0;
    else if (99 < i)
        i = 99;
    uint8_t level = lookupTable[i];

    // // assume linear relationship between voltage and soc
    // uint8_t level = map(voltage * 1000,
    //                     cellEmpty * 1000,
    //                     cellFull * 1000,
    //                     0,
    //                     100000) /
    //                 1000;

    // // polynomial approximation of the typical lipo discharge curve
    // // based on https://github.com/G6EJD/LiPo_Battery_Capacity_Estimator
    // double v = (double)voltage;
    // uint8_t level = (uint8_t)(2808.3808 * pow(v, 4) -
    //                           43560.9157 * pow(v, 3) +
    //                           252848.5888 * pow(v, 2) -
    //                           650767.4615 * v +
    //                           626532.5703);

    // log_d("%.2fV %ds = %d%%", voltage, cellCount, level);
    // if (100 < level) level = 100;

    return level;
}

float Battery::measureVoltage(bool useCorrection) {
    uint32_t sum = 0;
    uint8_t samples;
    for (samples = 1; samples <= 10; samples++) {
        sum += analogRead(ATOLL_BATTERY_PIN);
        delay(1);
    }
    uint32_t readMax = 4095 * samples;  // 2^12 - 1 (12bit adc)
    if (sum == readMax) log_e("overflow");
    pinVoltage =
        map(
            sum,
            0,
            readMax,
            0,
            330 * samples) /  // 3.3V
        samples /
        100.0;  // double division
    voltage = pinVoltage * corrF;
    if (voltage < ATOLL_BATTERY_EMPTY)
        voltage = ATOLL_BATTERY_EMPTY;
    else if (ATOLL_BATTERY_FULL < voltage)
        voltage = ATOLL_BATTERY_FULL;
    // log_i("Measured: (%d) = %fV ==> with correction: %fV", sum / samples, pinVoltage, voltage);
    _voltageBuf.push(voltage);
    return useCorrection ? voltage : pinVoltage;
}

void Battery::loadSettings() {
    if (!preferencesStartLoad()) return;
    if (!preferences->getBool("calibrated", false)) {
        log_e("not calibrated");
        preferencesEnd();
        return;
    }
    corrF = preferences->getFloat("corrF", 1.0);
    preferencesEnd();
}

void Battery::calibrateTo(float realVoltage) {
    float measuredVoltage = measureVoltage(false);
    if (-0.001 < measuredVoltage && measuredVoltage < 0.001)
        return;
    corrF = realVoltage / measuredVoltage;
}

void Battery::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putFloat("corrF", corrF);
    preferences->putBool("calibrated", true);
    preferencesEnd();
}

void Battery::printSettings() {
    log_i("correction factor: %f", corrF);
}

Atoll::Battery::ChargingState Battery::getChargingState() {
    return chargingState;
}

bool Battery::isCharging() {
    return csCharging == chargingState;
}

Api::Result *Battery::batteryProcessor(Api::Message *msg) {
    if (nullptr == instance) return Api::error();
    // set battery correction factor by supplying the measured voltage
    if (0 < strlen(msg->arg)) {
        float voltage = atof(msg->arg);
        if (ATOLL_BATTERY_EMPTY * 0.8F < voltage && voltage < ATOLL_BATTERY_FULL * 1.2F) {
            instance->calibrateTo(voltage);
            instance->saveSettings();
            instance->measureVoltage();
        }
    }
    // get current voltage and charging state
    snprintf(msg->reply, sizeof(msg->reply),
             "%.2f%s",
             instance->voltage,
             csCharging == instance->chargingState
                 ? "|charging"
             : csDischarging == instance->chargingState
                 ? "|discharging"
                 : "");
    return Api::success();
}

#endif  // FEATURE_BATTERY