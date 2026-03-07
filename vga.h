#ifndef VGA_H
#define VGA_H

#include "timer.h"
#include <stdint.h>
#include "piezo.h"
#include "font_data.h"

typedef struct {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    uint32_t boot_device;
    uint32_t cmdline;
    uint32_t mods_count;
    uint32_t mods_addr;
    uint32_t syms[4];
    uint32_t mmap_length;
    uint32_t mmap_addr;
    uint32_t drives_length;
    uint32_t drives_addr;
    uint32_t config_table;
    uint32_t boot_loader_name;
    uint32_t apm_table;
    uint32_t vbe_control_info;
    uint32_t vbe_mode_info;
    uint16_t vbe_mode;
    uint16_t vbe_interface_seg;
    uint16_t vbe_interface_off;
    uint16_t vbe_interface_len;
    uint64_t fb_addr;
    uint32_t fb_pitch;
    uint32_t fb_width;
    uint32_t fb_height;
    uint8_t  fb_bpp;
    uint8_t  fb_type;
} __attribute__((packed)) multiboot_info_t;

// ── Color constants ──────────────────────────────────────────────────────────
#define BLACK         0x0
#define BLUE          0x1
#define GREEN         0x2
#define CYAN          0x3
#define RED           0x4
#define MAGENTA       0x5
#define BROWN         0x6
#define LIGHT_GRAY    0x7
#define DARK_GRAY     0x8
#define LIGHT_BLUE    0x9
#define LIGHT_GREEN   0xA
#define LIGHT_CYAN    0xB
#define LIGHT_RED     0xC
#define LIGHT_MAGENTA 0xD
#define YELLOW        0xE
#define WHITE         0xF

#define VGA_WIDTH   80
#define VGA_HEIGHT  25
#define FULL_BLOCK  0xDB

// ── Externals — defined once in vga.c ───────────────────────────────────────
extern volatile uint16_t *vga_buffer;
extern uint8_t  color;
extern int      cursor_x;
extern int      cursor_y;
extern int      use_framebuffer;
extern uint8_t *fb_addr;
extern uint32_t fb_pitch;
extern uint32_t fb_width;
extern uint32_t fb_height;
extern uint8_t  fb_bpp;

// ── Function declarations ────────────────────────────────────────────────────
void vga_init(multiboot_info_t *mb);
void vga_clear(void);
void vga_print(const char *str);
void vga_putc(char c);
void vga_scroll(void);
void vga_backspace(char *command, int *cmd_index);
void update_cursor(int x, int y);
void put_block(int x, int y, uint8_t fg, uint8_t bg);
int  vga_used_lines(void);
void logo(char *lg);
void colortest(void (*delay_fn)(uint32_t));

#endif // VGA_H