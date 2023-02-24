#ifndef __atoll_sdcard_h
#define __atoll_sdcard_h

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// #include "vfs_fat_internal.h"

#include "atoll_fs.h"
#include "atoll_log.h"

/*
namespace Atoll {

class SDFS : public fs::SDFS {
   public:
    SDFS(fs::FSImplPtr impl) : fs::SDFS(impl) {
        // TODO get rid of SD
        // log_i("deleting SD");
        // delete &SD;
    }
    int format();
};
}  // namespace Atoll

extern Atoll::SDFS AtollSD;
*/

namespace Atoll {

class SdCard : public Fs {
   public:
    uint8_t csPin;
    fs::SDFS *card = &SD;  // = &AtollSD;

    SdCard(uint8_t cs, SemaphoreHandle_t *mutex = nullptr);
    virtual ~SdCard() {}
    void setup();
    fs::FS *pFs();
    void unmount();
    int format();
};

}  // namespace Atoll

#endif