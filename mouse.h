#ifndef PS2MOUSE_H
#define PS2MOUSE_H

#include <stdint.h>

//Extern
extern uint8_t inb(uint16_t port);
extern void outb(uint16_t port, uint8_t val);
extern uint32_t fb_width;
extern uint32_t fb_height;

typedef struct {
    int x;
    int y;
    uint8_t left;
    uint8_t right;
    uint8_t middle;
} ps2mouse_state_t;

static ps2mouse_state_t mouse;
static uint8_t mouse_cycle = 0;
static uint8_t mouse_packet[3];

static void ps2mouse_wait_input() {
    while (inb(0x64) & 2);
}

static void ps2mouse_wait_output() {
    while (!(inb(0x64) & 1));
}

//Mouse write
static void ps2mouse_write(uint8_t data) {
    ps2mouse_wait_input();
    outb(0x64, 0xD4);
    ps2mouse_wait_input();
    outb(0x60, data);
}
//Mouse Read
static uint8_t ps2mouse_read() {
    ps2mouse_wait_output();
    return inb(0x60);
}

//Init
static void ps2mouse_init() {
    outb(0x64, 0xA8);
    outb(0x64, 0x20);
    ps2mouse_wait_output();
    uint8_t status = inb(0x60);
    status |= 2;
    ps2mouse_wait_input();
    outb(0x64, 0x60);
    ps2mouse_wait_input();
    outb(0x60, status);
    ps2mouse_write(0xF6);
    ps2mouse_read();
    ps2mouse_write(0xF4);
    ps2mouse_read();

    mouse.x = 0;
    mouse.y = 0;
}

/* IRQ12 handler */
static void ps2mouse_handle_irq() {

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

    if (mouse.x > (int)fb_width)  mouse.x = fb_width;
    if (mouse.y > (int)fb_height) mouse.y = fb_height;

    mouse.left   = mouse_packet[0] & 1;
    mouse.right  = mouse_packet[0] & 2;
    mouse.middle = mouse_packet[0] & 4;
}

#endif