#include "vga.h"
#include "mouse.h"

// ── Global definitions — exactly one copy ───────────────────────────────────
volatile uint16_t *vga_buffer = (volatile uint16_t *)0xB8000;
uint8_t  color           = 0x0F;
int      cursor_x        = 0;
int      cursor_y        = 0;
int      use_framebuffer = 0;
uint8_t *fb_addr         = 0;
uint32_t fb_pitch        = 0;
uint32_t fb_width        = 0;
uint32_t fb_height       = 0;
uint8_t  fb_bpp          = 0;

// ── VGA color → 32-bit RGB ───────────────────────────────────────────────────
static const uint32_t vga_to_rgb[16] = {
    0x000000, 0x0000AA, 0x00AA00, 0x00AAAA,
    0xAA0000, 0xAA00AA, 0xAA5500, 0xAAAAAA,
    0x555555, 0x5555FF, 0x55FF55, 0x55FFFF,
    0xFF5555, 0xFF55FF, 0xFFFF55, 0xFFFFFF,
};

#define FB_CHAR_W  8
#define FB_CHAR_H  16
#define FB_COLS    (fb_width  / FB_CHAR_W)
#define FB_ROWS    (fb_height / FB_CHAR_H)

// ── Internal helpers ─────────────────────────────────────────────────────────
static inline void vga_outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

void fb_putpixel(uint32_t x, uint32_t y, uint32_t c) {
    uint32_t *pixel = (uint32_t *)(fb_addr + y * fb_pitch + x * 4);
    *pixel = vga_to_rgb_func(c);
}

void fb_draw_line(int x0, int y0, int x1, int y1, uint32_t color) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = (dx >= 0) ? 1 : -1;
    int sy = (dy >= 0) ? 1 : -1;
    dx = dx >= 0 ? dx : -dx;
    dy = dy >= 0 ? dy : -dy;
    int err = dx + ( -dy );

    while (1) {
        fb_putpixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 >= -dy) { err += -dy; x0 += sx; }
        if (e2 <= dx) { err += dx; y0 += sy; }
    }
}

static void fb_draw_line_thick(int x0, int y0, int x1, int y1, uint32_t color, int thickness) {
    int dx = x1 - x0;
    int dy = y1 - y0;
    int sx = dx >= 0 ? 1 : -1;
    int sy = dy >= 0 ? 1 : -1;

    dx = dx >= 0 ? dx : -dx;
    dy = dy >= 0 ? dy : -dy;

    int err = dx + (-dy);

    while (1) {
        for (int tx = -thickness/2; tx <= thickness/2; tx++) {
            for (int ty = -thickness/2; ty <= thickness/2; ty++) {
                uint32_t px = x0 + tx;
                uint32_t py = y0 + ty;
                if (px < fb_width && py < fb_height)
                    fb_putpixel(px, py, color);
            }
        }

        if (x0 == x1 && y0 == y1) break;

        int e2 = 2 * err;
        if (e2 >= -dy) { err += -dy; x0 += sx; }
        if (e2 <= dx)  { err += dx;  y0 += sy; }
    }
}

void fb_draw_triangle_outline_thick(int x0,int y0,int x1,int y1,int x2,int y2,uint32_t color,int thickness){
    fb_draw_line_thick(x0,y0,x1,y1,color,thickness);
    fb_draw_line_thick(x1,y1,x2,y2,color,thickness);
    fb_draw_line_thick(x2,y2,x0,y0,color,thickness);
}

void fb_draw_triangle_outline(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t color) {
    fb_draw_line(x0, y0, x1, y1, color);
    fb_draw_line(x1, y1, x2, y2, color);
    fb_draw_line(x2, y2, x0, y0, color);
}

void fb_drawchar(int cx, int cy, char c, uint8_t fg, uint8_t bg) {
    uint32_t fgc = vga_to_rgb[fg & 0xF];
    uint32_t bgc = vga_to_rgb[bg & 0xF];
    int px = cx * FB_CHAR_W;
    int py = cy * FB_CHAR_H;
    for (int row = 0; row < FB_CHAR_H; row++) {
        uint8_t glyph = font_bin[(uint8_t)c * 16 + row];
        for (int col = 0; col < FB_CHAR_W; col++) {
            if (glyph & (0x80 >> col))
                fb_putpixel(px + col, py + row, fgc);
            else
                fb_putpixel(px + col, py + row, bgc);
        }
    }
}

static void fb_scroll(void) {
    uint32_t row_bytes = FB_CHAR_H * fb_pitch;
    uint8_t *dst = fb_addr;
    uint8_t *src = fb_addr + row_bytes;
    uint32_t copy_size = (FB_ROWS - 1) * row_bytes;
    for (uint32_t i = 0; i < copy_size; i++)
        dst[i] = src[i];
    uint8_t *last = fb_addr + (FB_ROWS - 1) * row_bytes;
    uint32_t bgc = vga_to_rgb[0];
    for (uint32_t y = 0; y < FB_CHAR_H; y++) {
        uint32_t *row = (uint32_t *)(last + y * fb_pitch);
        for (uint32_t x = 0; x < fb_width; x++)
            row[x] = bgc;
    }
    cursor_y = (int)(FB_ROWS - 1);
}

// ── Public API ───────────────────────────────────────────────────────────────
void vga_init(multiboot_info_t *mb) {
    if (mb && (mb->flags & (1 << 12)) && mb->fb_addr && mb->fb_bpp == 32) {
        use_framebuffer = 1;
        vga_print("Framebuffer ON\n");
        fb_addr   = (uint8_t *)(uint32_t)mb->fb_addr;
        fb_pitch  = mb->fb_pitch;
        fb_width  = mb->fb_width;
        fb_height = mb->fb_height;
        fb_bpp    = mb->fb_bpp;
    } else {
        vga_print("Framebuffer OFF\n");
    }
}

void change_color_vga(uint8_t in_color) {
    color = in_color;
}

void update_cursor(int x, int y) {
    if (!use_framebuffer) {
        uint16_t pos = (uint16_t)(y * VGA_WIDTH + x);
        vga_outb(0x3D4, 0x0F); vga_outb(0x3D5, pos & 0xFF);
        vga_outb(0x3D4, 0x0E); vga_outb(0x3D5, (pos >> 8) & 0xFF);
    }
}

void put_block(int x, int y, uint8_t fg, uint8_t bg) {
    if (use_framebuffer) {
        fb_drawchar(x, y, (char)0xDB, fg, bg);
    } else {
        uint16_t col = (bg << 4) | fg;
        vga_buffer[y * VGA_WIDTH + x] = ((uint16_t)col << 8) | FULL_BLOCK;
    }
}

int vga_used_lines(void) {
    if (use_framebuffer) return cursor_y;
    for (int y = VGA_HEIGHT - 1; y >= 0; y--)
        for (int x = 0; x < VGA_WIDTH; x++)
            if ((vga_buffer[y * VGA_WIDTH + x] & 0xFF) != ' ')
                return y + 1;
    return 0;
}

void vga_scroll(void)
{
    if (use_framebuffer)
    {
        if (!first_mouse_frame)
            restore_cursor_bg(last_x, last_y);
        fb_scroll();
        first_mouse_frame = 1;
        return;
    }
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buffer[y * VGA_WIDTH + x] =
                vga_buffer[(y + 1) * VGA_WIDTH + x];

    for (int x = 0; x < VGA_WIDTH; x++)
        vga_buffer[(VGA_HEIGHT - 1) * VGA_WIDTH + x] =
            ((uint16_t)color << 8) | ' ';

    cursor_y = VGA_HEIGHT - 1;
}

void vga_putc(char c) {
    int cols = use_framebuffer ? (int)FB_COLS : VGA_WIDTH;
    int rows = use_framebuffer ? (int)FB_ROWS : VGA_HEIGHT;
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        if (use_framebuffer)
            fb_drawchar(cursor_x, cursor_y, c, color & 0xF, (color >> 4) & 0xF);
        else
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | (uint8_t)c;
        cursor_x++;
        if (cursor_x >= cols) { cursor_x = 0; cursor_y++; }
    }
    while (cursor_y >= rows) vga_scroll();
}

void vga_print(const char *str) {
    while (*str) vga_putc(*str++);
}

void vga_clear(void) {
    cursor_x = 0;
    cursor_y = 0;
    if (use_framebuffer) {
        uint32_t bgc = vga_to_rgb[0];
        for (uint32_t y = 0; y < fb_height; y++) {
            uint32_t *row = (uint32_t *)(fb_addr + y * fb_pitch);
            for (uint32_t x = 0; x < fb_width; x++) {
                if (row[x] != bgc)
                    row[x] = bgc;
            }
        }
    } else {
        uint16_t blank = ((uint16_t)color << 8) | ' ';
        for (int y = 0; y < VGA_HEIGHT; y++)
            for (int x = 0; x < VGA_WIDTH; x++)
                if (vga_buffer[y * VGA_WIDTH + x] != blank)
                    vga_buffer[y * VGA_WIDTH + x] = blank;
    }
}

void vga_backspace(char *command, int *cmd_index) {
    if (*cmd_index > 0) {
        int cols = use_framebuffer ? (int)FB_COLS : VGA_WIDTH;
        if (cursor_x > 2) cursor_x--;
        else if (cursor_y > 0) { cursor_y--; cursor_x = cols - 1; }
        (*cmd_index)--;
        command[*cmd_index] = 0;
        if (use_framebuffer)
            fb_drawchar(cursor_x, cursor_y, ' ', color & 0xF, (color >> 4) & 0xF);
        else
            vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | ' ';
    } else {
        beep(440, 100);
    }
}

void colortest(void (*delay_fn)(uint32_t)) {
    uint8_t colors[] = { RED, GREEN, BLUE };
    for (int c = 0; c < 3; c++) {
        vga_clear();
        int cols = use_framebuffer ? (int)FB_COLS : VGA_WIDTH;
        int rows = use_framebuffer ? (int)FB_ROWS : VGA_HEIGHT;
        for (int x = 0; x < cols; x++)
            for (int y = 0; y < rows; y++)
                put_block(x, y, colors[c], colors[c]);
        delay_fn(1000);
    }
    vga_clear();
}

void logo(char *lg) {
    vga_clear();
    int cols = use_framebuffer ? (int)FB_COLS : VGA_WIDTH;
    int rows = use_framebuffer ? (int)FB_ROWS : VGA_HEIGHT;
    int logo_width  = 36;
    int logo_height = 5;
    int start_x = (cols - logo_width) / 2;
    int start_y = (rows - logo_height) / 2;
    cursor_x = start_x;
    cursor_y = start_y;
    for (char *p = lg; *p; p++) {
        if (*p == '1')       vga_putc((char)219);
        else if (*p == '0')  vga_putc(' ');
        else if (*p == '\n') { cursor_x = start_x; cursor_y++; }
    }
    delay_ms(1000);
    vga_clear();
}

void logo_crash(char *lg) {
    uint8_t saved_color = color;
    color = 0x0E;
    int cols = use_framebuffer ? (int)FB_COLS : VGA_WIDTH;
    int rows = use_framebuffer ? (int)FB_ROWS : VGA_HEIGHT;
    int logo_width  = 15;
    int logo_height = 8;
    int start_x = (cols - logo_width) / 2;
    int start_y = (rows - logo_height) / 2;
    cursor_x = start_x;
    cursor_y = start_y;
    for (char *p = lg; *p; p++) {
        if (*p == '1')       vga_putc((char)219);
        else if (*p == '0')  vga_putc(' ');
        else if (*p == '\n') { cursor_x = start_x; cursor_y++; }
    }
    color = saved_color;
}

char panic_logo[] =
    "00000001\n"
    "000000101\n"
    "0000010101\n"
    "00001001001\n"
    "000100000001\n"
    "0010000100001\n"
    "01000000000001\n"
    "111111111111111\n";

void panic(char* message) {
    vga_clear();
    logo_crash(panic_logo);
    vga_print(message);
    while (1) {__asm__ volatile ("hlt");}
}