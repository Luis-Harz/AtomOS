#include "fat/ff.h"
#include "fat/diskio.h"
#include "ata/ata.c"
#include "device.h"

extern device_t *ata_primary_master;

DSTATUS disk_initialize(BYTE pdrv) {
    ata_init();
    return 0;
}

DSTATUS disk_status(BYTE pdrv) {
    return 0;
}

DRESULT disk_read(BYTE pdrv, BYTE* buff, DWORD sector, UINT count) {
    ata_read(buff, sector, count, ata_primary_master);
    return RES_OK;
}

DRESULT disk_write(BYTE pdrv, const BYTE* buff, DWORD sector, UINT count) {
    for (UINT i = 0; i < count; i++) {
        ata_write_one(buff + i*512, sector + i, ata_primary_master);
    }
    return RES_OK;
}
