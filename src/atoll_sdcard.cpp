#ifdef FEATURE_SDCARD
#include "atoll_sdcard.h"

// #include "vfs_api.h"
// #include "ff.h"

using namespace Atoll;

SdCard::SdCard(uint8_t cs, SemaphoreHandle_t *mutex) {
    csPin = cs;
    if (nullptr != mutex)
        this->mutex = mutex;
}

void SdCard::setup() {
    if (!aquireMutex()) {
        log_e("failed to aquire mutex");
        return;
    }

    /*
        fix for "sdcard_mount(): f_mount failed: (3) The physical drive cannot work"
        https://github.com/espressif/arduino-esp32/issues/6081#issuecomment-1015426248
        SD/src/sd_diskio.cpp
        bool sdSelectCard(uint8_t pdrv) {
            ardu_sdcard_t* card = s_cards[pdrv];
            digitalWrite(card->ssPin, LOW);
            bool s = sdWait(pdrv, 500);
            if (!s) {
                log_e("Select Failed");
                // digitalWrite(card->ssPin, HIGH); // commented out
                // return false;                    // commented out
            }
            return true;
        }
    */

    if (!card->begin(csPin, SPI,
                     4000000U,  //  4MHz default
                     // 16000000U,  //  16MHz
                     // 27000000U,  //  27MHz
                     // 40000000U,  //  40MHz
                     // 1000000U,   //   1MHz
                     // 400000U,    // 400kHz
                     "/sd", (uint8_t)5U, true)) {
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

fs::FS *SdCard::pFs() {
    return card;
}

void SdCard::unmount() {
    if (!aquireMutex()) return;
    card->end();
    releaseMutex();
}

int SdCard::format() {
    return -1;
    // return card->format();
}

/*
int Atoll::SDFS::format() {
    FRESULT res = FR_OK;
    esp_err_t err = ESP_OK;
    const size_t workbuf_size = 4096;
    void *workbuf = NULL;
    log_i("partitioning card");

    // // connect driver to FATFS
    // BYTE pdrv = 0xFF;
    // if (ff_diskio_get_drive(&pdrv) != ESP_OK) {
    //     log_i("the maximum count of volumes is already mounted");
    //     return ESP_ERR_NO_MEM;
    // }
    // log_i("using pdrv=%i", pdrv);
    // char drv[3] = {(char)('0' + pdrv), ':', 0};

    char drv[3] = {(char)('0' + _pdrv), ':', 0};
    log_i("_pdrv: %d, drv: %s", _pdrv, drv);

    workbuf = ff_memalloc(workbuf_size);
    if (workbuf == NULL) {
        return ESP_ERR_NO_MEM;
    }

    DWORD plist[] = {100, 0, 0, 0};
    res = f_fdisk(_pdrv, plist, workbuf);
    if (res != FR_OK) {
        err = ESP_FAIL;
        log_e("f_fdisk failed (%d)", res);
    } else {
        size_t alloc_unit_size = 512;
        log_i("formatting card, allocation unit size=%d", alloc_unit_size);
        res = f_mkfs(drv, FM_FAT, alloc_unit_size, workbuf, workbuf_size);
        if (res != FR_OK) {
            err = ESP_FAIL;
            log_e("f_mkfs failed (%d)", res);
        }
    }

    log_i("partitioning card finished");
    free(workbuf);
    return (int)err;
}

Atoll::SDFS AtollSD = Atoll::SDFS(FSImplPtr(new VFSImpl()));

*/

#endif