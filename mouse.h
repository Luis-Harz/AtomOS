#ifndef MOUSE_H
#define MOUSE_H

#include <stdint.h>
#include <stdbool.h>

extern uint32_t fb_width;
extern uint32_t fb_height;
extern uint8_t *fb_addr;
#define CURSOR_SIZE 8
extern uint32_t fb_pitch;
static const uint8_t cursor_bitmap[CURSOR_SIZE];
extern int last_x;
extern int last_y;
extern int first_mouse_frame;
extern uint8_t bat;
extern uint8_t ack;
extern uint8_t id;
extern uint8_t mouse_cycle;

typedef struct {
    int x;
    int y;
    uint8_t left;
    uint8_t right;
    uint8_t middle;
} ps2mouse_state_t;

extern ps2mouse_state_t mouse;

void ps2mouse_init(void);
void ps2mouse_poll(void);
void restore_cursor_bg(int x, int y);
void update_mouse(void);

#endif