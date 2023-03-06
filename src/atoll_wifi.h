#if !defined(__atoll_wifi_h) && defined(FEATURE_WIFI)
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
        bool enabled = false;
        bool apEnabled = false;
        char apSSID[SETTINGS_STR_LENGTH] = "";
        char apPassword[SETTINGS_STR_LENGTH] = "";
        bool staEnabled = false;
        char staSSID[SETTINGS_STR_LENGTH] = "";
        char staPassword[SETTINGS_STR_LENGTH] = "";
    } Settings;

    const char *taskName() { return "Wifi"; }
    char hostName[SETTINGS_STR_LENGTH] = "libAtollWifi_unnamed";
    Settings settings;

    static Wifi *instance;
    static Ota *ota;

    virtual void setup(
        const char *hostName,
        ::Preferences *p,
        const char *preferencesNS,
        Wifi *instance = nullptr,
        Api *api = nullptr,
        Ota *ota = nullptr);

    virtual void loop();
    virtual void start();
    virtual void stop();
    virtual void loadSettings();
    virtual void loadDefaultSettings();
    virtual void saveSettings();
    virtual void printSettings();
    virtual void printAPSettings();
    virtual void printSTASettings();
    virtual void applySettings();
    virtual void setEnabled(bool state, bool save = true);
    virtual bool isEnabled();
    virtual bool isConnected();
    virtual void registerCallbacks();

    virtual void onEvent(arduino_event_id_t event, arduino_event_info_t info);

    static ApiResult *enabledProcessor(ApiMessage *reply);
    static ApiResult *apProcessor(ApiMessage *reply);
    static ApiResult *apSSIDProcessor(ApiMessage *reply);
    static ApiResult *apPasswordProcessor(ApiMessage *reply);
    static ApiResult *staProcessor(ApiMessage *reply);
    static ApiResult *staSSIDProcessor(ApiMessage *reply);
    static ApiResult *staPasswordProcessor(ApiMessage *reply);
};

}  // namespace Atoll
#endif