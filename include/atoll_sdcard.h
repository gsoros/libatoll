#ifndef __atoll_sdcard_h
#define __atoll_sdcard_h

#include "FS.h"
#include "SD.h"
#include "SPI.h"

#include "atoll_fs.h"

namespace Atoll {

class SdCard : public Fs {
   public:
    SPIClass spi = SPIClass(VSPI);
    uint8_t sckPin;
    uint8_t misoPin;
    uint8_t mosiPin;
    uint8_t csPin;

    SdCard(uint8_t sck, uint8_t miso, uint8_t mosi, uint8_t cs) {
        sckPin = sck;
        misoPin = miso;
        mosiPin = mosi;
        csPin = cs;
    }
    virtual ~SdCard() {}

    void setup() {
        spi.begin(sckPin, misoPin, mosiPin, csPin);
        if (!SD.begin(csPin, spi)) {
            log_e("card mount failed");
            return;
        }

        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            log_e("no SD card inserted");
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

        log_i("card size: %lluMB, bytes total: %d, used: %d, free: %d",
              SD.cardSize() / (1024 * 1024),
              SD.totalBytes(),
              SD.usedBytes(),
              SD.totalBytes() - SD.usedBytes());
    }

    fs::FS *fsp() {
        return &SD;
    }
};

}  // namespace Atoll

#endif