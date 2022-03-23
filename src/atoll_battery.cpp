#include "atoll_battery.h"

using namespace Atoll;

Battery *Battery::instance = nullptr;

void Battery::setup(
    ::Preferences *p,
    Battery *instance,
    Api *api,
    BleServer *bleServer) {
    this->instance = instance;
    this->bleServer = bleServer;

    preferencesSetup(p, "Battery");
    loadSettings();
    printSettings();

    addBleService();

    if (nullptr == instance) return;
    if (nullptr != api)
        api->addCommand(ApiCommand("battery", batteryProcessor));
}

bool Battery::addBleService() {
    if (nullptr == bleServer) {
        log_i("bleServer is null, not adding battery service");
        return false;
    }
    BLEService *s = bleServer->createService(BLEUUID(BATTERY_SERVICE_UUID));
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
    BLEDescriptor *d = c->createDescriptor(
        BLEUUID(BATTERY_LEVEL_DESC_UUID),
        NIMBLE_PROPERTY::READ);
    if (nullptr == d) {
        log_e("could not create descriptor");
        return false;
    }
    char perc[11] = "Percentage";
    d->setValue((uint8_t *)perc, strlen(perc));
    if (!s->start()) {
        log_e("could not start service");
        return false;
    }
    bleServer->advertiseService(BLEUUID(BATTERY_SERVICE_UUID));
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

void Battery::output() {
    static ulong lastNotification = 0;
    static int16_t lastLevel = -1;
    const ulong delay = 10000;  // min. 10 secs between messages
    if ((uint8_t)lastLevel == level) return;
    ulong t = millis();
    if (((t < delay) && (0 == lastNotification)) ||
        ((delay < t) && (lastNotification < t - delay))) {
        lastNotification = t;
        lastLevel = (int16_t)level;
        log_i("%fV, %fVavg, %d%%", voltage, voltageAvg(), level);
        notifyChar(&level);
    }
}

void Battery::loop() {
    measureVoltage();
    calculateLevel();
    output();
}

float Battery::voltageAvg() {
    float voltage = 0.0;
    for (decltype(_voltageBuf)::index_t i = 0; i < _voltageBuf.size(); i++)
        voltage += _voltageBuf[i] / _voltageBuf.size();
    if (voltage < BATTERY_EMPTY)
        voltage = BATTERY_EMPTY;
    else if (BATTERY_FULL < voltage)
        voltage = BATTERY_FULL;
    return voltage;
}

int Battery::calculateLevel() {
    // level = map(voltageAvg() * 1000,
    //             BATTERY_EMPTY * 1000,
    //             BATTERY_FULL * 1000,
    //             0,
    //             100000) /
    //         1000;
    // log_i("old method: %d", level);

    // based on https://github.com/G6EJD/LiPo_Battery_Capacity_Estimator
    double v = (double)voltageAvg();
    level = (uint8_t)(2808.3808 * pow(v, 4) -
                      43560.9157 * pow(v, 3) +
                      252848.5888 * pow(v, 2) -
                      650767.4615 * v +
                      626532.5703);
    // log_i("new method: %d", level);
    if (100 < level)
        level = 100;
    return level;
}

float Battery::measureVoltage(bool useCorrection) {
    uint32_t sum = 0;
    uint8_t samples;
    for (samples = 1; samples <= 10; samples++) {
        sum += analogRead(BATTERY_PIN);
        delay(1);
    }
    uint32_t readMax = 4095 * samples;  // 2^12 - 1 (12bit adc)
    if (sum == readMax) log_e("[Battery] Overflow");
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
    // log_i("Measured: (%d) = %fV ==> with correction: %fV", sum / samples, pinVoltage, voltage);
    _voltageBuf.push(voltage);
    return useCorrection ? voltage : pinVoltage;
}

void Battery::loadSettings() {
    if (!preferencesStartLoad()) return;
    if (!preferences->getBool("calibrated", false)) {
        log_e("[Battery] Not calibrated");
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
    log_i("[Battery] Correction factor: %f", corrF);
}

ApiResult *Battery::batteryProcessor(ApiReply *reply) {
    if (nullptr == instance) return Api::error();
    // set battery correction factor by supplying the measured voltage
    if (0 < strlen(reply->arg)) {
        float voltage = atof(reply->arg);
        if (BATTERY_EMPTY * 0.8F < voltage && voltage < BATTERY_FULL * 1.2F) {
            instance->calibrateTo(voltage);
            instance->saveSettings();
            instance->measureVoltage();
        }
    }
    // get current voltage
    snprintf(reply->value, sizeof(reply->value), "%f", instance->voltage);
    return Api::success();
}