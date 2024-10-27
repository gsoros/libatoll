#ifdef FEATURE_WIFI

#include "atoll_wifi.h"
#include "atoll_serial.h"

using namespace Atoll;

Wifi *Wifi::instance;
Ota *Wifi::ota;

void Wifi::setup(
    const char *hostName,
    ::Preferences *p,
    const char *preferencesNS,
    Wifi *instance,
    Api *api,
    Ota *ota) {
    strncpy(this->hostName, hostName, sizeof(this->hostName));
    preferencesSetup(p, preferencesNS);
    this->instance = instance;
    this->ota = ota;

    taskSetFreq(WIFI_TASK_FREQ);
    loadDefaultSettings();
    loadSettings();
    registerCallbacks();

    if (nullptr == instance) return;
    if (nullptr == api) return;
    api->addCommand(Api::Command("w", enabledProcessor));
    api->addCommand(Api::Command("wa", apProcessor));
    api->addCommand(Api::Command("was", apSSIDProcessor));
    api->addCommand(Api::Command("wap", apPasswordProcessor));
    api->addCommand(Api::Command("ws", staProcessor));
    api->addCommand(Api::Command("wss", staSSIDProcessor));
    api->addCommand(Api::Command("wsp", staPasswordProcessor));
    api->addCommand(Api::Command("wst", staStaticProcessor));
};

void Wifi::loop() {
    if (settings.enabled &&
        settings.staEnabled &&
        !WiFi.isConnected() &&
        !staConnecting &&
        lastDisconnect + reconnectDelaySecs * 1000 < millis())
        startSta();
};

void Wifi::start() {
    applySettings();
    taskStart();
};

void Wifi::stop() {
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
    settings.staticIp.fromString(preferences->getString("staticIp"));
    settings.staticGateway.fromString(preferences->getString("staticGw"));
    settings.staticSubnet.fromString(preferences->getString("staticSn"));
    settings.staticDns0.fromString(preferences->getString("staticD0"));
    settings.staticDns1.fromString(preferences->getString("staticD1"));
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
    preferences->putString("staticIp", settings.staticIp.toString());
    preferences->putString("staticGw", settings.staticGateway.toString());
    preferences->putString("staticSn", settings.staticSubnet.toString());
    preferences->putString("staticD0", settings.staticDns0.toString());
    preferences->putString("staticD1", settings.staticDns1.toString());
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
    log_i("STA %sabled ap:'%s', pw: '%s'",
          settings.staEnabled ? "En" : "Dis",
          settings.staSSID,
          "***"  // settings.staPassword
    );
    log_i("STA %sconnected, IP: %s GW: %s SN: %s DNS0: %s DNS1: %s",
          WiFi.isConnected() ? "" : "NOT ",
          WiFi.localIP().toString().c_str(),
          WiFi.gatewayIP().toString().c_str(),
          WiFi.subnetMask().toString().c_str(),
          WiFi.dnsIP(0).toString().c_str(),
          WiFi.dnsIP(1).toString().c_str());
};

void Wifi::applySettings() {
    log_i("applying settings, wifi: %sabled ap: %sabled sta: %sabled",
          settings.enabled ? "en" : "dis",
          settings.apEnabled ? "en" : "dis",
          settings.staEnabled ? "en" : "dis");
#ifdef FEATURE_SERIAL
    Serial.flush();
#endif
    // delay(1000);
    wifi_mode_t mode = WIFI_MODE_NULL;
    WiFi.mode(mode);
    if (settings.enabled && (settings.apEnabled || settings.staEnabled)) {
        if (settings.apEnabled && settings.staEnabled)
            mode = WIFI_MODE_APSTA;
        else if (settings.apEnabled)
            mode = WIFI_MODE_AP;
        else
            mode = WIFI_MODE_STA;
    }
    WiFi.persistent(true);
    WiFi.setAutoReconnect(true);
    reconnectDelaySecs = 0;
    WiFi.mode(mode);

    if (settings.enabled && settings.apEnabled) {
        if (0 == strlen(settings.apSSID)) {
            log_w("cannot enable AP with empty SSID");
            // settings.apEnabled = false;
        } else {
            startAp();
        }
    }
    if (settings.enabled && settings.staEnabled) {
        if (0 == strlen(settings.staSSID)) {
            log_w("cannot enable STA with empty SSID");
            // settings.staEnabled = false;
        } else {
            startSta();
        }
    }
};

void Wifi::startAp() {
    log_i("setting up AP '%s' '%s'", settings.apSSID, settings.apPassword);
    WiFi.softAP(settings.apSSID, settings.apPassword);
};

void Wifi::startSta() {
    if (staConnecting) return;
    if (ipUnset != settings.staticIp &&
        ipUnset != settings.staticGateway &&
        ipUnset != settings.staticSubnet) {
        if (!WiFi.config(
                settings.staticIp,
                settings.staticGateway,
                settings.staticSubnet,
                settings.staticDns0,
                settings.staticDns1)) {
            log_e("config failed");
        }
    } else if (!WiFi.config(ipUnset, ipUnset, ipUnset)) {
        log_e("blank config failed");
    }

    log_i("connecting to AP '%s'", settings.staSSID);
    staConnecting = true;
    WiFi.begin(settings.staSSID, settings.staPassword);
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

void Wifi::apLogStations() {
    wifi_sta_list_t wifi_sta_list;
    memset(&wifi_sta_list, 0, sizeof(wifi_sta_list));
    tcpip_adapter_sta_list_t adapter_sta_list;
    memset(&adapter_sta_list, 0, sizeof(adapter_sta_list));
    esp_wifi_ap_get_sta_list(&wifi_sta_list);
    tcpip_adapter_get_sta_list(&wifi_sta_list, &adapter_sta_list);
    char ip[16] = "";
    for (int i = 0; i < adapter_sta_list.num; i++) {
        tcpip_adapter_sta_info_t station = adapter_sta_list.sta[i];
        log_i("AP sta #%d: %d.%d.%d.%d",
              i,
              ip4_addr1(&(station.ip)),
              ip4_addr2(&(station.ip)),
              ip4_addr3(&(station.ip)),
              ip4_addr4(&(station.ip)));
    }
}

void Wifi::onEvent(arduino_event_id_t event, arduino_event_info_t info) {
    switch (event) {
        case ARDUINO_EVENT_WIFI_READY:
            log_i("ARDUINO_EVENT_WIFI_READY");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            log_i("ARDUINO_EVENT_WIFI_SCAN_DONE");
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            log_i("ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            log_i("ARDUINO_EVENT_WIFI_STA_GOT_IP6");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            log_i("ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            log_i("ARDUINO_EVENT_WIFI_AP_GOT_IP6");
            break;
        case ARDUINO_EVENT_WIFI_FTM_REPORT:
            log_i("ARDUINO_EVENT_WIFI_FTM_REPORT");
            break;
        case ARDUINO_EVENT_ETH_START:
            log_i("ARDUINO_EVENT_ETH_START");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            log_i("ARDUINO_EVENT_ETH_STOP");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            log_i("ARDUINO_EVENT_ETH_CONNECTED");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            log_i("ARDUINO_EVENT_ETH_DISCONNECTED");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            log_i("ARDUINO_EVENT_ETH_GOT_IP");
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            log_i("ARDUINO_EVENT_WPS_ER_SUCCESS");
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            log_i("ARDUINO_EVENT_WPS_ER_FAILED");
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            log_i("ARDUINO_EVENT_WPS_ER_TIMEOUT");
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            log_i("ARDUINO_EVENT_WPS_ER_PIN");
            break;
        case ARDUINO_EVENT_WPS_ER_PBC_OVERLAP:
            log_i("ARDUINO_EVENT_WPS_ER_PBC_OVERLAP");
            break;
        case ARDUINO_EVENT_SC_SCAN_DONE:
            log_i("ARDUINO_EVENT_SC_SCAN_DONE");
            break;
        case ARDUINO_EVENT_SC_FOUND_CHANNEL:
            log_i("ARDUINO_EVENT_SC_FOUND_CHANNEL");
            break;
        case ARDUINO_EVENT_SC_GOT_SSID_PSWD:
            log_i("ARDUINO_EVENT_SC_GOT_SSID_PSWD");
            break;
        case ARDUINO_EVENT_SC_SEND_ACK_DONE:
            log_i("ARDUINO_EVENT_SC_SEND_ACK_DONE");
            break;
        case ARDUINO_EVENT_PROV_INIT:
            log_i("ARDUINO_EVENT_PROV_INIT");
            break;
        case ARDUINO_EVENT_PROV_DEINIT:
            log_i("ARDUINO_EVENT_PROV_DEINIT");
            break;
        case ARDUINO_EVENT_PROV_START:
            log_i("ARDUINO_EVENT_PROV_START");
            break;
        case ARDUINO_EVENT_PROV_END:
            log_i("ARDUINO_EVENT_PROV_END");
            break;
        case ARDUINO_EVENT_PROV_CRED_RECV:
            log_i("ARDUINO_EVENT_PROV_CRED_RECV");
            break;
        case ARDUINO_EVENT_PROV_CRED_FAIL:
            log_i("ARDUINO_EVENT_PROV_CRED_FAIL");
            break;
        case ARDUINO_EVENT_PROV_CRED_SUCCESS:
            log_i("ARDUINO_EVENT_PROV_CRED_SUCCESS");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            log_i("[AP] started, ip: %s", WiFi.softAPIP().toString().c_str());
            if (ipUnset != settings.staticIp && WiFi.softAPIP() != settings.staticIp) {
                if (settings.staEnabled) {
                    log_i("[AP] not setting static ip, already used by STA");
                    return;
                }
                log_i("[AP] setting ip %s/24", settings.staticIp.toString().c_str());
                if (!WiFi.softAPConfig(
                        settings.staticIp,
                        settings.staticIp,
                        IPAddress(255, 255, 255, 0))) {
                    log_e("error setting ip");
                }
            }
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            log_i("[AP] stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            log_i("[AP] station connected, now active: %d", WiFi.softAPgetStationNum());
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            log_i("[AP] station got IP");
            apLogStations();
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            log_i("[AP] station disconnected, now active: %d", WiFi.softAPgetStationNum());
            apLogStations();
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            log_i("[STA] starting");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            log_i("[STA] connected");
            reconnectDelaySecs = 0;
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            log_i("[STA] got ip: %s, gw: %s, sn: %s, d0: %s, d1: %s",
                  WiFi.localIP().toString().c_str(),
                  WiFi.gatewayIP().toString().c_str(),
                  WiFi.subnetMask().toString().c_str(),
                  WiFi.dnsIP(0).toString().c_str(),
                  WiFi.dnsIP(1).toString().c_str());
            if (ipUnset != settings.staticIp &&
                WiFi.localIP() != settings.staticIp) {
                if (settings.apEnabled && WiFi.softAPIP() == settings.staticIp) {
                    log_i("[STA] not setting static ip already used by AP");
                    return;
                }
                IPAddress gw = ipUnset != settings.staticGateway ? settings.staticGateway : WiFi.gatewayIP();
                IPAddress sn = ipUnset != settings.staticSubnet ? settings.staticSubnet : WiFi.subnetMask();
                if (ipUnset == gw || ipUnset == sn) {
                    log_i("[STA] not setting static, gw or sn invalid");
                    return;
                }
                IPAddress d0 = ipUnset != settings.staticDns0 ? settings.staticDns0 : WiFi.dnsIP(0);
                IPAddress d1 = ipUnset != settings.staticDns1 ? settings.staticDns1 : WiFi.dnsIP(1);
                log_i("[STA] reconnecting with static settings, ip: %s, gw: %s, sn: %s, d0: %s, d1: %s",
                      settings.staticIp.toString().c_str(),
                      gw.toString().c_str(),
                      sn.toString().c_str(),
                      d0.toString().c_str(),
                      d1.toString().c_str());
                if (!WiFi.config(settings.staticIp, gw, sn, d0, d1)) {
                    log_e("config failed");
                    return;
                }
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            log_i("[STA] lost ip");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            log_i("[STA] disconnected");
            WiFi.disconnect();
            staConnecting = false;
            lastDisconnect = millis();
            if (settings.enabled && settings.staEnabled) {
                reconnectDelaySecs += reconnectStep;
                if (reconnectDelaySecs < reconnectMin)
                    reconnectDelaySecs = reconnectMin;
                else if (reconnectMax < reconnectDelaySecs)
                    reconnectDelaySecs = reconnectMax;
                log_i("reconnecting in %d secs", reconnectDelaySecs);
                // log_i("[STA] reconnecting");
                //  WiFi.disconnect();
                // WiFi.reconnect();
                //  WiFi.begin();
            }
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            log_i("[STA] stopped");
            break;
        default:
            log_i("event: %d, info: %d", event, info);
    }
    /*
        static bool prevConnected = false;
        bool connected = isConnected();

        if (prevConnected != connected) {
            if (connected) {
    #ifdef FEATURE_SERIAL
    #endif
            } else {
    #ifdef FEATURE_SERIAL
    #endif
            }
        }
        prevConnected = connected;
    */
}

Api::Result *Wifi::enabledProcessor(Api::Message *msg) {
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

Api::Result *Wifi::apProcessor(Api::Message *msg) {
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

Api::Result *Wifi::apSSIDProcessor(Api::Message *msg) {
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

Api::Result *Wifi::apPasswordProcessor(Api::Message *msg) {
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

Api::Result *Wifi::staProcessor(Api::Message *msg) {
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

Api::Result *Wifi::staSSIDProcessor(Api::Message *msg) {
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

Api::Result *Wifi::staPasswordProcessor(Api::Message *msg) {
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

Api::Result *Wifi::staStaticProcessor(Api::Message *msg) {
    if (nullptr == instance) return Api::error();
    const IPAddress ipUnset = instance->ipUnset;
    Settings *s = &instance->settings;
    size_t len = strlen(msg->arg);
    char *stop = msg->arg + len;
    // set
    if (0 < len) {
        char ipbuf[3 * 4 + 3 + 1] = "";  // uint8 * 4 + 3 periods + nul
        // char buf[5 * (sizeof(ipbuf) - 1) + 4 + 1] = "";  // 5 ips + 4 commas + nul
        uint8_t ipMinLen = 7;
        IPAddress ip = ipUnset;
        char *cur = msg->arg;
        char *end = strchr(cur, ',');
        if (nullptr == end) end = cur + len;
        if (ipMinLen <= end - cur) {
            strncpy(ipbuf, cur, end - cur);
            ipbuf[end - cur] = '\0';
            ip.fromString(String(ipbuf));
            log_d("staticIp ipbuf: '%s', ip: '%s'", ipbuf, ip.toString().c_str());
            s->staticIp = ip;
        }
        cur = end + 1;
        if (stop <= cur) cur -= 1;
        end = strchr(cur, ',');
        if (nullptr == end) end = msg->arg + len;
        if (ipMinLen <= end - cur) {
            strncpy(ipbuf, cur, end - cur);
            ipbuf[end - cur] = '\0';
            ip.fromString(String(ipbuf));
            log_d("staticGateway ipbuf: '%s', ip: '%s'", ipbuf, ip.toString().c_str());
            s->staticGateway = ip;
        }
        cur = end + 1;
        if (stop <= cur) cur -= 1;
        end = strchr(cur, ',');
        if (nullptr == end) end = msg->arg + len;
        if (ipMinLen <= end - cur) {
            strncpy(ipbuf, cur, end - cur);
            ipbuf[end - cur] = '\0';
            ip.fromString(String(ipbuf));
            log_d("staticSubnet ipbuf: '%s', ip: '%s'", ipbuf, ip.toString().c_str());
            s->staticSubnet = ip;
        }
        cur = end + 1;
        if (stop <= cur) cur -= 1;
        end = strchr(cur, ',');
        if (nullptr == end) end = msg->arg + len;
        if (ipMinLen <= end - cur) {
            strncpy(ipbuf, cur, end - cur);
            ipbuf[end - cur] = '\0';
            ip.fromString(String(ipbuf));
            log_d("staticDns0 ipbuf: '%s', ip: '%s'", ipbuf, ip.toString().c_str());
            s->staticDns0 = ip;
        }
        cur = end + 1;
        if (stop <= cur) cur -= 1;
        end = strchr(cur, ',');
        if (nullptr == end) end = msg->arg + len;
        if (ipMinLen <= end - cur) {
            strncpy(ipbuf, cur, end - cur);
            ipbuf[end - cur] = '\0';
            ip.fromString(String(ipbuf));
            log_d("staticDns1 ipbuf: '%s', ip: '%s'", ipbuf, ip.toString().c_str());
            s->staticDns1 = ip;
        }
        instance->saveSettings();
        instance->applySettings();
    }
    // get
    if (ipUnset != s->staticIp) {
        msg->replyAppend("IP,GW,SN,D0,D1: ");
        msg->replyAppend(s->staticIp.toString().c_str());
        if (ipUnset != s->staticGateway) {
            msg->replyAppend(",");
            msg->replyAppend(s->staticGateway.toString().c_str());
            if (ipUnset != s->staticSubnet) {
                msg->replyAppend(",");
                msg->replyAppend(s->staticSubnet.toString().c_str());
                if (ipUnset != s->staticDns0) {
                    msg->replyAppend(",");
                    msg->replyAppend(s->staticDns0.toString().c_str());
                    if (ipUnset != s->staticDns1) {
                        msg->replyAppend(",");
                        msg->replyAppend(s->staticDns1.toString().c_str());
                    }
                }
            }
        }
    } else {
        msg->replyAppend("off");
    }
    return Api::success();
}

#endif