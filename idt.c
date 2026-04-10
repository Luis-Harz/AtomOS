#include "idt.h"
//#include "multitasking/irq0.c"
#include "mouse.h"

struct idt_entry idt[256];
struct idt_ptr   idtp;

char crash_logo[] =
    "00000001\n"
    "000000101\n"
    "0000010101\n"
    "00001001001\n"
    "000100000001\n"
    "0010000100001\n"
    "01000000000001\n"
    "111111111111111\n";

void unhandled_irq() {
    outb(0xA0, 0x20);
    outb(0x20, 0x20);
}

__asm__(
    ".section .text\n\t"
    ".extern unhandled_irq\n\t"
    ".global unhandled_irq_stub\n\t"
    "unhandled_irq_stub:\n\t"
    "cli\n\t"
    "pusha\n\t"
    "call unhandled_irq\n\t"
    "popa\n\t"
    "sti\n\t"
    "iret\n\t"
);
extern void unhandled_irq_stub();

#define IRQ_STUB(num, handler) \
    extern void irq##num##_stub(); \
    __asm__( \
        ".global irq" #num "_stub\n\t" \
        "irq" #num "_stub:\n\t" \
        "cli\n\t" \
        "pusha\n\t" \
        "call " #handler "\n\t" \
        "popa\n\t" \
        "sti\n\t" \
        "iret\n\t" \
    );


void generic_crash_handler(struct interrupt_frame* frame) {
    vga_clear();
    logo_crash(crash_logo);
    for (int i = 0; i < 2; i++) {
        vga_print(" ");
    }
    vga_print("CPU CRASH!\n");
    char buf[9];
    for (int i = 7; i >= 0; i--) {
        int nib = (frame->ip >> (i*4)) & 0xF;
        buf[7-i] = nib < 10 ? '0'+nib : 'A'+(nib-10);
    }
    buf[8] = 0;
    vga_print(buf);
    while(1) { __asm__("hlt"); }
}

#define EXCEPTION_STUB(num) \
extern void isr##num(); \
__asm__( \
".global isr" #num "\n\t" \
"isr" #num ":\n\t" \
"cli\n\t" \
"push %%eax\n\t" \
"push %%ecx\n\t" \
"push %%edx\n\t" \
"push %%ebx\n\t" \
"push %%ebp\n\t" \
"push %%esi\n\t" \
"push %%edi\n\t" \
"lea 28(%%esp), %%eax\n\t" \
"push %%eax\n\t" \
"call generic_crash_handler\n\t" \
"hlt\n\t" \
);

void irq14_handler() {
    outb(0xA0, 0x20);  // EOI to slave PIC
    outb(0x20, 0x20);  // EOI to master PIC
}

extern void isr0();  extern void isr1();  extern void isr2();  extern void isr3();
extern void isr4();  extern void isr5();  extern void isr6();  extern void isr7();
extern void isr8();  extern void isr9();  extern void isr10(); extern void isr11();
extern void isr12(); extern void isr13(); extern void isr14(); extern void isr15();
extern void isr16(); extern void isr17(); extern void isr18(); extern void isr19();
extern void isr20(); extern void isr21(); extern void isr22(); extern void isr23();
extern void isr24(); extern void isr25(); extern void isr26(); extern void isr27();
extern void isr28(); extern void isr29(); extern void isr30(); extern void isr31();

void pic_remap() {
    outb(0x20, 0x11);
    outb(0xA0, 0x11);
    outb(0x21, 0x20);
    outb(0xA1, 0x28);
    outb(0x21, 0x04);
    outb(0xA1, 0x02);
    outb(0x21, 0x01);
    outb(0xA1, 0x01);
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
}

void idt_init() {
    __asm__("cli");
    outb(0x21, 0xFF);
    outb(0xA1, 0xFF);
    for (int i = 0; i < 256; i++)
        idt_set_gate(i, (uint32_t)unhandled_irq_stub, 0x08, 0x8E);

    idt_set_gate(0,  (uint32_t)isr0,  0x08, 0x8E);
    idt_set_gate(1,  (uint32_t)isr1,  0x08, 0x8E);
    idt_set_gate(2,  (uint32_t)isr2,  0x08, 0x8E);
    idt_set_gate(3,  (uint32_t)isr3,  0x08, 0x8E);
    idt_set_gate(4,  (uint32_t)isr4,  0x08, 0x8E);
    idt_set_gate(5,  (uint32_t)isr5,  0x08, 0x8E);
    idt_set_gate(6,  (uint32_t)isr6,  0x08, 0x8E);
    idt_set_gate(7,  (uint32_t)isr7,  0x08, 0x8E);
    idt_set_gate(8,  (uint32_t)isr8,  0x08, 0x8E);
    idt_set_gate(9,  (uint32_t)isr9,  0x08, 0x8E);
    idt_set_gate(10, (uint32_t)isr10, 0x08, 0x8E);
    idt_set_gate(11, (uint32_t)isr11, 0x08, 0x8E);
    idt_set_gate(12, (uint32_t)isr12, 0x08, 0x8E);
    idt_set_gate(13, (uint32_t)isr13, 0x08, 0x8E);
    idt_set_gate(14, (uint32_t)isr14, 0x08, 0x8E);
    idt_set_gate(15, (uint32_t)isr15, 0x08, 0x8E);
    idt_set_gate(16, (uint32_t)isr16, 0x08, 0x8E);
    idt_set_gate(17, (uint32_t)isr17, 0x08, 0x8E);
    idt_set_gate(18, (uint32_t)isr18, 0x08, 0x8E);
    idt_set_gate(19, (uint32_t)isr19, 0x08, 0x8E);
    idt_set_gate(20, (uint32_t)isr20, 0x08, 0x8E);
    idt_set_gate(21, (uint32_t)isr21, 0x08, 0x8E);
    idt_set_gate(22, (uint32_t)isr22, 0x08, 0x8E);
    idt_set_gate(23, (uint32_t)isr23, 0x08, 0x8E);
    idt_set_gate(24, (uint32_t)isr24, 0x08, 0x8E);
    idt_set_gate(25, (uint32_t)isr25, 0x08, 0x8E);
    idt_set_gate(26, (uint32_t)isr26, 0x08, 0x8E);
    idt_set_gate(27, (uint32_t)isr27, 0x08, 0x8E);
    idt_set_gate(28, (uint32_t)isr28, 0x08, 0x8E);
    idt_set_gate(29, (uint32_t)isr29, 0x08, 0x8E);
    idt_set_gate(30, (uint32_t)isr30, 0x08, 0x8E);
    idt_set_gate(31, (uint32_t)isr31, 0x08, 0x8E);
    //extern void irq0_stub();
    //idt_set_gate(0x20, (uint32_t)irq0_stub, 0x08, 0x8E);
    idt_load();
    pic_remap();
    outb(0xA1, 0xAF);

    __asm__("sti");
}