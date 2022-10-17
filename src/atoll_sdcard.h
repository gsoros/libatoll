#ifndef __atoll_sdcard_h
#define __atoll_sdcard_h

#include "FS.h"
#include "SD.h"
#include "SPI.h"

//#include "vfs_fat_internal.h"

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

    SdCard(uint8_t cs, SemaphoreHandle_t *mutex = nullptr) {
        csPin = cs;
        if (nullptr != mutex)
            this->mutex = mutex;
    }

    virtual ~SdCard() {}

    void setup() {
        if (!aquireMutex()) {
            log_e("failed to aquire mutex");
            return;
        }
        if (!card->begin(csPin, SPI, 4000000U, "/sd", (uint8_t)5U, true)) {
            log_e("mount failed");
            releaseMutex();
            return;
        }

        uint8_t cardType = card->cardType();
        if (cardType == CARD_NONE) {
            log_e("no card");
            releaseMutex();
            return;
        }
        mounted = true;

        switch (cardType) {
            case CARD_MMC:
                log_i("card type: MMC");
                break;
            case CARD_SD:
                log_i("card type: SDSC");
                break;
            case CARD_SDHC:
                log_i("card type: SDHC");
                break;
            default:
                log_i("card type: unknown");
        }

        log_i("card size: %lluMB, bytes total: %llu, used: %llu, free: %llu",
              card->cardSize() / (1024 * 1024),
              card->totalBytes(),
              card->usedBytes(),
              card->totalBytes() - SD.usedBytes());

        releaseMutex();
    }

    fs::FS *pFs() {
        return card;
    }

    void unmount() {
        if (!aquireMutex()) return;
        card->end();
        releaseMutex();
    }

    int format() {
        return -1;
        // return card->format();
    }
};

}  // namespace Atoll

#endif