#include "atoll_battery.h"

using namespace Atoll;

Battery *Battery::instance = nullptr;

Battery::~Battery() {}

void Battery::setup(
    ::Preferences *p,
    int16_t pin,
    Battery *instance,
    Api *api,
    BleServer *bleServer) {
    this->pin = (pin < 0 || UINT8_MAX < pin) ? ATOLL_BATTERY_PIN : (uint8_t)pin;
    this->instance = instance;
    this->bleServer = bleServer;

    preferencesSetup(p, "Battery");
    loadSettings();
    printSettings();

    addBleService();

    _voltageBuf.clear();

    if (nullptr == instance) return;
    if (nullptr != api)
        api->addCommand(ApiCommand("bat", batteryProcessor));
}

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
        NIMBLE_PROPERTY::READ | NIMBLE_PROPERTY::NOTIFY);
    if (nullptr == c) {
        log_e("could not create char");
        return false;
    }
    c->setValue(&level, 1);
    c->setCallbacks(bleServer);
    // BLEDescriptor *d = c->createDescriptor(
    //     BLEUUID(BATTERY_LEVEL_DESC_UUID),
    //     NIMBLE_PROPERTY::READ);
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
    log_i("added ble service");
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
        notifyChar(&level);
        return true;
    }
    return false;
}

void Battery::loop() {
    float oldVoltage = voltage;
    measureVoltage();
    detectChargingEvent(oldVoltage);
    calculateLevel();
    report();
}

void Battery::detectChargingEvent(float oldVoltage) {
    // log_i("%f %f", oldVoltage, voltage);
    if (oldVoltage < ATOLL_BATTERY_EMPTY) return;
    if (voltage < ATOLL_BATTERY_EMPTY) return;
    if (oldVoltage == ATOLL_BATTERY_EMPTY && voltage == ATOLL_BATTERY_FULL) return;
    if (oldVoltage == ATOLL_BATTERY_FULL && voltage == ATOLL_BATTERY_EMPTY) return;

    if ((oldVoltage < voltage) &&
        (ATOLL_BATTERY_CHARGE_START_VOLTAGE_RISE <= (voltage - oldVoltage)))
        log_i("TODO charge START event %.2f => %.2f", oldVoltage, voltage);
    else if (ATOLL_BATTERY_CHARGE_END_VOLTAGE_DROP <= (oldVoltage - voltage))
        log_i("TODO charge END event %.2f => %.2f", oldVoltage, voltage);
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

uint8_t Battery::calculateLevel(float voltage, uint8_t series) {
    if (series < 1) series = 1;
    if (1 < series) voltage = voltage / series;

    // assume linear relationship between voltage and charge level
    // level = map(voltage * 1000,
    //             ATOLL_BATTERY_EMPTY * 1000,
    //             ATOLL_BATTERY_FULL * 1000,
    //             0,
    //             100000) /
    //         1000;

    // polynomial approximation of the typical lipo discharge curve
    // based on https://github.com/G6EJD/LiPo_Battery_Capacity_Estimator
    double v = (double)voltage;
    uint8_t level = (uint8_t)(2808.3808 * pow(v, 4) -
                              43560.9157 * pow(v, 3) +
                              252848.5888 * pow(v, 2) -
                              650767.4615 * v +
                              626532.5703);
    log_d("%.2fV %ds = %d%", voltage, series, level);
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

ApiResult *Battery::batteryProcessor(ApiMessage *msg) {
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
    // get current voltage
    snprintf(msg->reply, sizeof(msg->reply), "%.2f", instance->voltage);
    return Api::success();
}