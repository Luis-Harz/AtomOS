#include <stdint.h>
#include "vga.h"
#include "keyboard.h"
#include "timer.h"

#define MAX_SNAKE 200

uint16_t lfsr = 0xACE1u;
uint16_t bit;

int snakex[MAX_SNAKE];
int snakey[MAX_SNAKE];
int snakelength = 1;

int appleposx = 0;
int appleposy = 0;

int direction = 1;
int score = 0;
int runningsnake = 1;

uint8_t read_scancodesnake() {
    return inb(KEYBOARD_DATA);
}

uint16_t rand16() {
    bit = ((lfsr >> 0) ^ (lfsr >> 2) ^ (lfsr >> 3) ^ (lfsr >> 5)) & 1;
    lfsr = (lfsr >> 1) | (bit << 15);
    return lfsr;
}

void scorestring(int score2) {
    char buf[10];
    int i = 0;

    if (score2 == 0) {
        buf[i++] = '0';
    } else {
        int temp = score2;
        while (temp > 0) {
            buf[i++] = '0' + (temp % 10);
            temp /= 10;
        }
    }

    buf[i] = '\0';

    for(int j = 0; j < i/2; j++) {
        char t = buf[j];
        buf[j] = buf[i-1-j];
        buf[i-1-j] = t;
    }

    vga_print(buf);
}

void handle_key() {
    uint8_t sc = read_scancodesnake();

    switch(sc) {
        case 0x48: if (direction != 2) {direction = 1;} break;
        case 0x50: if (direction != 1) {direction = 2;} break;
        case 0x4B: if (direction != 4) {direction = 3;} break;
        case 0x4D: if (direction != 3) {direction = 4;} break;

        case 0x01:
            vga_clear();
            vga_print("Your Score:\n");
            scorestring(score);
            delay_ms(2000);
            runningsnake = 0;
            break;

        default:
            break;
    }
}

void spawn_apple() {
    int istouching = 1;
    while (istouching == 1) {
        for (int i = 0; i < snakelength; i++) {
            if (snakex[i] == appleposx && snakey[i] == appleposy) {
                istouching = 1;
            } else {
                istouching = 0;
            }
        }
        appleposx = rand16() % VGA_WIDTH;
        appleposy = rand16() % VGA_HEIGHT;
    }
}

void resetgame() {
    vga_clear();
    snakelength = 1;
    snakex[0] = 10;
    snakey[0] = 10;
    score = 0;
    spawn_apple();
}

void gameover() {
    vga_clear();
    vga_print("Game Over");
    delay_ms(2000);
    resetgame();
}

void checktouch() {

    if (snakex[0] == appleposx && snakey[0] == appleposy) {
        spawn_apple();
        score++;

        if (snakelength < MAX_SNAKE) {
            snakelength++;
        }
    }
    for (int i = 1; i < snakelength; i++) {
        if (snakex[0] == snakex[i] && snakey[0] == snakey[i]) {
            gameover();
        }
    }
    if (snakey[0] < 0) snakey[0] = VGA_HEIGHT - 1;
    if (snakey[0] >= VGA_HEIGHT) snakey[0] = 0;

    if (snakex[0] < 0) snakex[0] = VGA_WIDTH - 1;
    if (snakex[0] >= VGA_WIDTH) snakex[0] = 0;
}

void move() {

    for(int i = snakelength - 1; i > 0; i--) {
        snakex[i] = snakex[i-1];
        snakey[i] = snakey[i-1];
    }

    if (direction == 1) snakey[0]--;
    else if (direction == 2) snakey[0]++;
    else if (direction == 3) snakex[0]--;
    else if (direction == 4) snakex[0]++;
}

void render() {

    vga_clear();

    for(int i = 0; i < snakelength; i++) {
        if (i == 0) {
            put_block(snakex[i], snakey[i], LIGHT_GREEN, LIGHT_GREEN);
        } else {
            put_block(snakex[i], snakey[i], GREEN, GREEN);
        }
    }

    put_block(appleposx, appleposy, RED, RED);
}

char snake_logo[] =
"1111010001011110100101111\n"
"1000011001010010101001000\n"
"1111010101011110110001111\n"
"0001010011010010101001000\n"
"1111010001010010100101111\n";

void snake() {
    resetgame();
    update_cursor(0,0);
    logo(snake_logo);

    runningsnake = 1;

    timer_init();

    snakelength = 1;
    snakex[0] = 10;
    snakey[0] = 10;

    spawn_apple();

    while(runningsnake == 1) {

        handle_key();
        move();
        checktouch();
        render();

        delay_ms(100);
    }

    vga_clear();
}