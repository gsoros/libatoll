#ifndef __atoll_ota_h
#define __atoll_ota_h

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "atoll_task.h"
#include "atoll_recorder.h"

#ifndef ATOLL_OTA_TASK_FREQ_WHEN_UPLOADING
#define ATOLL_OTA_TASK_FREQ_WHEN_UPLOADING 20
#endif

namespace Atoll {

class Ota : public Task {
   public:
    char taskName[TASK_NAME_LEN] = "Ota";
    Recorder *recorder = nullptr;

    void setup();
    void setup(const char *hostName, Recorder *recorder = nullptr);
    void setup(const char *hostName, uint16_t port, Recorder *recorder = nullptr);
    void loop();
    void off();

    void onStart();
    void onEnd();
    void onProgress(uint progress, uint total);
    void onError(ota_error_t error);

   private:
    uint16_t savedTaskFreq = 10;
};

}  // namespace Atoll

#endif