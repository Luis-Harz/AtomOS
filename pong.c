#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "pong.h"
//For Randomization
#include "snake.h"

// Game state
static uint16_t back_buffer_pong[VGA_HEIGHT * VGA_WIDTH];
int pong1x = 5;
int pong2x = VGA_WIDTH - 5;
int pong1y = VGA_HEIGHT / 2;
int pong2y = VGA_HEIGHT / 2;
int ballx = 40;
int bally = 12;
//Is running
int running = 0;
//Ball direction x and y
int ball_dx = 1; // +1 = rechts, -1 = links
int ball_dy = 1; // +1 = unten, -1 = oben

// Check keyboard status register before reading data
// Port 0x64 bit 0 = output buffer full (data is ready)
static int keyboard_ready() {
    return inb(0x64) & 0x01;
}

void resetgamepong() {
    ballx = 40;
    bally = 12;
    //Randomize Ball start
    int balldxpre = rand16() % 2;
    int balldypre = rand16() % 2;
    if (balldxpre == 1) {
        ball_dx = 1;
    } else {
        ball_dx = -1;
    }
    if (balldypre == 1) {
        ball_dy = 1;
    } else {
        ball_dy = -1;
    }
    pong1y = VGA_HEIGHT / 2;
    pong2y = VGA_HEIGHT / 2;
}

void poll_keyboard() {
    //if (!keyboard_ready()) return;  // nothing to read

    uint8_t scancode = inb(KEYBOARD_DATA);
    switch (scancode) {
        case 0x11: if (pong1y > 2) {pong1y--;} break;
        case 0x1F: if (pong1y < VGA_HEIGHT - 3) {pong1y++;} break;
        case 0x01: running = 0;
        default: break;
    }
    pong2y = bally;
}

void change_ball_direction() {
    if (bally <= 0 || bally >= VGA_HEIGHT - 1) {
        ball_dy = -ball_dy;
    }
    if (ballx == pong1x + 1 && bally >= pong1y - 1 && bally <= pong1y + 1) {
        ball_dx = -ball_dx;
    }
    if (ballx == pong2x - 1 && bally >= pong2y - 1 && bally <= pong2y + 1) {
        ball_dx = -ball_dx;
    }
    if (ballx <= 0 || ballx >= VGA_WIDTH - 1) {
        //Also reset game
        resetgamepong();
    }
}

void put_c_back_buffer(char c) {
    back_buffer_pong[cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | (uint8_t)c;
}

void print_back_buffer(const char *str) {
    while (*str) put_c_back_buffer(*str++);
}


void put_char_back_buffer(int x, int y, char ch, uint8_t fg, uint8_t bg) {
    uint16_t entry = ((bg << 4 | fg) << 8) | (uint8_t)ch;
    back_buffer_pong[y * VGA_WIDTH + x] = entry;
}

void pong_put_block(int x, int y, uint8_t fg, uint8_t bg) {
    put_char_back_buffer(x, y, FULL_BLOCK, fg, bg);
}

void put_block_with_char(int x, int y, char ch, uint8_t fg, uint8_t bg) {
    uint16_t entry = ((bg << 4 | fg) << 8) | (uint8_t)ch;
    back_buffer_pong[y * VGA_WIDTH + x] = entry;
}

void put_char(int x, int y, char ch, uint8_t fg, uint8_t bg) {
    if (use_framebuffer) {
        fb_drawchar(x, y, ch, fg, bg);
    } else {
        uint16_t col = (bg << 4) | fg;
        vga_buffer[y * VGA_WIDTH + x] = ((uint16_t)col << 8) | (uint8_t)ch;
    }
}

void update() {
    //Change Ball Pos on Collision
    ballx += ball_dx;
    bally += ball_dy;
    //Ball Collision Check
    change_ball_direction();
}

void pong_clear_buffer() {
    uint8_t bg = 0;
    uint8_t fg = 0;
    uint16_t blank = ((bg << 4 | fg) << 8) | ' ';
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            back_buffer_pong[y * VGA_WIDTH + x] = blank;
        }
    }
}

void present() {
    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            uint16_t entry = back_buffer_pong[y * VGA_WIDTH + x];
            char ch      = entry & 0x00FF;
            uint8_t fg   = (entry & 0x0F00) >> 8;
            uint8_t bg   = (entry & 0xF000) >> 12;
            put_char(x, y, ch, fg, bg);
        }
    }
}

void draw_border() {
    uint8_t fg = WHITE;
    uint8_t bg = 0;
    for (int x = 0; x < VGA_WIDTH; x++) {
        pong_put_block(x, 0, fg, bg);
        pong_put_block(x, VGA_HEIGHT - 1, fg, bg);
    }
    for (int y = 0; y < VGA_HEIGHT; y++) {
        pong_put_block(0, y, fg, bg);
        pong_put_block(VGA_WIDTH - 1, y, fg, bg);
    }
}

static void render() {
    pong_clear_buffer();

    // Paddle 1 (RED)
    for (int i = -1; i <= 1; i++) {
        pong_put_block(pong1x, pong1y + i, RED, RED);
    }

    // Paddle 2 (GREEN)
    for (int i = -1; i <= 1; i++) {
        pong_put_block(pong2x, pong2y + i, GREEN, GREEN);
    }

    //Ball (BLUE)
    pong_put_block(ballx, bally, BLUE, BLUE);
    draw_border();
    present();
}

void pong() {
    resetgamepong();
    running = 1;
    while (running) {
        poll_keyboard();   // 1. input
        if (pong2y < 2) {
            pong2y = 2;
        }
        if (pong2y > VGA_HEIGHT - 3) {
            pong2y = VGA_HEIGHT - 3;
        }
        update();              // 2. logic
        render();              // 3. draw
        delay_ms(8);        // 4. ~60fps
    }
    resetgamepong();
    vga_clear();
}