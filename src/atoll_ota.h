#if !defined(__atoll_ota_h) && defined(FEATURE_OTA)
#define __atoll_ota_h

#include <ArduinoOTA.h>

#include "atoll_task.h"
#include "atoll_log.h"

#ifndef ATOLL_OTA_TASK_FREQ
#define ATOLL_OTA_TASK_FREQ 1
#endif

#ifndef ATOLL_OTA_TASK_FREQ_WHEN_UPLOADING
#define ATOLL_OTA_TASK_FREQ_WHEN_UPLOADING 5
#endif

namespace Atoll {

class Ota : public Task {
   public:
    const char *taskName() { return "Ota"; }
    bool serving = false;

    void setup();
    void setup(const char *hostName, uint16_t port = 3232, bool mdnsEnabled = false);
    void start();
    void loop();
    void stop();

    virtual void onStart();
    virtual void onEnd();
    virtual void onProgress(uint progress, uint total);
    virtual void onError(ota_error_t error);

   protected:
    uint16_t savedTaskFreq = ATOLL_OTA_TASK_FREQ;
    uint8_t lastPercent = 0;
};

}  // namespace Atoll

#endif