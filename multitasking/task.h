#pragma once
#include <stdint.h>

#define MAX_TASKS 4
#define STACK_SIZE 4096
extern int multitasking_active;

typedef struct {
    uint32_t esp;
    int active;
} task_t;

extern task_t tasks[MAX_TASKS];
extern int current_task;

void task_init();
void create_task(void (*func)(), int id);
void start_tasks();
void scheduler_tick();