#ifndef KEYBOARD_H
#define KEYBOARD_H

#include "ports.h"
#include <stdint.h>

#define KEYBOARD_DATA   0x60
#define KEYBOARD_STATUS 0x64

extern unsigned char *scancodes_lower[2];
extern unsigned char *scancodes_upper[2];

void itoa(int num, char *str);
int atoi(const char *str);
void itoa2(int num, char *str);
uint16_t keyboard_read_scancode(void);
void itoa_pad(int num, char *str, int width);
uint8_t atoi_uint8(const char *str);

#endif