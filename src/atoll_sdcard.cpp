/*

#include "atoll_sdcard.h"
#include "vfs_api.h"
#include "ff.h"

using namespace Atoll;

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