#ifndef LIBC_H
#define LIBC_H
#include <stdint.h>

int strcmp(const char* a, const char* b);
int contains(const char* str, const char* substr);
int kstrlen(const char* str);
int startswith(const char *str, const char *prefix);
char *strchr(const char *s, int c);
int memcmp(const void *a, const void *b, uint32_t n);
void *memcpy(void *dst, const void *src, uint32_t n);
void *memset(void *dst, int c, uint32_t n);
void reboot();

#endif