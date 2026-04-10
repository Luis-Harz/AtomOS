#include <stdint.h>
#include "vga.h"
#include "ttys.h"

#define FB_HEIGHT 400
#define FB_WIDTH 640
#define TTYS 4

uint32_t tty[TTYS][FB_WIDTH * FB_HEIGHT];
int usedttys[TTYS];
int UsedttysInt = 0;
int activetty = 0;

int get_active_tty() {
    return activetty;
}

int set_active_tty(int arg) {
    if (activetty != arg) {
        activetty = arg;
    }
}

int draw_tty(int arg) {
    if (usedttys[arg] == 1) {
        for (int i = 0; i < FB_WIDTH * FB_HEIGHT; i++) {
            fb_addr[i] = tty[arg][i];
        }
    }
}

int request_tty() {
    for (int i = 0; i < TTYS; i++) {
        if (usedttys[i] == 0) {
            usedttys[i] = 1;
            UsedttysInt++;
            return i;
        }
    }
    return -1;
}

int detach_tty(int arg) {
    if (usedttys[arg] == 1) {
        UsedttysInt--;
        usedttys[arg] = 0;
        return 1;
    } else {
        return 0;
    }
}