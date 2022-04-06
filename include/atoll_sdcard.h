#ifndef __atoll_sdcard_h
#define __atoll_sdcard_h

#include "FS.h"
#include "SD.h"
#include "SPI.h"

// #include "vfs_fat_internal.h" // for format()

#include "atoll_fs.h"
#include "atoll_log.h"

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
        if (!SD.begin(csPin, spi, 4000000U, "/sd", (uint8_t)5U, true)) {
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

    fs::FS *pFs() {
        return &SD;
    }

    fs::FS &refFs() {
        return SD;
    }

    void unmount() {
        SD.end();
    }

    int format() {
        log_e("cannot format");
        return -1;

        // FRESULT res = FR_OK;
        // esp_err_t err = ESP_OK;
        // const size_t workbuf_size = 4096;
        // void *workbuf = NULL;
        // log_i("partitioning card");

        // workbuf = ff_memalloc(workbuf_size);
        // if (workbuf == NULL) {
        //     return ESP_ERR_NO_MEM;
        // }

        // BYTE pdrv = 0xFF;  // TODO get physical drive address from SD
        // DWORD plist[] = {100, 0, 0, 0};
        // res = f_fdisk(pdrv, plist, workbuf);
        // if (res != FR_OK) {
        //     err = ESP_FAIL;
        //     log_e("f_fdisk failed (%d)", res);
        // } else {
        //     size_t alloc_unit_size = 512;
        //     log_i("formatting card, allocation unit size=%d", alloc_unit_size);
        //     res = f_mkfs("", FM_FAT, alloc_unit_size, workbuf, workbuf_size);
        //     if (res != FR_OK) {
        //         err = ESP_FAIL;
        //         log_e("f_mkfs failed (%d)", res);
        //     }
        // }

        // log_i("partitioning card finished");
        // free(workbuf);
        // return (int)err;
    }
};

}  // namespace Atoll

#endif