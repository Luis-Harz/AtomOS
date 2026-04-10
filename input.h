#ifndef INPUT_H
#define INPUT_H
#include "keyboard.h"
#include "kernel.h"
#include <stdint.h>
#include "vga.h"
void input(char *input, int max_len) {
    char command[128];
    int cmd_index = 0;
    char c;
    int keyboard_layout = get_keyboard_layout();
    while (1) {
        uint8_t scancode = keyboard_read_scancode();
        int shift_pressed = 0;
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
        } else if (scancode == 0xAA || scancode == 0xB6) { 
            shift_pressed = 0;
        }
        if (shift_pressed == 0) {
            c = scancodes_lower[keyboard_layout][scancode];
        } else {
            c = scancodes_upper[keyboard_layout][scancode];
        }
        if (c) {
            if (c == '\n') {
                vga_putc(c);
                command[cmd_index] = 0;
                cmd_index = 0;
                for (int i = 0; i < max_len - 1; i++) {
                    input[i] = command[i];
                }
                for (int i = 0; i < 128; i++)
                    command[i] = 0;
                break;
            } else if (c == '\b') {
                vga_backspace(command, &cmd_index);
            } else {
                vga_putc(c);
                if (cmd_index < 127) {
                    command[cmd_index++] = c;
                }
            }
        }
    }
}
#endif