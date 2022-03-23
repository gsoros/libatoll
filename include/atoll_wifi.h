#ifndef __atoll_wifi_h
#define __atoll_wifi_h

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "atoll_preferences.h"
#include "atoll_task.h"
#include "atoll_api.h"
#include "atoll_ota.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

namespace Atoll {

class Wifi : public Preferences, public Task {
   public:
    typedef struct
    {
        bool enabled;
        bool apEnabled;
        char apSSID[SETTINGS_STR_LENGTH];
        char apPassword[SETTINGS_STR_LENGTH];
        bool staEnabled;
        char staSSID[SETTINGS_STR_LENGTH];
        char staPassword[SETTINGS_STR_LENGTH];
    } Settings;

    char hostName[SETTINGS_STR_LENGTH] = "libAtollWifi_unnamed";
    Settings settings;

    static Wifi *instance;
    static Ota *ota;

    void setup(
        const char *hostName,
        ::Preferences *p,
        const char *preferencesNS,
        Wifi *instance = nullptr,
        Api *api = nullptr,
        Ota *ota = nullptr);

    void loop();
    void off();
    void loadSettings();
    void loadDefaultSettings();
    void saveSettings();
    void printSettings();
    void printAPSettings();
    void printSTASettings();
    void applySettings();
    void setEnabled(bool state);
    bool isEnabled();
    bool connected();
    void registerCallbacks();

    static ApiResult *enabledProcessor(ApiReply *reply);
    static ApiResult *apProcessor(ApiReply *reply);
    static ApiResult *apSSIDProcessor(ApiReply *reply);
    static ApiResult *apPasswordProcessor(ApiReply *reply);
    static ApiResult *staProcessor(ApiReply *reply);
    static ApiResult *staSSIDProcessor(ApiReply *reply);
    static ApiResult *staPasswordProcessor(ApiReply *reply);
};

}  // namespace Atoll
#endif