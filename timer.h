#ifndef TIMER_H
#define TIMER_H

#include "keyboard.h"
#include <stdint.h>

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

uint64_t ms_since_startup(void);
void timer_init(void);
void delay_ms(uint32_t ms);

#endif