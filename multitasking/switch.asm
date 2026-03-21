global switch_task

extern tasks
extern current_task

%define TASK_SIZE 8

switch_task:
    pusha
    mov eax, [current_task]
    mov [tasks + eax*TASK_SIZE], esp

.next:
    inc eax
    cmp eax, 4
    jl .ok
    mov eax, 0

.ok:
    mov [current_task], eax
    mov esp, [tasks + eax*TASK_SIZE]

    popa
    ret

global irq0_stub
extern irq0_handler
extern tasks
extern current_task
extern multitasking_active

%define MAX_TASKS 4
%define TASK_SIZE 8

irq0_stub:
    cli
    pusha

    mov eax, [multitasking_active]
    test eax, eax
    jz .no_switch

    ; Save current task ESP
    mov eax, [current_task]
    mov [tasks + eax * TASK_SIZE], esp

    ; Simple toggle between 0 and 1
    xor eax, eax
    mov eax, [current_task]
    inc eax
    cmp eax, 2          ; only 2 tasks
    jl .ok
    xor eax, eax
.ok:
    mov [current_task], eax
    mov esp, [tasks + eax * TASK_SIZE]

.no_switch:
    call irq0_handler
    popa
    sti
    iret