#include "atoll_wifi.h"
#include "atoll_serial.h"

using namespace Atoll;

Wifi *Wifi::instance;
Ota *Wifi::ota;
Recorder *Wifi::recorder;
WifiSerial *Wifi::wifiSerial;

void Wifi::setup(
    const char *hostName,
    ::Preferences *p,
    const char *preferencesNS,
    Wifi *instance,
    Api *api,
    Ota *ota,
    Recorder *recorder,
    WifiSerial *wifiSerial) {
    strncpy(this->hostName, hostName, sizeof(this->hostName));
    preferencesSetup(p, preferencesNS);
    this->instance = instance;
    this->ota = ota;
    this->recorder = recorder;
    this->wifiSerial = wifiSerial;

    loadDefaultSettings();
    loadSettings();
    applySettings();
    registerCallbacks();

    if (nullptr == instance) return;
    if (nullptr == api) return;
    api->addCommand(ApiCommand("wifi", enabledProcessor));
    api->addCommand(ApiCommand("wifiAp", apProcessor));
    api->addCommand(ApiCommand("wifiApSSID", apSSIDProcessor));
    api->addCommand(ApiCommand("wifiApPassword", apPasswordProcessor));
    api->addCommand(ApiCommand("wifiSta", staProcessor));
    api->addCommand(ApiCommand("wifiStaSSID", staSSIDProcessor));
    api->addCommand(ApiCommand("wifiStaPassword", staPasswordProcessor));
};

void Wifi::loop() {
    taskStop();
};

void Wifi::off() {
    log_i("shutting down");
    settings.enabled = false;
    applySettings();
    taskStop();
};

void Wifi::loadSettings() {
    if (!preferencesStartLoad()) return;
    settings.enabled = preferences->getBool("enabled", settings.enabled);
    settings.apEnabled = preferences->getBool("apEnabled", settings.apEnabled);
    strncpy(settings.apSSID,
            preferences->getString("apSSID", settings.apSSID).c_str(),
            sizeof(settings.apSSID));
    strncpy(settings.apPassword,
            preferences->getString("apPassword", settings.apPassword).c_str(),
            sizeof(settings.apPassword));
    settings.staEnabled = preferences->getBool("staEnabled", settings.staEnabled);
    strncpy(settings.staSSID,
            preferences->getString("staSSID", settings.staSSID).c_str(),
            sizeof(settings.staSSID));
    strncpy(settings.staPassword,
            preferences->getString("staPassword", settings.staPassword).c_str(),
            sizeof(settings.staPassword));
    preferencesEnd();
};

void Wifi::loadDefaultSettings() {
    settings.enabled = true;
    settings.apEnabled = true;
    strncpy(settings.apSSID, hostName, sizeof(settings.apSSID));
    strncpy(settings.apPassword, "", sizeof(settings.apPassword));
    settings.staEnabled = false;
    strncpy(settings.staSSID, "", sizeof(settings.staSSID));
    strncpy(settings.staPassword, "", sizeof(settings.staPassword));
};

void Wifi::saveSettings() {
    if (!preferencesStartSave()) return;
    preferences->putBool("enabled", settings.enabled);
    preferences->putBool("apEnabled", settings.apEnabled);
    preferences->putString("apSSID", settings.apSSID);
    preferences->putString("apPassword", settings.apPassword);
    preferences->putBool("staEnabled", settings.staEnabled);
    preferences->putString("staSSID", settings.staSSID);
    preferences->putString("staPassword", settings.staPassword);
    preferencesEnd();
};

void Wifi::printSettings() {
    log_i("Wifi %sabled",
          settings.enabled ? "En" : "Dis");
    printAPSettings();
    printSTASettings();
};

void Wifi::printAPSettings() {
    log_i("AP %sabled '%s' '%s'",
          settings.apEnabled ? "En" : "Dis",
          settings.apSSID,
          "***"  // settings.apPassword
    );
    if (settings.apEnabled)
        log_i("AP online, IP: %s", WiFi.softAPIP().toString().c_str());
};

void Wifi::printSTASettings() {
    log_i("STA %sabled '%s' '%s'",
          settings.staEnabled ? "En" : "Dis",
          settings.staSSID,
          "***"  // settings.staPassword
    );
    if (WiFi.isConnected())
        log_i("STA connected, local IP: %s", WiFi.localIP().toString().c_str());
};

void Wifi::applySettings() {
    log_i("Applying settings, connections will be reset");
#ifdef FEATURE_SERIAL
    Serial.flush();
#endif
    // delay(1000);
    wifi_mode_t oldMode = WiFi.getMode();
    wifi_mode_t wifiMode;
    if (settings.enabled && (settings.apEnabled || settings.staEnabled)) {
        if (settings.apEnabled && settings.staEnabled)
            wifiMode = WIFI_MODE_APSTA;
        else if (settings.apEnabled)
            wifiMode = WIFI_MODE_AP;
        else
            wifiMode = WIFI_MODE_STA;
    } else
        wifiMode = WIFI_MODE_NULL;
    WiFi.mode(wifiMode);
    if (settings.enabled && settings.apEnabled) {
        if (0 == strcmp("", const_cast<char *>(settings.apSSID))) {
            log_w("cannot enable AP with empty SSID");
            settings.apEnabled = false;
        } else {
            log_i("setting up AP '%s'", settings.apSSID);
            WiFi.softAP(settings.apSSID, settings.apPassword);
        }
    }
    if (settings.enabled && settings.staEnabled) {
        if (0 == strcmp("", const_cast<char *>(settings.staSSID))) {
            log_w("cannot enable STA with empty SSID");
            settings.staEnabled = false;
        } else {
            log_i("connecting to AP '%s'", settings.staSSID);
            WiFi.begin(settings.staSSID, settings.staPassword);
        }
    }

    /*
    if (oldMode == wifiMode) {
        log_i("mode unchanged, not manipulating ota and wifiserial");
        return;
    }
    if (wifiMode == WIFI_MODE_NULL) {
        if (nullptr != ota) {
            log_i("stopping Ota");
            ota->taskStop();
        }
#ifdef FEATURE_SERIAL
        if (nullptr != wifiSerial) {
            log_i("stopping wifiSerial");
            wifiSerial->off();
        }
#endif
    } else {
        if (nullptr != ota) {
            log_i("restarting Ota");
            ota->off();
            ota->taskStop();
            ota->setup(hostName, recorder);
            ota->taskStart(ATOLL_OTA_TASK_FREQ);
        }
#ifdef FEATURE_SERIAL
        if (nullptr != wifiSerial) {
            log_i("restarting wifiSerial");
            wifiSerial->off();
            wifiSerial->setup();
            wifiSerial->taskStart();
        }
#endif
    }
    */
};

void Wifi::setEnabled(bool state, bool save) {
    settings.enabled = state;
    applySettings();
    if (save) saveSettings();
};

bool Wifi::isEnabled() {
    return settings.enabled;
};

bool Wifi::isConnected() {
    return WiFi.isConnected() || WiFi.softAPgetStationNum();
};

void Wifi::registerCallbacks() {
    WiFi.onEvent(
        [this](arduino_event_id_t event, arduino_event_info_t info) {
            onEvent(event, info);
        },
        ARDUINO_EVENT_MAX);
}

void Wifi::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            log_i("[AP] station connected, now active: %d", WiFi.softAPgetStationNum());
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            log_i("[AP] station got IP");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            log_i("[AP] station disconnected, now active: %d", WiFi.softAPgetStationNum());
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            log_i("[STA] starting");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            log_i("[STA] connected");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            log_i("[STA] got IP: %s", WiFi.localIP().toString().c_str());
            // WiFi.setAutoReconnect(true);
            // WiFi.persistent(true);
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            log_i("[STA] disconnected");
            if (settings.enabled && settings.staEnabled) {
                log_i("[STA] reconnecting");
                // WiFi.disconnect();
                WiFi.reconnect();
                // WiFi.begin();
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            log_i("[STA] stopped");
            break;
        default:
            log_i("event: %d, info: %d", event, info);
    }

    static bool prevConnected = false;
    bool connected = isConnected();

    if (prevConnected != connected) {
        if (connected) {
            if (nullptr != ota) {
                log_i("restarting Ota");
                ota->off();
                ota->taskStop();
                ota->setup(hostName, recorder);
                ota->taskStart(ATOLL_OTA_TASK_FREQ);
            }
#ifdef FEATURE_SERIAL
            if (nullptr != wifiSerial) {
                log_i("restarting wifiSerial");
                wifiSerial->off();
                wifiSerial->setup();
                wifiSerial->taskStart();
            }
#endif
        } else {
            if (nullptr != ota) {
                log_i("stopping Ota");
                ota->taskStop();
            }
#ifdef FEATURE_SERIAL
            if (nullptr != wifiSerial) {
                log_i("stopping wifiSerial");
                wifiSerial->off();
            }
#endif
        }
    }
    prevConnected = connected;
}

ApiResult *Wifi::enabledProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    // set wifi
    bool newValue = true;  // enable wifi by default
    if (0 < strlen(msg->arg)) {
        if (0 == strcmp("false", msg->arg) || 0 == strcmp("0", msg->arg))
            newValue = false;
        instance->setEnabled(newValue);
    }
    // get wifi
    strncpy(msg->reply, instance->isEnabled() ? "1" : "0", sizeof(msg->reply));
    return Api::success();
}

ApiResult *Wifi::apProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    // set
    bool newValue = true;  // enable by default
    if (0 < strlen(msg->arg)) {
        if (0 == strcmp("false", msg->arg) ||
            0 == strcmp("0", msg->arg))
            newValue = false;
        instance->settings.apEnabled = newValue;
        instance->saveSettings();
        instance->applySettings();
    }
    // get
    strncpy(msg->reply, instance->settings.apEnabled ? "1" : "0",
            sizeof(msg->reply));
    return Api::success();
}

ApiResult *Wifi::apSSIDProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    // set
    if (0 < strlen(msg->arg)) {
        strncpy(instance->settings.apSSID, msg->arg,
                sizeof(instance->settings.apSSID));
        instance->saveSettings();
        instance->applySettings();
    }
    // get
    strncpy(msg->reply, instance->settings.apSSID,
            sizeof(msg->reply));
    return Api::success();
}

ApiResult *Wifi::apPasswordProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    // set
    if (0 < strlen(msg->arg)) {
        strncpy(instance->settings.apPassword, msg->arg,
                sizeof(instance->settings.apPassword));
        instance->saveSettings();
        instance->applySettings();
    }
    // get
    strncpy(msg->reply, instance->settings.apPassword,
            sizeof(msg->reply));
    return Api::success();
}

ApiResult *Wifi::staProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    // set
    bool newValue = true;  // enable by default
    if (0 < strlen(msg->arg)) {
        if (0 == strcmp("false", msg->arg) ||
            0 == strcmp("0", msg->arg))
            newValue = false;
        instance->settings.staEnabled = newValue;
        instance->saveSettings();
        instance->applySettings();
    }
    // get
    strncpy(msg->reply, instance->settings.staEnabled ? "1" : "0",
            sizeof(msg->reply));
    return Api::success();
}

ApiResult *Wifi::staSSIDProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    // set
    if (0 < strlen(msg->arg)) {
        strncpy(instance->settings.staSSID, msg->arg,
                sizeof(instance->settings.staSSID));
        instance->saveSettings();
        instance->applySettings();
    }
    // get
    strncpy(msg->reply, instance->settings.staSSID,
            sizeof(msg->reply));
    return Api::success();
}

ApiResult *Wifi::staPasswordProcessor(ApiMessage *msg) {
    if (nullptr == instance) return Api::error();
    // set
    if (0 < strlen(msg->arg)) {
        strncpy(instance->settings.staPassword, msg->arg,
                sizeof(instance->settings.staPassword));
        instance->saveSettings();
        instance->applySettings();
    }
    // get
    strncpy(msg->reply, instance->settings.staPassword,
            sizeof(msg->reply));
    return Api::success();
}