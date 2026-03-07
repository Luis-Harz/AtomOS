#ifndef ATA_H
#define ATA_H
#include <stdint.h>

int      ata_read(uint8_t *buf, uint32_t lba, uint32_t count);
int      ata_write(const uint8_t *buf, uint32_t lba, uint32_t count);
uint32_t ata_find_fat_partition(void);

#endif