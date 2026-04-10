#ifndef LIBC_H
#define LIBC_H
#include <stdint.h>
#include <stddef.h>
#include "vga.h"

int strcmp(const char* a, const char* b);
int contains(const char* str, const char* substr);
int kstrlen(const char* str);
int startswith(const char *str, const char *prefix);
char *strchr(const char *s, int c);
int memcmp(const void *a, const void *b, uint32_t n);
void *memcpy(void *dst, const void *src, uint32_t n);
void *memset(void *dst, int c, uint32_t n);
void reboot();
void kstrncpy(char* dest, const char* src, uint32_t max);
void str_pad_left(const char *src, char *dst, int width);
static inline uint16_t htons(uint16_t x) { return (x >> 8) | (x << 8); }
static inline uint16_t ntohs(uint16_t x) { return (x >> 8) | (x << 8); }
static inline uint32_t htonl(uint32_t x) { return ((x >> 24)) | ((x >> 8) & 0xFF00) | ((x & 0xFF00) << 8) | (x << 24); }
static inline uint32_t ntohl(uint32_t x) { return htonl(x); }
static void print_hex(uint32_t val, int digits) {
    char buf[9];
    buf[digits] = '\0';
    for (int i = digits - 1; i >= 0; i--) {
        int nibble = val & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
        val >>= 4;
    }
    vga_print(buf);
}

static void print_dec(size_t val) {
    char buf[20];
    int  i = 18;
    buf[19] = '\0';
    if (val == 0) { vga_print("0"); return; }
    while (val && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    vga_print(&buf[i + 1]);
}

#endif