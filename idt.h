#ifndef IDT_H
#define IDT_H
#include <stdint.h>
#include "vga.h"

struct idt_entry {
    uint16_t base_low;
    uint16_t sel;
    uint8_t  always0;
    uint8_t  flags;
    uint16_t base_high;
} __attribute__((packed));

struct idt_ptr {
    uint16_t limit;
    uint32_t base;
} __attribute__((packed));

struct interrupt_frame {
    uint32_t ip;
    uint32_t cs;
    uint32_t flags;
    uint32_t sp;
    uint32_t ss;
};

extern struct idt_entry idt[256];
extern struct idt_ptr   idtp;
extern char crash_logo[];

void generic_crash_handler(struct interrupt_frame* frame);
void idt_init();

static inline void idt_set_gate(int n, uint32_t handler, uint16_t sel, uint8_t flags) {
    idt[n].base_low  = handler & 0xFFFF;
    idt[n].base_high = (handler >> 16) & 0xFFFF;
    idt[n].sel       = sel;
    idt[n].always0   = 0;
    idt[n].flags     = flags;
}

static inline void idt_load() {
    idtp.limit = sizeof(idt) - 1;
    idtp.base  = (uint32_t)&idt;
    __asm__ volatile("lidt %0" : : "m"(idtp));
}

#endif