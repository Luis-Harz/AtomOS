#include <stdint.h>
#include "fs.c"
volatile uint16_t* vga_buffer = (uint16_t*)0xB8000;
#define KEYBOARD_DATA 0x60
#define KEYBOARD_STATUS 0x64
#define VGA_WIDTH 80
#define VGA_HEIGHT 25
int cursor_x = 0;
int cursor_y = 0;
#define FULL_BLOCK 0xDB
uint8_t color = 0x0F;
#define MULTIBOOT_MAGIC  0x1BADB002
#define MULTIBOOT_FLAGS 0x00000001
#define MULTIBOOT_CHECKSUM  -(MULTIBOOT_MAGIC + MULTIBOOT_FLAGS)
#define BLACK       0x0
#define BLUE        0x1
#define GREEN       0x2
#define CYAN        0x3
#define RED         0x4
#define MAGENTA     0x5
#define BROWN       0x6
#define LIGHT_GRAY  0x7
#define DARK_GRAY   0x8
#define LIGHT_BLUE  0x9
#define LIGHT_GREEN 0xA
#define LIGHT_CYAN  0xB
#define LIGHT_RED   0xC
#define LIGHT_MAGENTA 0xD
#define YELLOW      0xE
#define WHITE       0xF
#define version "0.0.1\n"
uint8_t fs_buffer[FS_IMAGE_SIZE];
fs_t fs;

__attribute__((section(".multiboot")))
__attribute__((used))
unsigned int multiboot_header[] = {
    MULTIBOOT_MAGIC,
    MULTIBOOT_FLAGS,
    MULTIBOOT_CHECKSUM
};


static inline void outb(uint16_t port, uint8_t val) {
    __asm__ volatile ("outb %0, %1" : : "a"(val), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    __asm__ volatile ("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint64_t rdtsc(void) {
    uint32_t lo, hi;
    __asm__ volatile ("rdtsc" : "=a"(lo), "=d"(hi));
    return ((uint64_t)hi << 32) | lo;
}

static uint32_t tsc_ticks_per_ms = 0;

static void tsc_calibrate_pit(void) {
    uint8_t val = (inb(0x61) & 0xFD) | 0x01;
    outb(0x61, val);
    outb(0x43, 0b10110010);
    outb(0x42, 0xDC);
    outb(0x42, 0x2E);
    uint64_t start = rdtsc();
    while (!(inb(0x61) & 0x20));
    uint64_t end = rdtsc();
    uint32_t ticks_10ms = (uint32_t)(end - start);
    tsc_ticks_per_ms = ticks_10ms / 10;
}

static void tsc_calibrate_cpuid(void) {
    uint32_t eax, ebx, ecx, edx;
    __asm__ volatile (
        "cpuid"
        : "=a"(eax), "=b"(ebx), "=c"(ecx), "=d"(edx)
        : "a"(0x15)
    );
    if (!eax || !ebx || !ecx)
        return;
    uint32_t ratio = ebx / eax;
    uint32_t freq_khz = (ecx / 1000);
    tsc_ticks_per_ms = freq_khz * ratio;
}

void timer_init(void) {
    tsc_calibrate_cpuid();
    if (!tsc_ticks_per_ms)
        tsc_calibrate_pit();
}

void update_cursor(int x, int y) {
    uint16_t pos = y * VGA_WIDTH + x;
    outb(0x3D4, 0x0F);
    outb(0x3D5, pos & 0xFF);
    outb(0x3D4, 0x0E);
    outb(0x3D5, (pos >> 8) & 0xFF);
}

void delay_ms(uint32_t ms) {
    uint64_t ticks = (uint64_t)tsc_ticks_per_ms * ms;
    uint64_t start = rdtsc();
    while ((rdtsc() - start) < ticks);
}

char scancode_to_ascii[256] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z',
    'x','c','v','b','n','m',',','.','/',0,'*',0,' ','\0'
};

char scancode_upper[256] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|','Z',
    'X','C','V','B','N','M','<','>','?',0,'*',0,' ','\0'
};

uint8_t keyboard_read_scancode() {
    while (!(inb(KEYBOARD_STATUS) & 1)) {
        
    }
    return inb(KEYBOARD_DATA);
}

int vga_used_lines() {
    for (int y = VGA_HEIGHT - 1; y >= 0; y--) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            uint16_t cell = vga_buffer[y * VGA_WIDTH + x];
            char ch = cell & 0xFF;
            if (ch != ' ') {
                return y + 1;
            }
        }
    }
    return 0;
}

void put_block(int x, int y, uint8_t fg, uint8_t bg) {
    uint16_t color = (bg << 4) | fg;
    vga_buffer[y * 80 + x] = ((uint16_t)color << 8) | FULL_BLOCK;
}

void vga_scroll() {
    for (int y = 0; y < VGA_HEIGHT - 1; y++)
        for (int x = 0; x < VGA_WIDTH; x++)
            vga_buffer[y * VGA_WIDTH + x] = vga_buffer[(y+1) * VGA_WIDTH + x];
    for (int x = 0; x < VGA_WIDTH; x++)
        vga_buffer[(VGA_HEIGHT-1) * VGA_WIDTH + x] = ((uint16_t)color << 8) | ' ';
    cursor_y = VGA_HEIGHT - 1;
}

void vga_backspace(char* command, int* cmd_index) {
    if (*cmd_index > 0) {
        if (cursor_x > 2) {
            cursor_x--;
        } else if (cursor_y > 0) {
            cursor_y--;
            cursor_x = VGA_WIDTH - 1;
        }
        (*cmd_index)--;
        command[*cmd_index] = 0;
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | ' ';
    }
}

void vga_putc(char c) {
    if (c == '\n') {
        cursor_x = 0;
        cursor_y++;
    } else {
        vga_buffer[cursor_y * VGA_WIDTH + cursor_x] = ((uint16_t)color << 8) | c;
        cursor_x++;
        if (cursor_x >= VGA_WIDTH) {
            cursor_x = 0;
            cursor_y++;
        }
    }

    if (cursor_y >= VGA_HEIGHT) {
        vga_scroll();
    }
}

void vga_print(const char* str) {
    while (*str) {
        vga_putc(*str++);
    }
}

void vga_clear() {
    uint16_t blank = ((uint16_t)color << 8) | ' ';

    for (int y = 0; y < VGA_HEIGHT; y++) {
        for (int x = 0; x < VGA_WIDTH; x++) {
            vga_buffer[y * VGA_WIDTH + x] = blank;
        }
    }

    cursor_x = 0;
    cursor_y = 0;
}

void colortest() {
    vga_clear();
    int y = 0;
    int x = 0;
    while(1) {
        if (y == VGA_HEIGHT - 1 && x == VGA_WIDTH) {
            break;
        } else {
            put_block(x, y, RED, RED);
        }
        if (y < VGA_HEIGHT) {
            y++;
        } else {
            y = 0;
            x++;
        }
    }
    delay_ms(1000);
    vga_clear();
    y = 0;
    x = 0;
    while(1) {
        if (y == VGA_HEIGHT - 1 && x == VGA_WIDTH) {
            break;
        } else {
            put_block(x, y, GREEN, GREEN);
        }
        if (y < VGA_HEIGHT) {
            y++;
        } else {
            y = 0;
            x++;
        }
    }
    delay_ms(1000);
    vga_clear();
    y = 0;
    x = 0;
    while(1) {
        if (y == VGA_HEIGHT - 1 && x == VGA_WIDTH) {
            break;
        } else {
            put_block(x, y, BLUE, BLUE);
        }
        if (y < VGA_HEIGHT) {
            y++;
        } else {
            y = 0;
            x++;
        }
    }
    delay_ms(1000);
    vga_clear();
    return;
}

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

int strlen(const char* str) {
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
        vga_print("Commands:\n help\n clear\n color\n version\n ls\n cat\n write\n rm\n");
    } 
    else if (strcmp(command, "clear") == 0) {
        vga_clear();
    } 
    else if (strcmp(command, "echo") == 0) {
        char toecho[128];
        int i = 0;
        echo(command, toecho, i);
        if (!contains(toecho, "\n")) {
            int len = strlen(toecho);
            toecho[len] = '\n';
            toecho[len + 1] = 0;
        }
        vga_print(toecho);
    } 
    else if (strcmp(command, "color") == 0) {
        colortest();
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

void logo() {
    vga_clear();
    int logo_width  = 25;
    int logo_height = 5;
    int start_x = (VGA_WIDTH - logo_width) / 2;
    int start_y = (VGA_HEIGHT - logo_height) / 2;
    cursor_x = start_x;
    cursor_y = start_y;
    for (char* p = tinyos_logo; *p; p++) {
        if (*p == '1') {
            vga_putc(219);
        } else if (*p == '0') {
            vga_putc(' ');
        } else if (*p == '\n') {
            cursor_x = start_x;
            cursor_y++;
        }
    }
    delay_ms(10000);
    vga_clear();
}

void kernel_main() {
    fs_init(&fs, fs_buffer, FS_IMAGE_SIZE);
    vga_clear();
    timer_init();
    update_cursor(0, 0);
    logo();
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