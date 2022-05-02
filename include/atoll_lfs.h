/*

#ifndef __atoll_lfs_h
#define __atoll_lfs_h

#include <Arduino.h>
#include "FS.h"
#include <LITTLEFS.h>

#include "atoll_fs.h"

namespace Atoll {

class Lfs : public Fs {
   public:
    void setup() {
        if (!LITTLEFS.begin(true, "/littlefs", 1)) {
            LITTLEFS.end();  // maybe this is the first time and it is now formatted
            if (!LITTLEFS.begin(false, "/littlefs", 1)) {
                log_e("could not initialize littlefs");
                return;
            }
        }
        mounted = true;

        log_i("bytes total: %d, used: %d, free: %d",
              LITTLEFS.totalBytes(),
              LITTLEFS.usedBytes(),
              LITTLEFS.totalBytes() - LITTLEFS.usedBytes());
    }

    fs::FS *pFs() {
        return &LITTLEFS;
    }
};

}  // namespace Atoll

#endif

*/