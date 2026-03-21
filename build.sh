#!/bin/bash
set -e

CFLAGS_KERNEL="-m32 -ffreestanding -mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-avx2 -fno-pie -fno-stack-protector -nostdlib -fno-builtin-memset"
CFLAGS_OTHER="-m32 -ffreestanding -mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-avx2 -fno-pie -fno-stack-protector -nostdlib -fno-builtin-memset"
echo "[1/6] Clear build/"
rm -rf build/*

echo "[2/6] Assemble boot stub..."
nasm -f elf32 boot.asm -o build/boot.o

echo "[3/6] Compile kernel..."
gcc $CFLAGS_KERNEL -c kernel.c        -o build/kernel.o
gcc $CFLAGS_OTHER  -c timer.c         -o build/timer.o
gcc $CFLAGS_OTHER  -c piezo.c         -o build/piezo.o
gcc $CFLAGS_OTHER  -c font_data.c     -o build/font_data.o
gcc $CFLAGS_OTHER  -c vga.c           -o build/vga.o
gcc $CFLAGS_OTHER  -c fs/fat/ff.c          -o build/ff.o
gcc $CFLAGS_OTHER  -c fs/fat/ffsystem.c    -o build/ffsystem.o
gcc $CFLAGS_OTHER  -c fs/fat/ffunicode.c   -o build/ffunicode.o
gcc $CFLAGS_OTHER  -c diskio_impl.c   -o build/diskio.o
gcc $CFLAGS_OTHER  -c libc.c          -o build/libc.o
gcc $CFLAGS_OTHER  -c fs/ata/ata.c    -o build/ata.o
gcc $CFLAGS_OTHER  -c keyboard.c    -o build/keyboard.o
gcc $CFLAGS_OTHER  -c snake.c    -o build/snake.o
gcc $CFLAGS_OTHER  -c pong.c    -o build/pong.o
gcc $CFLAGS_OTHER  -c idt.c    -o build/idt.o
gcc $CFLAGS_OTHER  -c gdt.c    -o build/gdt.o
gcc $CFLAGS_OTHER  -c mouse.c    -o build/mouse.o
gcc $CFLAGS_OTHER  -c multitasking/task.c    -o build/task.o
as --32 isr_stubs.s -o build/isr_stubs.o

echo "[3/6] also Assemble ASM"
nasm -f elf32 multitasking/switch.asm -o build/switch_mt.o

echo "[4/6] Link kernel..."
gcc -m32 -ffreestanding -nostdlib -o kernel.elf -T linker.ld \
    build/boot.o build/kernel.o build/timer.o build/piezo.o build/font_data.o build/vga.o \
    build/ff.o build/ffsystem.o build/ffunicode.o build/diskio.o build/libc.o build/ata.o build/keyboard.o \
    build/snake.o build/pong.o build/idt.o build/isr_stubs.o build/gdt.o build/mouse.o build/task.o \
    build/switch_mt.o -lgcc

echo "[5/6] Create ISO..."
mkdir -p iso/boot/grub
cp kernel.elf iso/boot/
cp grub.cfg iso/boot/grub/
grub-mkrescue -o TinyOS.iso iso

echo "[6/6] Done!"
