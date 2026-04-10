#include <stdint.h>
#include "libc.h"
#include <stdbool.h>
#include "multitasking.h"

#define MAX_TASKS 16
int running_task = -1;

Task tasks[MAX_TASKS];
int task_count = 0;

int add_task(task_func func, char* description) {
    if(task_count < MAX_TASKS) {
        tasks[task_count].func = func;
        tasks[task_count].active = true;
        kstrncpy(tasks[task_count].desc, description, 128);
        task_count++;
    }
    return task_count;
}

int check_task_existing_by_func(task_func func) {
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].func == func) {
            return 1;
        }
    }
    return 0;
}

int get_active_process_by_func(task_func function) {
    int found = 0;
    int process = -1;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].func == function && tasks[i].active == true) {found = 1; process = i;}
    }
    if (found == 1) {
        return process;
    } else {
        return -1;
    }
}

int get_process_by_func(task_func function) {
    int found = 0;
    int process = -1;
    for (int i = 0; i < task_count; i++) {
        if (tasks[i].func == function) {found = 1; process = i;}
    }
    if (found == 1) {
        return process;
    } else {
        return -1;
    }
}

int enable_task(int task_num) {
    if (task_num < task_count) {
        tasks[task_num].active = true;
        return 1;
    } else {
        return 0;
    }
}

int disable_task(int task_num) {
    if (task_num <= task_count) {
        tasks[task_num].active = false;
        return 1;
    } else {
        return 0;
    }
}

void run_scheduler() {
    while (1) {
        for (int i = 0; i < task_count; i++) {
            if (tasks[i].active) {
                tasks[i].func();
            }
        }
    }
}