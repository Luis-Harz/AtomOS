#include "fs/fat/ff.h"
#include "fs/fat/diskio.h"

uint32_t fat_partition_lba = 0;

extern int ata_read(uint8_t *buf, uint32_t lba, uint32_t count);
extern int ata_write(const uint8_t *buf, uint32_t lba, uint32_t count);

DSTATUS disk_initialize(BYTE pdrv) { return 0; }
DSTATUS disk_status(BYTE pdrv)     { return 0; }

DRESULT disk_read(BYTE pdrv, BYTE *buff, DWORD sector, UINT count) {
    return ata_read(buff, sector + fat_partition_lba, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT disk_write(BYTE pdrv, const BYTE *buff, DWORD sector, UINT count) {
    return ata_write(buff, sector + fat_partition_lba, count) == 0 ? RES_OK : RES_ERROR;
}

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff) {
    switch (cmd) {
        case CTRL_SYNC:       return RES_OK;
        case GET_SECTOR_SIZE: *(WORD *)buff  = 512; return RES_OK;
        case GET_BLOCK_SIZE:  *(DWORD *)buff = 1;   return RES_OK;
    }
    return RES_PARERR;
}
DWORD get_fattime(void) {
    return ((DWORD)(2025 - 1980) << 25) | ((DWORD)1 << 21) | ((DWORD)1 << 16);
}