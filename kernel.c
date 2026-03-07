#include <stdint.h>
#include "fs.c"
#include "snake.c"
#include "asm.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "rtc.h"
#include "piezo.h"
#include "fs/ata/ata.h"
#include "fs/fat/ff.h"
#include "fs/fat/diskio.h"
#include "pci.h"
static FATFS fatfs;
#define version "0.0.2\n"
uint8_t fs_buffer[FS_IMAGE_SIZE];
fs_t fs;
static pci_bus_t g_pci_bus;

//PCI
static void print_hex(u32 val, int digits)
{
    char buf[9];
    buf[digits] = '\0';
    for (int i = digits - 1; i >= 0; i--) {
        int nibble = val & 0xF;
        buf[i] = nibble < 10 ? '0' + nibble : 'a' + nibble - 10;
        val >>= 4;
    }
    vga_print(buf);
}

static void print_dec(size_t val)
{
    char buf[20];
    int  i = 18;
    buf[19] = '\0';
    if (val == 0) { vga_print("0"); return; }
    while (val && i >= 0) {
        buf[i--] = '0' + (val % 10);
        val /= 10;
    }
    vga_print(&buf[i + 1]);
}

void pci_list_devices(pci_bus_t *bus)
{
    vga_print("Found ");
    print_dec(bus->count);
    vga_print(" PCI device(s):\n\n");
    for (size_t i = 0; i < bus->count; i++) {
        pci_device_t *d = &bus->devices[i];
        vga_print("[");
        print_hex(d->bus,      2);
        vga_print(":");
        print_hex(d->device,   2);
        vga_print(".");
        print_hex(d->function, 1);
        vga_print("] ");
        vga_print("Vendor=");
        print_hex(d->vendor_id, 4);
        vga_print(" Device=");
        print_hex(d->device_id, 4);
        vga_print(" Class=");
        print_hex(d->class_code, 2);
        vga_print(":");
        print_hex(d->subclass,   2);
        vga_print(" IRQ=");
        print_dec(d->irq_line);
        vga_print("\n");
        for (int b = 0; b < PCI_MAX_BARS; b++) {
            if (d->bars[b].type == PCI_BAR_NONE) continue;

            vga_print("  BAR");
            print_dec(b);
            vga_print(" [");
            if      (d->bars[b].type == PCI_BAR_IO)    vga_print("I/O  ");
            else if (d->bars[b].type == PCI_BAR_MEM32) vga_print("MEM32");
            else                                        vga_print("MEM64");
            vga_print("] base=0x");
            print_hex((u32)(d->bars[b].base >> 32), 8);
            print_hex((u32)(d->bars[b].base),       8);
            vga_print(" size=0x");
            print_hex((u32)d->bars[b].size, 8);
            vga_print("\n");
        }
    }
}
//END PCI

int strcmp(const char* a, const char* b) {
    while(*a && (*a == *b)) { a++; b++; }
   return *a - *b;
}

void echo(char* command, char* toecho, int length) {
    int i = 5;
    int j = 0;
    int quote_count = 0;

    while (command[i] && j < 127) {
        if (command[i] == '"') {
            quote_count++;
            i++;
            if (quote_count == 2) break;
            continue;
        }
        toecho[j++] = command[i++];
    }
    length = j;
    toecho[j] = 0;
}

int startswith(const char *str, const char *prefix) {
    while (*prefix) {
        if (*str++ != *prefix++) return 0;
    }
    return 1;
}

int kstrlen(const char* str) {
    int len = 0;
    while (str[len]) len++;
    return len;
}

int contains(const char* str, const char* substr) {
    for (; *str; str++) {
        const char* s1 = str;
        const char* s2 = substr;
        while (*s1 && *s2 && (*s1 == *s2)) { s1++; s2++; }
        if (*s2 == 0) return 1;
    }
    return 0;
}

void check_command(char* command) {
    if (strcmp(command, "help") == 0) {
        vga_print("Commands:\n help\n clear\n color\n version\n ls\n cat\n write\n rm\n snake\n time\n date\n");
    } 
    else if (strcmp(command, "clear") == 0) {
        vga_clear();
    } 
    else if (startswith(command, "echo")) {
        char* toecho = command + 4;
        if (*toecho == '\0') {
            vga_print("\n");
        } else {
            if (*toecho == ' ') toecho++;
            vga_print(toecho);
            vga_putc('\n');
        }
    }
    else if (strcmp(command, "color") == 0) {
        colortest(delay_ms);
    }
    else if (strcmp(command, "snake") == 0) {
        snake();
    }
    else if (strcmp(command, "pci") == 0) {
        pci_list_devices(&g_pci_bus);
    }
    else if (strcmp(command, "time") == 0) {
        int h, m, s;
        char hs[3], ms[3], ss[3];
        rtc_get_time(&h, &m, &s);
        itoa2(h, hs);
        itoa2(m, ms);
        itoa2(s, ss);
        vga_print(hs);
        vga_print(":");
        vga_print(ms);
        vga_print(":");
        vga_print(ss);
        vga_print("\n");
        delay_ms(100);
    }
    else if (strcmp(command, "date") == 0) {
        int day, month, year;
        char days[4], months[4], years[6];
        rtc_get_date(&day, &month, &year);
        itoa2(day,days);
        itoa2(month, months);
        itoa2(year, years);
        vga_print(days);
        vga_print(".");
        vga_print(months);
        vga_print(".");
        vga_print(years);
        vga_print("\n");
    }
    else if (strcmp(command, "sync") == 0) {
        fs_compact(&fs);
        int r = fs_save(&fs);
        if (r == 0) vga_print("Saved OK\n");
        else vga_print("Save FAILED\n");
    }
    else if (strcmp(command, "version") == 0) {
        vga_print(version);
    } else if (startswith(command, "ls")) {
        fs_list(&fs, vga_print); 
    } else if (startswith(command, "cat ")) {
        char* path = command + 4;
        fs_file_t* f = fs_find(&fs, path);
        if (f) {
            vga_print(f->content);
            vga_putc('\n');
        } else {
            vga_print("File not found!\n");
        }
    } else if (startswith(command, "write ")) {
        char* rest = command + 6;
        char* space = rest;
        while (*space && *space != ' ') space++;
        if (*space == 0) {
            vga_print("Usage: write <path> <content>\n");
            return;
        }
        *space = 0;
        char* path = rest;
        char* content = space + 1;
        if (fs_write(&fs, path, content) == 0) {
            vga_print("File written!\n");
        } else {
            vga_print("Write failed, FS full!\n");
        }
    } else if (startswith(command, "rm ")) {
        char* path = command + 3;
        if (fs_delete(&fs, path) == 0) {
            vga_print("File deleted!\n");
        } else {
            vga_print("File not found!\n");
            vga_print(path);
            vga_print("\n");
        }
    } else if (strcmp(command, "") != 0) {
        vga_print("Command not found!\n");
    }
}

char tinyos_logo[] =
"1111100000000000111101111\n"
"0010001000000000100101000\n"
"0010000011101010100101111\n"
"0010001010101110100100001\n"
"0010001010100010111101111\n";

void kernel_main(uint32_t magic, uint32_t mb_addr) {
    multiboot_info_t *mb = (multiboot_info_t *)mb_addr;
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    vga[0] = 0x0F00 | 'A';
    vga[1] = 0x0F00 | 'B';
    vga[2] = 0x0F00 | 'C';
    vga_init(mb);
    vga_print("Screen Good!");
    timer_init();
    vga_clear();
    uint32_t fat_lba = ata_find_fat_partition();
    if (fat_lba == 0) vga_print("No FAT partition found!\n");
    else {
        vga_print("FAT partition at LBA: ");
        vga_print("\n");
    }
    FRESULT mr = f_mount(&fatfs, "0:", 0);
    if (mr == FR_OK) vga_print("FAT mount OK\n");
    else vga_print("FAT mount FAILED\n");
    vga_print("1");
    fs_init(&fs, fs_buffer, FS_IMAGE_SIZE);
    vga_print("2");
    int lr = fs_load(&fs);
    vga_print("3");
    if (lr == 0) vga_print("FS load OK\n");
    else vga_print("FS load failed (first boot?)\n");
    update_cursor(0, 0);
    vga_clear();
    logo(tinyos_logo);
    //static pci_bus_t bus = {0};
    vga_print("A\n");
    //cli();
    pci_enumerate(&g_pci_bus);
    //sti();
    vga_print("B\n");
    vga_clear();
    pci_list_devices(&g_pci_bus);
    vga_print("C\n");
    vga_print("Welcome to TinyOS!\n");
    vga_print("This is an OS written in C!\n");
    char command[128];
    int cmd_index = 0;
    int shift_pressed = 0;
    char c;
    while(1) {
        if (vga_used_lines() == VGA_HEIGHT) {
            vga_clear();
        }
        vga_print("> ");
        while(1) {
            update_cursor(cursor_x, cursor_y);
            uint8_t scancode = keyboard_read_scancode();
            if (scancode == 0x2A || scancode == 0x36) {
                shift_pressed = 1;
            } else if (scancode == 0xAA || scancode == 0xB6) { 
                shift_pressed = 0;
            }
            if (shift_pressed == 0) {
                c = scancode_to_ascii[scancode];
            } else {
                c = scancode_upper[scancode];
            }
            if (c) {
                if (c == '\n') {
                    vga_putc(c);
                    command[cmd_index] = 0;
                    cmd_index = 0;
                    check_command(command);
                    break;
                } else if (c == '\b') {
                    vga_backspace(command, &cmd_index);
                } else {
                    vga_putc(c);
                    if (cmd_index < 127) {
                        command[cmd_index++] = c;
                    }
                }
            }
        }
    }
}
