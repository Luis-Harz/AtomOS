#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "ports.h"
#include <stdint.h>

#define KEYBOARD_DATA   0x60
#define KEYBOARD_STATUS 0x64

extern char scancode_to_ascii[256];
extern char scancode_upper[256];

void itoa(int num, char *str);
void itoa2(int num, char *str);
uint8_t keyboard_read_scancode(void);

#endif