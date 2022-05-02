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
    SPIClass spi = SPIClass();  // = SPIClass(VSPI);
    uint8_t sckPin;
    uint8_t misoPin;
    uint8_t mosiPin;
    uint8_t csPin;
    fs::SDFS *card = &SD;  // = &AtollSD;

    SdCard(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t cs) {
        sckPin = sck;
        misoPin = miso;
        mosiPin = mosi;
        csPin = cs;
    }
    virtual ~SdCard() {}

    void setup() {
        pinMode(misoPin, INPUT_PULLUP);
        // digitalWrite(csPin, HIGH);
        spi.begin(sckPin, misoPin, mosiPin, csPin);
        // digitalWrite(csPin, LOW);
        if (!card->begin(csPin, spi, 4000000U, "/sd", (uint8_t)5U, true)) {
            log_e("mount failed");
            return;
        }

        uint8_t cardType = card->cardType();
        if (cardType == CARD_NONE) {
            log_e("no card");
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
    }

    fs::FS *pFs() {
        return card;
    }

    void unmount() {
        card->end();
    }

    int format() {
        return -1;
        // return card->format();
    }
};

}  // namespace Atoll

#endif