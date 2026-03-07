global _start
extern kernel_main
extern _bss_start
extern _bss_end

section .multiboot
align 4
dd 0x1BADB002
dd 0x00000007
dd -(0x1BADB002 + 0x00000007)
dd 0, 0, 0, 0, 0, 0, 0
dd 800
dd 600
dd 32

section .bootstrap_stack nobits alloc write
align 16
stack_bottom:
    resb 1048576
stack_top:

section .text
_start:
    mov  esp, stack_top

    ; Save multiboot values on stack BEFORE BSS zero wipes eax
    push ebx             ; multiboot info pointer
    push eax             ; multiboot magic

    ; Zero BSS (trashes eax, edi, ecx — but we saved what we need)
    mov  edi, _bss_start
    mov  ecx, _bss_end
    sub  ecx, edi
    shr  ecx, 2
    xor  eax, eax
    rep  stosd

    ; Args already on stack in correct order for kernel_main(magic, mb_addr)
    call kernel_main

    cli
.hang:
    hlt
    jmp .hang