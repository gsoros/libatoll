#include "atoll_wifi.h"

using namespace Atoll;

void Wifi::setup(
    const char *hostName,
    ::Preferences *p,
    const char *preferencesNS) {
    strncpy(this->hostName, hostName, sizeof(this->hostName));
    preferencesSetup(p, preferencesNS);
    loadDefaultSettings();
    loadSettings();
    applySettings();
    registerCallbacks();
};

void Wifi::loop(){};

void Wifi::off() {
    log_i("[Wifi] Shutting down");
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
    log_i("[Wifi] Wifi %sabled",
          settings.enabled ? "En" : "Dis");
    printAPSettings();
    printSTASettings();
};

void Wifi::printAPSettings() {
    log_i("[Wifi] AP %sabled '%s' '%s'",
          settings.apEnabled ? "En" : "Dis",
          settings.apSSID,
          "***"  // settings.apPassword
    );
    if (settings.apEnabled)
        log_i("[Wifi] AP online, IP: %s", WiFi.softAPIP().toString().c_str());
};

void Wifi::printSTASettings() {
    log_i("[Wifi] STA %sabled '%s' '%s'",
          settings.staEnabled ? "En" : "Dis",
          settings.staSSID,
          "***"  // settings.staPassword
    );
    if (WiFi.isConnected())
        log_i("[Wifi] STA connected, local IP: %s", WiFi.localIP().toString().c_str());
};

void Wifi::applySettings() {
    log_i("[Wifi] Applying settings, connections will be reset");
    Serial.flush();
    delay(1000);
    wifi_mode_t newWifiMode;
    if (settings.enabled && (settings.apEnabled || settings.staEnabled)) {
        if (settings.apEnabled && settings.staEnabled)
            newWifiMode = WIFI_MODE_APSTA;
        else if (settings.apEnabled)
            newWifiMode = WIFI_MODE_AP;
        else
            newWifiMode = WIFI_MODE_STA;
    } else
        newWifiMode = WIFI_MODE_NULL;
    WiFi.mode(newWifiMode);
    if (settings.enabled && settings.apEnabled) {
        if (0 == strcmp("", const_cast<char *>(settings.apSSID))) {
            log_e("Warning: cannot enable AP with empty SSID");
            settings.apEnabled = false;
        } else {
            log_i("Setting up WiFi AP '%s'", settings.apSSID);
            WiFi.softAP(settings.apSSID, settings.apPassword);
        }
    }
    if (settings.enabled && settings.staEnabled) {
        if (0 == strcmp("", const_cast<char *>(settings.staSSID))) {
            log_e("Warning: cannot enable STA with empty SSID");
            settings.staEnabled = false;
        } else {
            log_i("Connecting WiFi STA to AP '%s'", settings.staSSID);
            WiFi.begin(settings.staSSID, settings.staPassword);
        }
    }
};

void Wifi::setEnabled(bool state) {
    settings.enabled = state;
    applySettings();
    saveSettings();
};

bool Wifi::isEnabled() {
    return settings.enabled;
};

bool Wifi::connected() {
    return WiFi.isConnected() || WiFi.softAPgetStationNum() > 0;
};

void Wifi::registerCallbacks() {
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        log_i("WiFi AP new connection, now active: %d", WiFi.softAPgetStationNum());
    },
                 SYSTEM_EVENT_AP_STACONNECTED);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        log_i("WiFi AP station disconnected, now active: %d", WiFi.softAPgetStationNum());
    },
                 SYSTEM_EVENT_AP_STADISCONNECTED);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        log_i("WiFi STA connected, IP: %s", WiFi.localIP().toString().c_str());
    },
                 SYSTEM_EVENT_STA_GOT_IP);
    WiFi.onEvent([](WiFiEvent_t event, WiFiEventInfo_t info) {
        log_i("WiFi STA disconnected");
    },
                 SYSTEM_EVENT_STA_LOST_IP);
}
