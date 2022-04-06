#ifndef __atoll_fs_h
#define __atoll_fs_h

#include <Arduino.h>
#include "FS.h"

#include "atoll_serial.h"

namespace Atoll {

class Fs {
   public:
    bool mounted = false;

    virtual void setup() = 0;

    virtual fs::FS *pFs() = 0;

    virtual fs::FS &refFs() = 0;

    virtual void unmount() { log_i("not implemented"); }

    virtual int format() {
        log_i("not implemented");
        return -1;
    }
};

}  // namespace Atoll

#endif