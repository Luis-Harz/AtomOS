#include "timer.h"
#include "ports.h"

static uint32_t tsc_ticks_per_ms = 0;

static void tsc_calibrate_pit(void) {
    outb(0x43, 0xB0);
    outb(0x42, 0xFF);
    outb(0x42, 0xFF);
    uint8_t val = (inb(0x61) & 0xFC) | 0x01;
    outb(0x61, val);
    uint64_t start = rdtsc();
    while (inb(0x61) & 0x20);
    while (!(inb(0x61) & 0x20));
    uint64_t end = rdtsc();
    uint32_t elapsed = (uint32_t)(end - start);
    tsc_ticks_per_ms = elapsed / 55;
    if (tsc_ticks_per_ms == 0)
        tsc_ticks_per_ms = 1000000;
}

static void tsc_calibrate_cpuid(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile ("cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx) : "a"(0x15));
    if (!eax || !ebx || !ecx) return;
    tsc_ticks_per_ms = (ecx / 1000) * (ebx / eax);
}

void timer_init(void) {
    tsc_calibrate_cpuid();
    if (!tsc_ticks_per_ms)
        tsc_calibrate_pit();
    if (!tsc_ticks_per_ms)
        tsc_ticks_per_ms = 1000000;
}

void delay_ms(uint32_t ms) {
    if (tsc_ticks_per_ms == 0) return;
    uint64_t ticks = (uint64_t)tsc_ticks_per_ms * ms;
    uint64_t start = rdtsc();
    while ((rdtsc() - start) < ticks);
}

uint64_t ms_since_startup(void) {
    uint64_t ticks = rdtsc();
    return ticks / tsc_ticks_per_ms;
}