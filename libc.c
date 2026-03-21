#include <stdint.h>
#include "ports.h"

void *memset(void *dst, int c, uint32_t n) {
    uint8_t *d = dst;
    while (n--) *d++ = (uint8_t)c;
    return dst;
}

//reboot
void reboot() {
    outb(0x64, 0xFE);
    while(1) { __asm__ volatile ("hlt"); }
}

void *memcpy(void *dst, const void *src, uint32_t n) {
    uint8_t *d = dst;
    const uint8_t *s = src;
    while (n--) *d++ = *s++;
    return dst;
}

int memcmp(const void *a, const void *b, uint32_t n) {
    const uint8_t *p = a, *q = b;
    while (n--) {
        if (*p != *q) return *p - *q;
        p++; q++;
    }
    return 0;
}

char *strchr(const char *s, int c) {
    while (*s) {
        if (*s == (char)c) return (char *)s;
        s++;
    }
    return (c == 0) ? (char *)s : 0;
}

int startswith(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

int kstrlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int contains(const char* str, const char* substr) {
    for (; *str; str++) {
        const char* s1 = str;
        const char* s2 = substr;
        while (*s1 && *s2 && (*s1 == *s2)) { s1++; s2++; }
        if (*s2 == 0) return 1;
    }
    return 0;
}

int strcmp(const char* a, const char* b) {
    while(*a && (*a == *b)) { a++; b++; }
   return *a - *b;
}