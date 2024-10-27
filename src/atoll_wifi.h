#if !defined(__atoll_wifi_h) && defined(FEATURE_WIFI)
#define __atoll_wifi_h

#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>

#include "esp_wifi.h"  // for apLogStations

#include "atoll_preferences.h"
#include "atoll_task.h"
#include "atoll_api.h"
#include "atoll_ota.h"

#ifndef SETTINGS_STR_LENGTH
#define SETTINGS_STR_LENGTH 32
#endif

#ifndef WIFI_TASK_FREQ
#define WIFI_TASK_FREQ (1.0f / 3.0f)  // run every 3 secs
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
        IPAddress staticIp = IPAddress(0, 0, 0, 0);
        IPAddress staticSubnet = IPAddress(0, 0, 0, 0);
        IPAddress staticGateway = IPAddress(0, 0, 0, 0);
        IPAddress staticDns0 = IPAddress(0, 0, 0, 0);
        IPAddress staticDns1 = IPAddress(0, 0, 0, 0);
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
    virtual void startAp();
    virtual void startSta();
    virtual void setEnabled(bool state, bool save = true);
    virtual bool isEnabled();
    virtual bool isConnected();
    virtual void registerCallbacks();
    virtual void apLogStations();

    virtual void onEvent(arduino_event_id_t event, arduino_event_info_t info);

    static Api::Result *enabledProcessor(Api::Message *reply);
    static Api::Result *apProcessor(Api::Message *reply);
    static Api::Result *apSSIDProcessor(Api::Message *reply);
    static Api::Result *apPasswordProcessor(Api::Message *reply);
    static Api::Result *staProcessor(Api::Message *reply);
    static Api::Result *staSSIDProcessor(Api::Message *reply);
    static Api::Result *staPasswordProcessor(Api::Message *reply);
    static Api::Result *staStaticProcessor(Api::Message *reply);

   protected:
    const IPAddress ipUnset = IPAddress(0, 0, 0, 0);
    bool staConnecting = false;
    uint16_t reconnectDelaySecs = 0;
    const uint16_t reconnectMin = 5;
    const uint16_t reconnectMax = 300;
    const uint16_t reconnectStep = 5;
    ulong lastDisconnect = 0;
};

}  // namespace Atoll
#endif