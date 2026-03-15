#include <stdint.h>
#include "fs.c"
#include "snake.h"
#include "asm.h"
#include "vga.h"
#include "keyboard.h"
#include "timer.h"
#include "rtc.h"
#include "idt.h"
#include "gdt.h"
#include "piezo.h"
#include "fs/ata/ata.h"
#include "mouse.h"
#include "fs/fat/ff.h"
#include "fs/fat/diskio.h"
#include "pci.h"
#include "pong.h"
#define CURSOR_SIZE 8
static FATFS fatfs;
#define version "0.0.4\n"
fs_t fs;
static pci_bus_t g_pci_bus;
static uint32_t cursor_bg[CURSOR_SIZE * CURSOR_SIZE];

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

//----Test Crash----
void test_crash_div0() {
    int a = 1;
    int b = 0;
    int c = a / b;
    (void)c;
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

void reboot() {
    outb(0x64, 0xFE);
    while(1) { __asm__ volatile ("hlt"); }
}

void check_command(char* command) {
    if (strcmp(command, "help") == 0) {
        vga_print("Commands:\n help\n clear\n color\n version\n ls\n cat\n write\n rm\n snake\n time\n date\n triangle\n triangle red\n triangle green\n triangle blue\n pci\n pci enumerate\n pong\n ontime\n");
    }
    else if (strcmp(command, "pong") == 0) {
        pong();
    }
    else if (strcmp(command, "clear") == 0) {
        vga_clear();
    }
    else if (strcmp(command, "testcrash") == 0) {
        test_crash_div0();
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
    else if (strcmp(command, "reboot") == 0) {
        reboot();
    }
    else if (startswith(command, "triangle")) {
        vga_clear();
        if (use_framebuffer == 1) {
            int x0 = 700, y0 = 50;
            int x1 = 650,  y1 = 150;
            int x2 = 750, y2 = 150;
            char *color_str = command + 9;
            uint32_t color = 0xFF0000;
            if (strcmp(color_str, "red") == 0)        color = RED;
            else if (strcmp(color_str, "green") == 0) color = GREEN;
            else if (strcmp(color_str, "blue") == 0)  color = BLUE;
            else                                       color = 0xFFFFFF;
            fb_draw_triangle_outline_thick(x0, y0, x1, y1, x2, y2, color, 5);
        } else {
            vga_print("You don't use Modern Framebuffer!\n");
        }
    }
    else if (strcmp(command, "pci") == 0) {
        pci_list_devices(&g_pci_bus);
    }
    else if (strcmp(command, "pci enumerate") == 0) {
        pci_enumerate(&g_pci_bus);
    }
    else if (strcmp(command, "time") == 0) {
        int h, m, s;
        char hs[3], ms[3], ss[3];
        rtc_get_time(&h, &m, &s);
        itoa2(h, hs); itoa2(m, ms); itoa2(s, ss);
        vga_print(hs); vga_print(":"); vga_print(ms); vga_print(":"); vga_print(ss); vga_print("\n");
        delay_ms(100);
    }
    else if (strcmp(command, "date") == 0) {
        int day, month, year;
        char days[4], months[4], years[6];
        rtc_get_date(&day, &month, &year);
        itoa2(day, days); itoa2(month, months); itoa2(year, years);
        vga_print(days); vga_print("."); vga_print(months); vga_print("."); vga_print(years); vga_print("\n");
    }
    else if (strcmp(command, "version") == 0) {
        vga_print(version);
    }
    else if (startswith(command, "ls")) {
        fs_list(vga_print);
    }
    else if (startswith(command, "cat ")) {
        char* path = command + 4;
        static uint8_t cat_buf[4096];
        int n = fs_read(path, cat_buf, sizeof(cat_buf) - 1);
        if (n >= 0) {
            cat_buf[n] = 0;
            vga_print((char*)cat_buf);
            vga_putc('\n');
        } else {
            vga_print("File not found!\n");
        }
    }
    else if (startswith(command, "write ")) {
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
        if (fs_write(path, (uint8_t*)content, kstrlen(content)) == 0)
            vga_print("File written!\n");
        else
            vga_print("Write failed!\n");
    }
    else if (strcmp(command, "ontime") == 0) {
        int timesince = ms_since_startup();
        int timesinces = timesince / 1000;
        char timesincesStr[500];
        itoa(timesinces, timesincesStr);
        vga_print(timesincesStr);
        vga_print("s since startup\n");
    }
    else if (startswith(command, "rm ")) {
        char* path = command + 3;
        if (fs_delete(path) == 0)
            vga_print("File deleted!\n");
        else {
            vga_print("File not found!\n");
            vga_print(path); vga_print("\n");
        }
    }
    else if (strcmp(command, "") != 0) {
        vga_print("Command not found!\n");
        vga_print("Trying to run User Space Program\n");
        run_program(command);
    }
}

//---------------------Mouse---------------------
static int last_x = -1;
static int last_y = -1;

void xor_cursor(int x, int y)
{
    for (int dy = 0; dy < CURSOR_SIZE; dy++) {
        for (int dx = 0; dx <= dy; dx++) {

            int px = x + dx;
            int py = y + dy;

            if (px >= fb_width || py >= fb_height)
                continue;

            uint32_t *pixel =
                (uint32_t*)(fb_addr + py * fb_pitch + px * 4);

            *pixel ^= 0x00FFFFFF;
        }
    }
}

void ps2mouse_poll() {
    uint8_t status = inb(0x64);
    if (!(status & 1)) return;
    if (!(status & (1 << 5))) return;
    uint8_t data = inb(0x60);
    static uint8_t mouse_cycle = 0;
    static uint8_t mouse_packet[3];
    mouse_packet[mouse_cycle++] = data;
    if (mouse_cycle < 3) return;
    mouse_cycle = 0;
    if (!(mouse_packet[0] & 0x08)) return;
    int dx = (int8_t)mouse_packet[1];
    int dy = (int8_t)mouse_packet[2];
    mouse.x += dx;
    mouse.y -= dy;
}

void update_mouse() {
    if(mouse.x == last_x && mouse.y == last_y) {
        return;
    }
    //Revert Mouse
    xor_cursor(last_x, last_y);
    //Draw New One
    xor_cursor(mouse.x, mouse.y);
    //Save Pos
    last_x = mouse.x;
    last_y = mouse.y;
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
    vga_print("2");
    vga_print("3");
    if (use_framebuffer) {
        vga_print("mouse init\n");
        ps2mouse_init();
        mouse.x = fb_width / 2;
        mouse.y = fb_height / 2;
        vga_print("mouse init done\n");
    }
    gdt_init();
    vga_print("Init IDT");
    idt_init();
    vga_print("Init IDT Done");
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
    char lastcommand[128];
    int cmd_index = 0;
    int shift_pressed = 0;
    char c;
    while(1) {
        vga_print("> ");
        while(1) {
            update_cursor(cursor_x, cursor_y);
            if (use_framebuffer) {
                ps2mouse_poll();
                update_mouse();
            }
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
                    for (int i = 0; i < 128; i++)
                        command[i] = 0;
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
