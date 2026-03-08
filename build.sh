#!/bin/bash
set -e

CFLAGS_KERNEL="-m32 -ffreestanding -mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-avx2 -fno-pie -fno-stack-protector -nostdlib -fno-builtin-memset"
CFLAGS_OTHER="-m32 -ffreestanding -mno-sse -mno-sse2 -mno-mmx -mno-avx -mno-avx2 -fno-pie -fno-stack-protector -nostdlib -fno-builtin-memset"

echo "[1/5] Assemble boot stub..."
nasm -f elf32 boot.asm -o boot.o

echo "[2/5] Compile kernel..."
gcc $CFLAGS_KERNEL -c kernel.c        -o kernel.o
gcc $CFLAGS_OTHER  -c timer.c         -o timer.o
gcc $CFLAGS_OTHER  -c piezo.c         -o piezo.o
gcc $CFLAGS_OTHER  -c font_data.c     -o font_data.o
gcc $CFLAGS_OTHER  -c vga.c           -o vga.o
gcc $CFLAGS_OTHER  -c fs/fat/ff.c          -o ff.o
gcc $CFLAGS_OTHER  -c fs/fat/ffsystem.c    -o ffsystem.o
gcc $CFLAGS_OTHER  -c fs/fat/ffunicode.c   -o ffunicode.o
gcc $CFLAGS_OTHER  -c diskio_impl.c   -o diskio.o
gcc $CFLAGS_OTHER  -c libc.c          -o libc.o
gcc $CFLAGS_OTHER  -c fs/ata/ata.c    -o ata.o
gcc $CFLAGS_OTHER  -c keyboard.c    -o keyboard.o

echo "[3/5] Link kernel..."
ld -m elf_i386 -z max-page-size=0x1 -T linker.ld -o kernel.elf \
    boot.o kernel.o timer.o piezo.o font_data.o vga.o \
    ff.o ffsystem.o ffunicode.o diskio.o libc.o ata.o keyboard.o

echo "[4/5] Create ISO..."
mkdir -p iso/boot/grub
cp kernel.elf iso/boot/
cp grub.cfg iso/boot/grub/
grub-mkrescue -o TinyOS.iso iso

echo "[5/5] Done!"
