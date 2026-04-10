#include <stdint.h>
#include "fs.c"
#include "vga.h"
#include "keyboard.h"
#include "fs/ata/ata.h"
#include "libc.h"
#include "mouse.h"
#include "fs/fat/ff.h"
#include "fs/fat/diskio.h"
#include "pci.h"
#include "kernel.h"
#include "pong.h"
#include "com_rtl.h"

int write(char* file, char* content) {
    int status = fs_write(file, (uint8_t*)content, kstrlen(content));
    if (status == 0) {
        return 0;
    } else {
        return -1;
    }
}

int read(char* path, char* buf) {
    static uint8_t cat_buf[4096];
    int n = fs_read(path, cat_buf, sizeof(cat_buf) - 1);
    if (n >= 0) {
        cat_buf[n] = 0;
        memcpy(buf, (char*)cat_buf, sizeof(cat_buf) - 1);
        return 0;
    } else {
        return -1;
    }
}