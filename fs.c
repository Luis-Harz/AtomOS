#include <stdint.h>
#include "fs.h"
#include "fs/fat/ff.h"

static int fs_strlen(const char *s) {
    int n = 0;
    while (s[n]) n++;
    return n;
}

static uint8_t hex_to_byte(char high, char low) {
    uint8_t val = 0;
    if (high >= '0' && high <= '9') val += (high - '0') << 4;
    else if (high >= 'A' && high <= 'F') val += (high - 'A' + 10) << 4;
    else if (high >= 'a' && high <= 'f') val += (high - 'a' + 10) << 4;

    if (low >= '0' && low <= '9') val += (low - '0');
    else if (low >= 'A' && low <= 'F') val += (low - 'A' + 10);
    else if (low >= 'a' && low <= 'f') val += (low - 'a' + 10);

    return val;
}

static uint32_t parse_size(const char *str) {
    uint32_t val = 0;
    while (*str >= '0' && *str <= '9') {
        val = val * 10 + (*str - '0');
        str++;
    }
    return val;
}

/* fs_init: mounts the FAT volume */
void fs_init(fs_t *fs) {
    f_mount(&fs->fatfs, "0:", 1);
}

/* fs_deinit: unmounts the FAT volume */
void fs_deinit(fs_t *fs) {
    (void)fs;
    f_unmount("0:");
}

/* fs_read: reads a file into buf, returns bytes read or -1 on error */
int fs_read(const char *path, uint8_t *buf, uint32_t capacity) {
    FIL fil;
    UINT br;

    if (f_open(&fil, path, FA_READ) != FR_OK)
        return -1;

    FRESULT res = f_read(&fil, buf, capacity, &br);
    f_close(&fil);

    return (res == FR_OK) ? (int)br : -1;
}

/* fs_write: creates or overwrites a file with the given content */
int fs_write(const char *path, const uint8_t *content, uint32_t size) {
    FIL fil;
    UINT bw;

    if (f_open(&fil, path, FA_WRITE | FA_CREATE_ALWAYS) != FR_OK)
        return -1;

    FRESULT res = f_write(&fil, content, size, &bw);
    f_close(&fil);

    return (res == FR_OK && bw == size) ? 0 : -1;
}

/*fs_exists: check if file exists*/
int fs_exists(const char *path) {
    FILINFO fno;
    return (f_stat(path, &fno) == FR_OK);
}

/* fs_delete: removes a file */
int fs_delete(const char *path) {
    return (f_unlink(path) == FR_OK) ? 0 : -1;
}

/* fs_getcwd: gets current working directory */
int fs_getcwd(char *buf, uint32_t size) {
    return (f_getcwd(buf, size) == FR_OK) ? 0 : -1;
}

/*fs_cd: changes current directory*/
int fs_cd(const char *path) {
    return (f_chdir(path) == FR_OK) ? 0 : -1;
}

/*fs_mkdir: make a directory*/
int fs_mkdir(const char *path) {
    return (f_mkdir(path) == FR_OK) ? 0 : -1;
}

/* fs_list: calls callback once per file in the root directory */
void fs_list(void (*callback)(const char *path)) {
    DIR dir;
    FILINFO fno;

    if (f_opendir(&dir, ".") != FR_OK)
        return;

    while (f_readdir(&dir, &fno) == FR_OK && fno.fname[0] != 0) {
        char buf[FF_LFN_BUF + 4];
        int i = 0;

        while (fno.fname[i] && i < FF_LFN_BUF) {
            buf[i] = fno.fname[i];
            i++;
        }

        if (fno.fattrib & AM_DIR) {
            buf[i++] = '/';
        }

        buf[i++] = '\n';
        buf[i] = 0;

        callback(buf);
    }

    f_closedir(&dir);
}

/* run_program: Simple ELF Loader
*/

typedef struct {
    unsigned char e_ident[16];
    uint16_t e_type;
    uint16_t e_machine;
    uint32_t e_version;
    uint32_t e_entry;
    uint32_t e_phoff;
    uint32_t e_shoff;
    uint32_t e_flags;
    uint16_t e_ehsize;
    uint16_t e_phentsize;
    uint16_t e_phnum;
} Elf32_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_offset;
    uint32_t p_vaddr;
    uint32_t p_filesz;
    uint32_t p_memsz;
    uint32_t p_flags;
    uint32_t p_align;
} Elf32_Phdr;

#define PT_LOAD 1

void run_program(const char *path) {
    FIL fil;
    UINT br;

    if (f_open(&fil, path, FA_READ) != FR_OK) return;

    Elf32_Ehdr hdr;
    if (f_read(&fil, &hdr, sizeof(hdr), &br) != FR_OK || br != sizeof(hdr)) {
        f_close(&fil);
        return;
    }

    if (hdr.e_ident[0] != 0x7F || hdr.e_ident[1] != 'E' || 
        hdr.e_ident[2] != 'L' || hdr.e_ident[3] != 'F') {
        f_close(&fil);
        return;
    }

    for (int i = 0; i < hdr.e_phnum; i++) {
        Elf32_Phdr ph;
        f_lseek(&fil, hdr.e_phoff + i * sizeof(ph));
        if (f_read(&fil, &ph, sizeof(ph), &br) != FR_OK || br != sizeof(ph)) {
            f_close(&fil);
            return;
        }

        if (ph.p_type != PT_LOAD) continue;

        f_lseek(&fil, ph.p_offset);
        if (f_read(&fil, (void*)ph.p_vaddr, ph.p_filesz, &br) != FR_OK || br != ph.p_filesz) {
            f_close(&fil);
            return;
        }

        for (uint32_t j = ph.p_filesz; j < ph.p_memsz; j++)
            ((uint8_t*)ph.p_vaddr)[j] = 0;
    }

    f_close(&fil);
    uint32_t *stack = (uint32_t*)0x8000;
    asm volatile("movl %0, %%esp" : : "r"(stack));
    void (*entry)() = (void (*)())hdr.e_entry;
    entry();
}


//Append at end of file
int fs_append(const char *path, const uint8_t *content, uint32_t size) {
    FIL fil;
    UINT bw;
    if (f_open(&fil, path, FA_WRITE | FA_OPEN_ALWAYS) != FR_OK) {
        return -1;
    }

    if (f_lseek(&fil, f_size(&fil)) != FR_OK) {
        f_close(&fil);
        return -1;
    }

    FRESULT res = f_write(&fil, content, size, &bw);
    f_close(&fil);
    return (res == FR_OK && bw == size) ? 0 : -1;
}