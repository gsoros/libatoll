#include "atoll_ota.h"

using namespace Atoll;

void Ota::setup() {
    setup("libAtollOta_unnamed");
}
void Ota::setup(const char *hostName, Recorder *recorder) {
    setup(hostName, 3232, recorder);
}
void Ota::setup(const char *hostName, uint16_t port, Recorder *recorder) {
    log_i("[OTA] Setup hostname: %s port: %d", hostName, port);
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        log_i("[OTA] Wifi is disabled, not starting");
        return;
    }
    ArduinoOTA.setHostname(hostName);  // Hostname defaults to esp3232-[MAC]
    ArduinoOTA.setPort(port);          // Port defaults to 3232
    ArduinoOTA.setTimeout(5000);       // for choppy WiFi
    ArduinoOTA.setMdnsEnabled(true);

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
    ArduinoOTA.begin();
    this->recorder = recorder;
}

void Ota::loop() {
    if (WiFi.getMode() == WIFI_MODE_NULL) {
        log_i("[OTA] Wifi is disabled, task should be stopped");
        return;
    }
    ArduinoOTA.handle();
}

void Ota::off() {
    log_i("[OTA] Shutting down");
    ArduinoOTA.end();
    taskStop();
}

void Ota::onStart() {
    log_i("[OTA] Update start");
    savedTaskFreq = taskFreq;
    taskSetFreq(ATOLL_OTA_TASK_FREQ_WHEN_UPLOADING);

    // log_i("[OTA] Disabling sleep");
    //  board.sleepEnabled = false;

    if (ArduinoOTA.getCommand() == U_FLASH)
        log_i("[OTA] Flash");
    else {  // U_SPIFFS
        log_i("[OTA] FS");
    }
    if (nullptr != recorder) {
        log_i("stopping recorder");
        recorder->stop();
    }
}

void Ota::onEnd() {
    // log_i("[OTA] Enabling sleep");
    // board.sleepEnabled = true;

    taskSetFreq(savedTaskFreq);

    log_i("[OTA] End");
}

void Ota::onProgress(uint progress, uint total) {
    static uint8_t lastPercent = 0;
    uint8_t percent = (uint8_t)((float)progress / (float)total * 100.0);
    if (percent > lastPercent) {
        log_i("[OTA] %d%%", percent);
        lastPercent = percent;
    }
}

void Ota::onError(ota_error_t error) {
    log_i("[OTA] Error %u", error);
    if (error == OTA_AUTH_ERROR)
        log_i("^^^ = Auth Failed");
    else if (error == OTA_BEGIN_ERROR)
        log_i("^^^ = Begin Failed");
    else if (error == OTA_CONNECT_ERROR)
        log_i("^^^ = Connect Failed");
    else if (error == OTA_RECEIVE_ERROR)
        log_i("^^^ = Receive Failed");
    else if (error == OTA_END_ERROR)
        log_i("^^^ = End Failed");
    // log_i("[OTA] Enabling sleep");
    //  board.sleepEnabled = true;
}
