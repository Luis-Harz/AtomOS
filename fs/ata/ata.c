#include <stdint.h>
/* ATA primary bus ports */
#define ATA_DATA        0x1F0
#define ATA_ERROR       0x1F1
#define ATA_SECT_COUNT  0x1F2
#define ATA_LBA_LO      0x1F3
#define ATA_LBA_MID     0x1F4
#define ATA_LBA_HI      0x1F5
#define ATA_DRIVE_HEAD  0x1F6
#define ATA_STATUS      0x1F7
#define ATA_COMMAND     0x1F7

#define ATA_STATUS_BSY  0x80
#define ATA_STATUS_RDY  0x40
#define ATA_STATUS_DRQ  0x08
#define ATA_STATUS_ERR  0x01

#define ATA_CMD_READ    0x20
#define ATA_CMD_WRITE   0x30

extern uint32_t fat_partition_lba;

static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}
static inline uint8_t inb(uint16_t port) {
    uint8_t val;
    __asm__ volatile ("inb %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t val;
    __asm__ volatile ("inw %1, %0" : "=a"(val) : "Nd"(port));
    return val;
}
static inline void outw(uint16_t port, uint16_t val) {
    __asm__ volatile ("outw %0, %1" : : "a"(val), "Nd"(port));
}

static void ata_wait_bsy(void) {
    while (inb(ATA_STATUS) & ATA_STATUS_BSY);
}
static void ata_wait_drq(void) {
    while (!(inb(ATA_STATUS) & ATA_STATUS_DRQ));
}

static void ata_setup(uint32_t lba, uint8_t count) {
    ata_wait_bsy();
    outb(ATA_DRIVE_HEAD, 0xE0 | ((lba >> 24) & 0x0F)); /* master, LBA mode */
    outb(ATA_SECT_COUNT, count);
    outb(ATA_LBA_LO,  (uint8_t)(lba));
    outb(ATA_LBA_MID, (uint8_t)(lba >> 8));
    outb(ATA_LBA_HI,  (uint8_t)(lba >> 16));
}

int ata_read(uint8_t *buf, uint32_t lba, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ata_setup(lba + i, 1);
        outb(ATA_COMMAND, ATA_CMD_READ);
        ata_wait_bsy();
        ata_wait_drq();

        if (inb(ATA_STATUS) & ATA_STATUS_ERR)
            return -1;

        uint16_t *ptr = (uint16_t *)(buf + i * 512);
        for (int j = 0; j < 256; j++)
            ptr[j] = inw(ATA_DATA);
    }
    return 0;
}

// MBR partition entry structure
typedef struct {
    uint8_t  status;
    uint8_t  chs_first[3];
    uint8_t  type;
    uint8_t  chs_last[3];
    uint32_t lba_start;
    uint32_t lba_size;
} __attribute__((packed)) mbr_partition_t;

uint32_t ata_find_fat_partition(void) {
    uint8_t mbr[512];
    if (ata_read(mbr, 0, 1) != 0)
        return 0;
    
    if (mbr[510] != 0x55 || mbr[511] != 0xAA)
        return 0;
    
    mbr_partition_t *parts = (mbr_partition_t *)(mbr + 0x1BE);
    for (int i = 0; i < 5; i++) {
        uint8_t type = parts[i].type;
        if (type == 0x0B || type == 0x0C || type == 0x06) {
            fat_partition_lba = parts[i].lba_start;
            return fat_partition_lba;
        }
    }
    return 0;
}

int ata_write(const uint8_t *buf, uint32_t lba, uint32_t count) {
    for (uint32_t i = 0; i < count; i++) {
        ata_setup(lba + i, 1);
        outb(ATA_COMMAND, ATA_CMD_WRITE);
        ata_wait_bsy();
        ata_wait_drq();

        if (inb(ATA_STATUS) & ATA_STATUS_ERR)
            return -1;

        const uint16_t *ptr = (const uint16_t *)(buf + i * 512);
        for (int j = 0; j < 256; j++)
            outw(ATA_DATA, ptr[j]);

        /* flush cache */
        outb(ATA_COMMAND, 0xE7);
        ata_wait_bsy();
    }
    return 0;
}
