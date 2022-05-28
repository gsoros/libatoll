//#include <ESPmDNS.h>
#include <WiFi.h>
//#include <WiFiUdp.h>

#include "atoll_ota.h"

using namespace Atoll;

void Ota::setup() {
    setup("libAtollOta_unnamed");
}
void Ota::setup(const char *hostName, Recorder *recorder) {
    setup(hostName, 3232, recorder);
}
void Ota::setup(const char *hostName, uint16_t port, Recorder *recorder) {
    if (serving) {
        log_i("already serving");
        return;
    }
    log_i("%s:%d", hostName, port);
    ArduinoOTA.setHostname(hostName);  // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setPort(port);          // Port defaults to 3232
    ArduinoOTA.setTimeout(5000);       // for choppy WiFi
    ArduinoOTA.setMdnsEnabled(false);

    // ArduinoOTA.setPassword("admin");// No authentication by default
    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA
        .onStart([this]() {
            onStart();
        })
        .onEnd([this]() {
            onEnd();
        })
        .onProgress([this](uint progress, uint total) {
            onProgress(progress, total);
        })
        .onError([this](ota_error_t error) {
            onError(error);
        });
    // start();
    serving = true;
    this->recorder = recorder;
}

void Ota::start() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        log_i("Wifi is disabled, not starting");
        return;
    }
    ArduinoOTA.begin();
}

void Ota::loop() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        log_i("Wifi is disabled, task should be stopped");
        return;
    }
    ArduinoOTA.handle();
}

void Ota::stop() {
    if (!serving) {
        log_i("not serving");
        return;
    }
    log_i("Shutting down");
    ArduinoOTA.end();
    serving = false;
    taskStop();
}

void Ota::onStart() {
    log_i("Update start");

    if (ArduinoOTA.getCommand() == U_FLASH)
        log_i("Flash");
    else {  // U_SPIFFS
        log_i("FS");
    }

    // log_i("setting cpu freq to 240 MHz");
    // setCpuFrequencyMhz(240);

    log_i("setting task freq to %d Hz", ATOLL_OTA_TASK_FREQ_WHEN_UPLOADING);
    savedTaskFreq = _taskFreq;
    taskSetFreq(ATOLL_OTA_TASK_FREQ_WHEN_UPLOADING);

    // log_i("Disabling sleep");
    //  board.sleepEnabled = false;

    if (nullptr != recorder) {
        log_i("pausing recorder");
        recorder->pause();
    }
}

void Ota::onEnd() {
    // log_i("Enabling sleep");
    // board.sleepEnabled = true;

    taskSetFreq(savedTaskFreq);
    log_i("end");
    if (100 == lastPercent) {
        log_i("rebooting");
        ESP.restart();
    }
}

void Ota::onProgress(uint progress, uint total) {
    uint8_t percent = (uint8_t)((float)progress / (float)total * 100.0);
    if (percent != lastPercent) {
        log_i("%d%%", percent);
        lastPercent = percent;
    }
}

void Ota::onError(ota_error_t error) {
    if (error == OTA_AUTH_ERROR)
        log_e("Auth");
    else if (error == OTA_BEGIN_ERROR)
        log_e("Begin");
    else if (error == OTA_CONNECT_ERROR)
        log_e("Connect");
    else if (error == OTA_RECEIVE_ERROR)
        log_e("Receive");
    else if (error == OTA_END_ERROR)
        log_e("End");
    else
        log_e("%d", error);
}
