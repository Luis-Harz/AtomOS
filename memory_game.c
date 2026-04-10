#include "keyboard.h"
#include "vga.h"
#define FB_WIDTH 640
#define FB_HEIGHT 400
uint32_t backbuffer[FB_WIDTH * FB_HEIGHT];

void bb_putpixel(uint32_t x, uint32_t y, uint32_t c) {
    uint32_t *pixel = backbuffer + y * FB_WIDTH + x;
    *pixel = vga_to_rgb_func(c);
}

void make_box(int start_x, int start_y, int width, int height, uint32_t c) {
    for (int x = start_x; x < (start_x + width); x++) {
        for (int y = start_y; y < (start_y + height); y++) {
            bb_putpixel(x, y, c);
        }
    }
}

void present() {
    uint32_t *fb = (uint32_t*)fb_addr;
    for (int y = 0; y < FB_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            fb[y * (fb_pitch / 4) + x] = backbuffer[y * FB_WIDTH + x];
        }
    }
}