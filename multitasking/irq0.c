#include "task.h"
#include "../ports.h"

void irq0_handler() {
    //scheduler_tick();
    outb(0x20, 0x20);
}