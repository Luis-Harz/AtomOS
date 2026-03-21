#ifndef INPUT_H
#define INPUT_H
#include "keyboard.h"
#include <stdint.h>
#include "vga.h"
void input(char *input) {
    char command[128];
    int cmd_index = 0;
    char c;
    while (1) {
        uint8_t scancode = keyboard_read_scancode();
        int shift_pressed = 0;
        if (scancode == 0x2A || scancode == 0x36) {
            shift_pressed = 1;
        } else if (scancode == 0xAA || scancode == 0xB6) { 
            shift_pressed = 0;
        }
        if (shift_pressed == 0) {
            c = scancode_to_ascii[scancode];
        } else {
            c = scancode_upper[scancode];
        }
        if (c) {
            if (c == '\n') {
                vga_putc(c);
                command[cmd_index] = 0;
                cmd_index = 0;
                for (int i = 0; i < 127; i++) {
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