#include "task.h"

task_t tasks[MAX_TASKS];
int current_task = 0;

uint8_t stacks[MAX_TASKS][STACK_SIZE];

extern void switch_task();

void task_init() {
    for (int i = 0; i < MAX_TASKS; i++) {
        tasks[i].active = 0;
        tasks[i].esp = 0;
    }
}

int multitasking_active = 0;

void create_task(void (*func)(), int id) {
    uint32_t *stk = (uint32_t*)(stacks[id] + STACK_SIZE);
    *(--stk) = 0x00000202;       // EFLAGS
    *(--stk) = 0x08;             // CS
    *(--stk) = (uint32_t)func;  // EIP
    for (int i = 0; i < 8; i++)
        *(--stk) = 0;
    tasks[id].esp = (uint32_t)stk;
    tasks[id].active = 1;
}

void scheduler_tick() {
    switch_task();
}

void start_tasks() {
    multitasking_active = 1;
    current_task = 0;
    __asm__ volatile(
        "mov %0, %%esp\n"
        "popa\n"
        "iret\n"
        :
        : "r"(tasks[0].esp)
        : "memory"
    );
}