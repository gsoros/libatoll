#ifndef __atoll_uploader_h
#define __atoll_uploader_h

#include <Arduino.h>
#include <WiFiClientSecure.h>

#include "atoll_task.h"
#include "atoll_recorder.h"
#include "atoll_fs.h"
#include "atoll_wifi.h"

#ifndef ATOLL_UPLOADER_STATUS_EXT
#define ATOLL_UPLOADER_STATUS_EXT ".upl"
#endif

#ifndef ATOLL_UPLOADER_URL
#define ATOLL_UPLOADER_URL "https://hostname.local/uploads"
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

    void loop() {
        return;  // TODO
        File dir = fs->open(recorder->basePath);
        if (!dir) {
            log_e("could not open %s", recorder->basePath);
            dir.close();
            return;
        }
        if (!dir.isDirectory()) {
            log_e("%s is not a directory", recorder->basePath);
            dir.close();
            return;
        }
        File file;
        char filePath[ATOLL_RECORDER_PATH_LENGTH] = "";
        char gpxPath[ATOLL_RECORDER_PATH_LENGTH] = "";
        const char *currentRecPath = recorder->currentPath();
        while (file = dir.openNextFile()) {
            if (strlen(file.name()) - strlen(recorder->basePath) - 1 != 8) {
                // log_i("%s is not a recording, skipping", file.name());
                file.close();
                continue;
            }
            if (currentRecPath && 0 == strcmp(currentRecPath, file.name())) {
                // log_i("currently recording %s, skipping", file.name());
                file.close();
                continue;
            }
            strncpy(filePath, file.name(), sizeof(filePath));
            file.close();
            snprintf(gpxPath, sizeof(gpxPath), "%s.gpx", filePath);
            rec2gpx(filePath, gpxPath);
        }
        dir.close();
    }

    void rec2gpx(const char *in, const char *out) {
        log_i("TODO generate %s", out);
    }
};

}  // namespace Atoll

#endif