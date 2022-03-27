#ifndef __atoll_uploader_h
#define __atoll_uploader_h

#include <Arduino.h>
#include <WiFiClientSecure.h>

#include "atoll_task.h"
#include "atoll_recorder.h"
#include "atoll_fs.h"
#include "atoll_wifi.h"

#ifndef ATOLL_UPLOADER_URL
#define ATOLL_UPLOADER_URL "https://www.strava.com/api/v3/uploads"
#endif

namespace Atoll {

class Uploader : public Task {
   public:
    const char *taskName() { return "Uploader"; }
    Recorder *recorder = nullptr;  //
    FS *fs = nullptr;              //
    Wifi *wifi = nullptr;          //

    void setup(Recorder *recorder, Fs *fs, Wifi *wifi) {
        this->recorder = recorder;
        if (!fs->mounted)
            log_e("fs not mounted");
        this->fs = fs->fsp();
        this->recorder = recorder;
    }

    void loop() {}
};

}  // namespace Atoll

#endif