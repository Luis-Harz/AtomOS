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
#include "rand.h"
#include "piezo.h"
#include "fs/ata/ata.h"
#include "login.h"
//#include "gui.h"
#include "libc.h"
#include "mouse.h"
#include "fs/fat/ff.h"
#include "memory.h"
#include "fs/fat/diskio.h"
#include "pci.h"
#include "kernel.h"
#include "multitasking.h"
#include "pong.h"
#include "com_rtl.h"
#include "input.h"
#define MAX_WINDOWS 7
#define SPACES 7
#define CURSOR_SIZE 8
static FATFS fatfs;
#define version "0.0.1\n"
fs_t fs;
static pci_bus_t g_pci_bus;
static uint32_t cursor_bg[CURSOR_SIZE * CURSOR_SIZE];
int desktopmode = 0;
int testmode = 0;
int win_pressed = 0;
int alt_pressed = 0;
int DHCP_DONE = 0;
int RTL_INIT = 0;
int keyboard_layout = 0; //0 = En; 1 = DE
pci_device_t *rtl;

//Logo
char atomos_logo[] =
"000100011111011111011111011111011111\n"
"001010000100010001010101010001010000\n"
"001110000100010001010001010001011111\n"
"010001000100010001010001010001000001\n"
"010001000100011111010001011111011111\n";


//Is Ip or Hostname
int is_ip(const char *str) {
    while (*str) {
        if (*str != '.' && (*str < '0' || *str > '9'))
            return 0;
        str++;
    }
    return 1;
}

//Multitasking
//#include "multitasking/task.h"
//#include "multitasking/pit.h"

//PCI

int startswith_number(char* input) {
    if (startswith(input, "0") || startswith(input, "1") || startswith(input, "2") || startswith(input, "3") || startswith(input, "4") || startswith(input, "5") || startswith(input, "6") || startswith(input, "7") || startswith(input, "8") || startswith(input, "9")) {
        return 1;
    }
    return 0;
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

//Calc
int calc(char input[128]) {
    int result = 0;
    int num = 0;
    int last = 0;
    char op = '+';

    for (int i = 0; input[i] != '\0'; i++) {
        char c = input[i];

        if (c >= '0' && c <= '9') {
            num = num * 10 + (c - '0');
        }

        if (c == '+' || c == '-' || c == '*' || c == '/' || input[i+1] == '\0') {

            if (op == '+') {
                result += last;
                last = num;
            } 
            else if (op == '-') {
                result += last;
                last = -num;
            } 
            else if (op == '*') {
                last = last * num;
            } 
            else if (op == '/') {
                last = last / num;
            }

            op = c;
            num = 0;
        }
    }

    result += last;
    return result;
}

void task1() {
    vga_print("Flip\n");
    delay_ms(100);
}

void task2() {
    vga_print("Flop\n");
    delay_ms(100);
}

//ASCII tools
void ascii(char tool[128]) {

}

typedef struct { int x, y; } vec2;
typedef struct { int x, y, z; } vec3;
int sin_table[360] = {
    0, 17, 35, 52, 70, 87, 105, 122, 139, 156, 174, 191, 208, 225, 242,
    258, 275, 292, 309, 325, 342, 358, 374, 390, 406, 422, 438, 453, 469, 484,
    500, 515, 530, 545, 559, 574, 588, 602, 616, 629, 643, 656, 669, 682, 695,
    707, 719, 731, 743, 754, 766, 777, 788, 798, 809, 819, 829, 838, 848, 857,
    866, 874, 882, 891, 898, 906, 913, 920, 927, 934, 940, 946, 951, 957, 962,
    966, 971, 975, 979, 982, 985, 988, 990, 992, 994, 996, 997, 998, 999, 999,
    1000, 999, 999, 998, 997, 996, 994, 992, 990, 988, 985, 982, 979, 975, 971,
    966, 962, 957, 951, 946, 940, 934, 927, 920, 913, 906, 898, 891, 882, 874,
    866, 857, 848, 838, 829, 819, 809, 798, 788, 777, 766, 754, 743, 731, 719,
    707, 695, 682, 669, 656, 643, 629, 616, 602, 588, 574, 559, 545, 530, 515,
    500, 484, 469, 453, 438, 422, 406, 390, 374, 358, 342, 325, 309, 292, 275,
    258, 242, 225, 208, 191, 174, 156, 139, 122, 105, 87, 70, 52, 35, 17,
    0, -17, -35, -52, -70, -87, -105, -122, -139, -156, -174, -191, -208, -225, -242,
    -258, -275, -292, -309, -325, -342, -358, -374, -390, -406, -422, -438, -453, -469, -484,
    -500, -515, -530, -545, -559, -574, -588, -602, -616, -629, -643, -656, -669, -682, -695,
    -707, -719, -731, -743, -754, -766, -777, -788, -798, -809, -819, -829, -838, -848, -857,
    -866, -874, -882, -891, -898, -906, -913, -920, -927, -934, -940, -946, -951, -957, -962,
    -966, -971, -975, -979, -982, -985, -988, -990, -992, -994, -996, -997, -998, -999, -999,
    -1000, -999, -999, -998, -997, -996, -994, -992, -990, -988, -985, -982, -979, -975, -971,
    -966, -962, -957, -951, -946, -940, -934, -927, -920, -913, -906, -898, -891, -882, -874,
    -866, -857, -848, -838, -829, -819, -809, -798, -788, -777, -766, -754, -743, -731, -719,
    -707, -695, -682, -669, -656, -643, -629, -616, -602, -588, -574, -559, -545, -530, -515,
    -500, -484, -469, -453, -438, -422, -406, -390, -374, -358, -342, -325, -309, -292, -275,
    -258, -242, -225, -208, -191, -174, -156, -139, -122, -105, -87, -70, -52, -35, -17
};

#define isin(a) sin_table[(a) % 360]
#define icos(a) sin_table[((a) + 90) % 360]

vec2 project(vec3 p) {
    vec2 out;
    float f = 200.0f;
    out.x = (p.x * f) / p.z + 160;
    out.y = (p.y * f) / p.z + 100;
    return out;
}

vec3 rotate_x(vec3 p, int angle, int cy, int cz) {
    int y = p.y - cy;
    int z = p.z - cz;
    vec3 out;
    out.x = p.x;
    out.y = (y * icos(angle) - z * isin(angle)) / 1024 + cy;
    out.z = (y * isin(angle) + z * icos(angle)) / 1024 + cz;
    return out;
}

vec3 rotate_y(vec3 p, int angle, int cx, int cz) {
    int x = p.x - cx;
    int z = p.z - cz;
    vec3 out;
    out.x = (x * icos(angle) - z * isin(angle)) / 1024 + cx;
    out.y = p.y;
    out.z = (x * isin(angle) + z * icos(angle)) / 1024 + cz;
    return out;
}

vec3 rotate_z(vec3 p, int angle, int cx, int cy) {
    int x = p.x - cx;
    int y = p.y - cy;
    vec3 out;
    out.x = (x * icos(angle) - y * isin(angle)) / 1024 + cx;
    out.y = (x * isin(angle) + y * icos(angle)) / 1024 + cy;
    out.z = p.z;
    return out;
}

void draw_cube(int cx, int cy, int cz, int size, int ax, int ay, int az) {
    vec3 verts[8] = {
        {cx,        cy,        cz       },
        {cx + size, cy,        cz       },
        {cx,        cy + size, cz       },
        {cx + size, cy + size, cz       },
        {cx,        cy,        cz + size},
        {cx + size, cy,        cz + size},
        {cx,        cy + size, cz + size},
        {cx + size, cy + size, cz + size},
    };

    int mcx = cx + size / 2;
    int mcy = cy + size / 2;
    int mcz = cz + size / 2;

    for (int i = 0; i < 8; i++) {
        verts[i] = rotate_x(verts[i], ax, mcy, mcz);
        verts[i] = rotate_y(verts[i], ay, mcx, mcz);
        verts[i] = rotate_z(verts[i], az, mcx, mcy);
    }

    vec2 p[8];
    for (int i = 0; i < 8; i++)
        p[i] = project(verts[i]);

    fb_draw_line(p[0].x, p[0].y, p[1].x, p[1].y, RED);
    fb_draw_line(p[1].x, p[1].y, p[3].x, p[3].y, RED);
    fb_draw_line(p[3].x, p[3].y, p[2].x, p[2].y, RED);
    fb_draw_line(p[2].x, p[2].y, p[0].x, p[0].y, RED);
    fb_draw_line(p[4].x, p[4].y, p[5].x, p[5].y, RED);
    fb_draw_line(p[5].x, p[5].y, p[7].x, p[7].y, RED);
    fb_draw_line(p[7].x, p[7].y, p[6].x, p[6].y, RED);
    fb_draw_line(p[6].x, p[6].y, p[4].x, p[4].y, RED);
    fb_draw_line(p[0].x, p[0].y, p[4].x, p[4].y, RED);
    fb_draw_line(p[1].x, p[1].y, p[5].x, p[5].y, RED);
    fb_draw_line(p[2].x, p[2].y, p[6].x, p[6].y, RED);
    fb_draw_line(p[3].x, p[3].y, p[7].x, p[7].y, RED);
}

void command_needs_sudo() {
    vga_print("This command needs sudo!\n");
}

void check_command(char* command, char* user, int sudo) {
    if (strcmp(command, "help") == 0) {
        vga_print("Commands:\n sudo {command}\n help\n clear\n colortest\n version\n ls\n cat\n write\n rm\n snake\n time\n date\n triangle\n triangle red\n triangle green\n triangle blue\n pci {enumerate}\n pong\n ontime\n multitaskingtest\n name {version, logo}\n disable num\n enable num\n ptop\n fetch\n color {uint8}\n");
    }
    else if (strcmp(command, "pong") == 0) {
        pong();
    }
    else if(strcmp(command, "test") == 0) {
        int ax = 0, ay = 0, az = 0;
        while (1) {
            vga_clear();
            draw_cube(140, 80, 400, 40, ax, ay, az);
            ax = (ax + 1) % 360;
            ay = (ay + 2) % 360;
            az = (az + 3) % 360;
            delay_ms(16);
        }
    }
    else if (startswith(command, "random ")) {
        char* seed = command + 7;
        int seedInt = atoi(seed);
        srand(seedInt);
        char buf[11];
        itoa(rand(), buf);
        vga_print(buf); vga_print("\n");
    }
    else if (startswith(command, "network ")) {
        char* arg = command + 8;
        if (strcmp(arg, "init") == 0) {
            rtl = pci_find_device(&g_pci_bus, 0x10EC, 0x8139);
            if (rtl) {
                vga_print("RTL8139 found!\n");
                if (cable_in()) {
                    vga_print("Connected!\n");
                    if (RTL_INIT == 0) {
                        rtl_init(rtl);
                        RTL_INIT = 1;
                    }
                    if (DHCP_DONE == 0) {
                        vga_print("[DHCP] Start\n");
                        dhcp_request(rtl);
                        vga_print("[DHCP] Done\n");
                        DHCP_DONE = 1;
                    }
                } else {
                    vga_print("Not Connected!\n");
                }
            } else {
                vga_print("no RTL8139 found!\n");
            }
        }
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
    else if (startswith(command, "disable ")) {
        if (sudo == 1) {
            char* num = command + 8;
            int numInt;
            numInt = atoi(num);
            int success = disable_task(numInt);
            if (success == 1) {
                vga_print("Successfully disabled Task\n");
            } else {
                vga_print("Failed to disabled Task\n");
            }
        } else {
            command_needs_sudo();
        }
    }
    else if (startswith(command, "enable ")) {
        if (sudo == 1) {
            char* num = command + 7;
            int numInt;
            numInt = atoi(num);
            int success = enable_task(numInt);
            if (success == 1) {
                vga_print("Successfully enabled Task\n");
            } else {
                vga_print("Failed to enable Task\n");
            }
        } else {
            command_needs_sudo();
        }
    }
    else if (strcmp(command, "colortest") == 0) {
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
            int x0 = 400, y0 = 25;
            int x1 = 350,  y1 = 100;
            int x2 = 450, y2 = 100;
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
    else if (startswith(command, "pci")) {
        if (kstrlen(command) > 3) {
            pci_enumerate(&g_pci_bus);
        } else {
            pci_list_devices(&g_pci_bus);
        }
    }
    else if (startswith(command, "ping ")) {
        char *ip = command + 5;
        uint8_t target[4] = {0, 0, 0, 0};
        if (is_ip(ip)) {
            int octet = 0;
            int val = 0;
            for (int i = 0; ip[i] != '\0' && octet < 4; i++) {
                if (ip[i] == '.') {
                    target[octet++] = (uint8_t)val;
                    val = 0;
                } else if (ip[i] >= '0' && ip[i] <= '9') {
                    val = val * 10 + (ip[i] - '0');
                }
            }
            target[octet] = (uint8_t)val;
        } else {
            dns_resolve(rtl, ip, target);
        }
        ping(rtl, target);
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
    else if (startswith(command, "user ")) {
        if (sudo == 1) {
            char* arg = command + 5;
            if (strcmp(arg, "create") == 0) {
                make_user();
            } else if (strcmp(arg, "delete") == 0) {
                delete_user();
            } else {
                vga_print("Usage: user <argument>\n Arguments:\n create\n delete\n");
            }
        } else {
            command_needs_sudo();
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
        if (content[0] == '"') content++;
        if (content[kstrlen(content)-1] == '"') content[kstrlen(content)-1] = '\0';
        if (fs_write(path, (uint8_t*)content, kstrlen(content)) == 0)
            vga_print("File written!\n");
        else
            vga_print("Write failed!\n");
    }
    else if (contains(command, "+") || contains(command, "-") || contains(command, "*") || contains(command, "/")) {
        if (startswith_number(command)) {
            vga_print("Result: ");
            char result[128];
            itoa(calc(command), result);
            vga_print(result);
            vga_print("\n");
        }
    }
    else if (startswith(command, "name")) {
        char* argument = command + 4;
        while (*argument == ' ') argument++;
        if (strcmp(argument, "") == 0) {
            vga_print("TinyOS "); vga_print("V"); vga_print(version);
            vga_print("made by Luis Harz\n");
        } else if (strcmp(argument, "version") == 0) {
            vga_print(version);
        } else if (strcmp(argument, "logo") == 0) {
            for (char *p = atomos_logo; *p; p++) {
                if (*p == '1')       vga_putc((char)219);
                else if (*p == '0')  vga_putc(' ');
                else if (*p == '\n') { cursor_x = 0; cursor_y++; }
            }
            vga_print("\n");
        }
    }
    else if (startswith(command, "cd ")) {
        char* arg = command + 3;
        if (fs_exists(arg) == 1) {
            int status = fs_cd(arg);
            if (status == 0) {
                vga_print("[SUCCESS]\n");
            } else {
                vga_print("[FAILED]\n");
            }
        } else {
            vga_print("Doesn't exist!\n");
        }
    }
    else if (startswith(command, "touch ")) {
        char* path = command + 6;
        if (fs_exists(path) != 1) {
            if (fs_write(path, (uint8_t*)"", 0) == 0)
                vga_print("File created\n");
            else
                vga_print("Failed\n");
        } else {
            vga_print("Already exists!\n");
        }
    }
    else if (startswith(command, "mkdir ")) {
        char* arg = command + 6;
        if (fs_exists(arg) != 1) {
            int status = fs_mkdir(arg);
            if (status == 0) {
                vga_print("[SUCCESS]\n");
            } else {
                vga_print("[FAILED]\n");
            }
        } else {
            vga_print("Already exists!\n");
        }
    }
    else if (strcmp(command, "fetch") == 0) {
        for (char *p = atomos_logo; *p; p++) {
            if (*p == '1')       vga_putc((char)219);
            else if (*p == '0')  vga_putc(' ');
            else if (*p == '\n') { cursor_x = 0; cursor_y++; }
        }
        vga_print("\n");
        int timesince = ms_since_startup();
        int timesinces = timesince / 1000;
        char timesincesStr[500];
        itoa(timesinces, timesincesStr);
        vga_print("Ontime: "); vga_print(timesincesStr);vga_print("s"); vga_print("\n");
        vga_print("User: "); vga_print(user); vga_print("\n");
        vga_print("Version: "); vga_print(version);
    }
    else if (strcmp(command, "ontime") == 0) {
        int timesince = ms_since_startup();
        int timesinces = timesince / 1000;
        char timesincesStr[500];
        itoa(timesinces, timesincesStr);
        vga_print(timesincesStr);
        vga_print("s since startup\n");
    }
    else if (strcmp(command, "multitaskingtest") == 0) {
        add_task(task1, "Test Task 1");
        add_task(task2, "Test Task 2");
    }
    else if (startswith(command, "nslookup ")) {
        char *hostname = command + 9;
        uint8_t ip[4];
        if (dns_resolve(rtl, hostname, ip)) {
            for (int i = 0; i < 4; i++) {
                char ipStr[4];
                itoa(ip[i], ipStr);
                vga_print(ipStr);
                if (i != 3) vga_print(".");
            }
            vga_print("\n");
        } else {
            vga_print("nslookup: could not resolve ");
            vga_print(hostname);
            vga_print("\n");
        }
    }
    else if (strcmp(command, "ptop") == 0) {
        vga_clear();
        vga_print("|ID  |                     Description|  ON/OFF|\n");
        char numbuff[16];
        if (task_count < VGA_HEIGHT) {
            for (int i = 0; i < task_count; i++) {
                itoa_pad(i, numbuff, 4);
                vga_print("|");
                vga_print(numbuff);
                vga_print("|");
                char taskdesc[32];
                str_pad_left(tasks[i].desc, taskdesc, 32);
                vga_print(taskdesc);
                vga_print("|");
                if (tasks[i].active == 1) {
                    vga_print(" Enabled");
                } else {
                    vga_print("Disabled");
                }
                vga_print("|");
                vga_print("\n");
            }
        } else {
            for (int i = 0; i < VGA_HEIGHT; i++) {
                itoa(i, numbuff);
                vga_print(numbuff);
                vga_print(" ");
                vga_print(tasks[i].desc);
                vga_print("\n");
            }
        }
    }
    else if (startswith(command, "exists ")) {
        char* path = command + 7;
        if (fs_exists(path))
            vga_print("Yes\n");
        else
            vga_print("No\n");
    }
    else if (startswith(command, "panic")) {
        if (sudo == 1) {
            char* reason = command + 6;
            panic(reason);
        } else {
            command_needs_sudo();
        }
    }
    else if (startswith(command, "passgen ")) {
        char* arg = command + 8;
        int len = atoi(arg);
        if (len > 30) {
            vga_print("Max length(30) exceeded!\n");
            return;
        }
        char passcharacters[68] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz!?.,-_0123456789";
        char* password = (char*)malloc((len));
        for (int i = 0; i <= (len - 1); i++) {
            password[i] = passcharacters[(rand() % len)];
        }
        password[len] = '\0';
        vga_print(password); vga_print("\n");
        free(password);
    }
    else if (startswith(command, "klayout ")) {
        char* arg = command + 8;
        if (strcmp(arg, "help") == 0) {
            vga_print("0 = EN\n1 = DE\n");
        } else {
            int layoutInt = atoi(arg);
            if (layoutInt < 2) {
                keyboard_layout = layoutInt;
            } else {
                vga_print("Invalid\n");
            }
        }
    }
    else if (startswith(command, "color ")) {
        char* color_in = command + 6;
        uint8_t color_in_int = atoi_uint8(color_in);
        change_color_vga(color_in_int);
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
        vga_print("run 'help' to see all commands\n");
        if (fs_exists(command) == 1) {
            vga_print("running executable\n");
            run_program(command);
        }
    }
}

void contains_sudo(char* command, char* user_now) {
    int can_sudo = 0;
    if (startswith(command, "sudo ")) {
        int status = authenticate();
        if (status == 1) {
            check_command(command + 5, user_now, 1);
        }
    } else {
        check_command(command, user_now, 0);
    }
}

//mouse
void mouse_update() {
    if (use_framebuffer) {
        ps2mouse_poll();
        update_mouse();
    }
}

//----------Shell----------
char command[128];
int cmd_index = 0;
int shift_pressed = 0;
char c;
int new_prompt = 0;
char GlobalUser[16];

void shell_init() {
    vga_print("---PCI---");
    vga_print("A\n");
    //cli();
    pci_enumerate(&g_pci_bus);
    //sti();
    vga_print("B\n");
    vga_clear();
    pci_list_devices(&g_pci_bus);
    vga_print("---LAN---\n");
    rtl = pci_find_device(&g_pci_bus, 0x10EC, 0x8139);
    if (rtl) {
        vga_print("RTL8139 found!\n");
        if (cable_in()) {
            vga_print("Connected!");
            rtl_init(rtl);
            RTL_INIT = 1;
            vga_print("[DHCP] Start\n");
            dhcp_request(rtl);
            vga_print("[DHCP] Done\n");
            DHCP_DONE = 1;
        } else {
            vga_print("Not Connected!");
        }
    } else {
        vga_print("no RTL8139 found!\n");
    }
    vga_print("C\n");
    vga_print("Welcome to AtomOS!\n");
    change_color_vga(GREEN);
    vga_print("You are on version "); vga_print(version);
    change_color_vga(WHITE);
    new_prompt = 1;
}

int get_keyboard_layout() {
    return keyboard_layout;
}

char cwd_buf[64];

void shell() {
    if (new_prompt == 1) {
        fs_getcwd(cwd_buf, sizeof(cwd_buf));
        vga_print(cwd_buf);vga_print(" "); vga_print(GlobalUser); vga_print("> ");
        new_prompt = 0;
    }
    update_cursor(cursor_x, cursor_y);
    uint16_t scancode = keyboard_read_scancode();
    if (scancode == 0x2A || scancode == 0x36) {
        shift_pressed = 1;
    } else if (scancode == 0xAA || scancode == 0xB6) { 
        shift_pressed = 0;
    }    
    if (shift_pressed == 0) {
        c = scancodes_lower[keyboard_layout][scancode];
    } else {
        c = scancodes_upper[keyboard_layout][scancode];
    }
    if (c) {
        if (c == '\n') {
            vga_putc(c);
            command[cmd_index] = 0;
            cmd_index = 0;
            new_prompt = 1;
            contains_sudo(command, GlobalUser);
            for (int i = 0; i < 128; i++)
                command[i] = 0;
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

void SysLogin() {
    if (testmode == 0) {
        char* loginstatus = login();
        if (strcmp(loginstatus, "0") != 0) {
            if (get_process_by_func(shell) == -1) {
                memcpy(GlobalUser, loginstatus, 16);
                add_task(shell, "User Shell");
            }
        } else {
            vga_print("too many times wrong. Rebooting...");
            delay_ms(500);
            reboot();
        }
    } else {
        if (get_process_by_func(shell) == -1) {
            memcpy(GlobalUser, "test", 16);
            add_task(shell, "User Shell");
        }
    }
    shell_init();
    disable_task(get_active_process_by_func(SysLogin));
}

//----------Kernel Main----------
void kernel_main(uint32_t magic, uint32_t mb_addr) {
    multiboot_info_t *mb = (multiboot_info_t *)mb_addr;
    volatile uint16_t *vga = (volatile uint16_t *)0xB8000;
    vga[0] = 0x0F00 | 'A';
    vga[1] = 0x0F00 | 'B';
    vga[2] = 0x0F00 | 'C';
    //Inits
    vga[0] = 0x0F00 | 'V';
    vga[1] = 0x0F00 | 'G';
    vga[2] = 0x0F00 | 'A';
    vga_init(mb);
    change_color_vga(GREEN);
    vga_print("Screen Good!");
    vga_print("---Init---\n");
    vga_clear();
    gdt_init();
    idt_init();
    timer_init();
    init_heap();
    //task_init();
    //pit_init(100);
    if (use_framebuffer) {
        vga_print("mouse init\n");
        ps2mouse_init();
        mouse.x = fb_width / 2;
        mouse.y = fb_height / 2;
        last_x = mouse.x;
        last_y = mouse.y;
        vga_print("mouse init done\n");
    }
    vga_print("---Activationg Tasks---\n");
    vga_print("Activating Mouse\n");
    add_task(mouse_update, "Basic mouse");
    add_task(SysLogin, "Login");
    uint32_t fat_lba = ata_find_fat_partition();
    if (fat_lba == 0) vga_print("No FAT partition found!\n");
    else vga_print("FAT partition found!\n");

    FRESULT mr = f_mount(&fatfs, "0:", 0);
    if (mr == FR_OK) vga_print("FAT mount OK\n");
    else vga_print("FAT mount FAILED\n");
    fs_cd("0:");
    update_cursor(0, 0);
    logo(atomos_logo);
    vga_print("---Loading Users---\n");
    load_users();
    vga_print("Loading Users Done\n");
    vga_print("Starting Multitasking Scheduler");
    change_color_vga(WHITE);
    run_scheduler();
}