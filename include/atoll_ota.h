#ifndef __atoll_ota_h
#define __atoll_ota_h

#include <ArduinoOTA.h>
#include <ESPmDNS.h>
#include <WiFi.h>
#include <WiFiUdp.h>

#include "atoll_task.h"

namespace Atoll {

class Ota : public Task {
   public:
    void setup();
    void setup(const char *hostName);
    void setup(const char *hostName, uint16_t port);
    void loop();
    void off();

    void onStart();
    void onEnd();
    void onProgress(uint progress, uint total);
    void onError(ota_error_t error);

   private:
    uint8_t lastProgressPercent = 0;
};

}  // namespace Atoll

#endif