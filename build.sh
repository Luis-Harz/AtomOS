#!/bin/bash
set -e

echo "[1/5] Compile kernel..."
gcc -m32 -ffreestanding -fno-pie -fno-stack-protector -nostdlib -c kernel.c -o kernel.o

echo "[2/5] Link kernel..."
ld -m elf_i386 -T linker.ld -o kernel.elf kernel.o

echo "[3/5] Create ISO structure..."
mkdir -p iso/boot/grub
cp kernel.elf iso/boot/
cp grub.cfg iso/boot/grub/

echo "[4/5] Create ISO..."
grub-mkrescue -o TinyOS.iso iso

echo "[5/5] Done!"
echo "Run with: qemu-system-i386 -cdrom TinyOS.iso"
