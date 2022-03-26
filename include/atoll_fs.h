#ifndef __atoll_fs_h
#define __atoll_fs_h

#include <Arduino.h>

namespace Atoll {

class Fs {
   public:
    bool mounted = false;

    virtual void setup() = 0;
};
}  // namespace Atoll

#endif