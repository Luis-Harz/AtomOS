#include <stdint.h>
#include "mouse.h"
#include "vga.h"

extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);

ps2mouse_state_t mouse;

uint8_t mouse_cycle = 0;
uint8_t mouse_packet[3];

int last_x = -1;
int last_y = -1;
int first_mouse_frame = 1;

static uint32_t cursor_bg[CURSOR_SIZE * CURSOR_SIZE];

static const uint8_t cursor_bitmap[CURSOR_SIZE] = {
    0b11110000,
    0b11000000,
    0b10100000,
    0b10010000,
    0b00001000,
    0b00000100,
    0b00000010,
    0b00000001
};

static void ps2mouse_wait_input() {
    while (inb(0x64) & 2);
}

static void ps2mouse_wait_output() {
    while (!(inb(0x64) & 1));
}

void save_cursor_bg(int x, int y)
{
    for (int dy = 0; dy < CURSOR_SIZE; dy++)
    {
        for (int dx = 0; dx < CURSOR_SIZE; dx++)
        {
            int px = x + dx;
            int py = y + dy;

            if (px >= (int)fb_width || py >= (int)fb_height)
                continue;

            uint32_t *pixel =
                (uint32_t*)(fb_addr + py * fb_pitch + px * 4);

            cursor_bg[dy * CURSOR_SIZE + dx] = *pixel;
        }
    }
}

void restore_cursor_bg(int x, int y)
{
    for (int dy = 0; dy < CURSOR_SIZE; dy++)
    {
        for (int dx = 0; dx < CURSOR_SIZE; dx++)
        {
            int px = x + dx;
            int py = y + dy;

            if (px >= (int)fb_width || py >= (int)fb_height)
                continue;

            uint32_t *pixel =
                (uint32_t*)(fb_addr + py * fb_pitch + px * 4);

            *pixel = cursor_bg[dy * CURSOR_SIZE + dx];
        }
    }
}

void draw_cursor(int x, int y)
{
    for (int dy = 0; dy < CURSOR_SIZE; dy++)
    {
        for (int dx = 0; dx < CURSOR_SIZE; dx++)
        {
            if (!(cursor_bitmap[dy] & (0x80 >> dx)))
                continue;

            int px = x + dx;
            int py = y + dy;

            if (px >= (int)fb_width || py >= (int)fb_height)
                continue;

            uint32_t *pixel =
                (uint32_t*)(fb_addr + py * fb_pitch + px * 4);

            *pixel ^= 0xFFFFFF;
        }
    }
}

void update_mouse()
{
    if(mouse.x == last_x && mouse.y == last_y)
        return;

    if(first_mouse_frame == 0)
        restore_cursor_bg(last_x, last_y);

    save_cursor_bg(mouse.x, mouse.y);

    draw_cursor(mouse.x, mouse.y);

    last_x = mouse.x;
    last_y = mouse.y;

    first_mouse_frame = 0;
}

void ps2mouse_poll()
{
    uint8_t status = inb(0x64);

    if (!(status & 1))
        return;

    if (!(status & (1 << 5)))
        return;

    uint8_t data = inb(0x60);

    mouse_packet[mouse_cycle++] = data;

    if (mouse_cycle < 3)
        return;

    mouse_cycle = 0;

    if (!(mouse_packet[0] & 0x08))
        return;

    int dx = (int8_t)mouse_packet[1];
    int dy = (int8_t)mouse_packet[2];

    mouse.x += dx;
    mouse.y -= dy;

    if (mouse.x < 0) mouse.x = 0;
    if (mouse.y < 0) mouse.y = 0;

    if (mouse.x > (int)fb_width - CURSOR_SIZE)
        mouse.x = fb_width - CURSOR_SIZE;

    if (mouse.y > (int)fb_height - CURSOR_SIZE)
        mouse.y = fb_height - CURSOR_SIZE;

    mouse.left   = mouse_packet[0] & 1;
    mouse.right  = mouse_packet[0] & 2;
    mouse.middle = mouse_packet[0] & 4;
}

static void ps2mouse_write(uint8_t data)
{
    ps2mouse_wait_input();
    outb(0x64, 0xD4);
    ps2mouse_wait_input();
    outb(0x60, data);
}

static uint8_t ps2mouse_read()
{
    ps2mouse_wait_output();
    return inb(0x60);
}

void ps2mouse_init() {
    ps2mouse_wait_input(); outb(0x64, 0xAD); // disable keyboard
    ps2mouse_wait_input(); outb(0x64, 0xA7); // disable mouse
    while (inb(0x64) & 1) inb(0x60);
    ps2mouse_wait_input(); outb(0x64, 0x20);
    ps2mouse_wait_output();
    uint8_t config = inb(0x60);
    config &= ~(1 << 5); // clear "disable mouse clock"
    config &= ~(1 << 1); // clear mouse IRQ12 enable (we're polling)
    config &= ~(1 << 0); // clear keyboard IRQ1 enable
    ps2mouse_wait_input(); outb(0x64, 0x60);
    ps2mouse_wait_input(); outb(0x60, config);
    ps2mouse_wait_input(); outb(0x64, 0xA8);
    ps2mouse_write(0xFF);
    uint8_t ack = ps2mouse_read();
    uint8_t bat = ps2mouse_read();
    uint8_t id  = ps2mouse_read();
    ps2mouse_write(0xF6); ps2mouse_read();
    ps2mouse_write(0xF4); ps2mouse_read();
    ps2mouse_wait_input(); outb(0x64, 0xAE);
    mouse.x = fb_width  / 2;
    mouse.y = fb_height / 2;
    char buf[8];
    vga_print("ACK="); itoa(ack, buf); vga_print(buf);
    vga_print(" BAT="); itoa(bat, buf); vga_print(buf);
    vga_print(" ID=");  itoa(id,  buf); vga_print(buf);
    vga_print("\n");
}