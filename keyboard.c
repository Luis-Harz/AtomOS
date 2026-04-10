#include "keyboard.h"

char scancodes_lower_en[256] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0','-','=','\b',
    '\t','q','w','e','r','t','y','u','i','o','p','[',']','\n',0,
    'a','s','d','f','g','h','j','k','l',';','\'','`',0,'\\','z',
    'x','c','v','b','n','m',',','.','/',0,'*',0,' ','\0'
};

char scancodes_upper_en[256] = {
    0, 27, '!', '@', '#', '$', '%', '^', '&', '*', '(', ')', '_', '+', '\b',
    '\t','Q','W','E','R','T','Y','U','I','O','P','{','}','\n',0,
    'A','S','D','F','G','H','J','K','L',':','"','~',0,'|','Z',
    'X','C','V','B','N','M','<','>','?',0,'*',0,' ','\0'
};

unsigned char scancodes_lower_de[256] = {
    0, 27, '1','2','3','4','5','6','7','8','9','0',0xDF,0xB4,'\b', // ß=0xDF, ´=0xB4
    '\t','q','w','e','r','t','z','u','i','o','p',0xFC,'+','\n',0, // ü=0xFC
    'a','s','d','f','g','h','j','k','l',0xF6,0xE4,'^',0,'#','y',   // ö=0xF6, ä=0xE4
    'x','c','v','b','n','m',',','.','-','\0','*',0,' ','\0'
};

unsigned char scancodes_upper_de[256] = {
    0, 27, '!','"','\xA7','$','%','&','/','(',')','=','?','`','\b', // §=0xA7
    '\t','Q','W','E','R','T','Z','U','I','O','P',0xDC,'*','\n',0,   // Ü=0xDC
    'A','S','D','F','G','H','J','K','L',0xD6,0xC4,0xB0,0,'\'','Y', // Ö=0xD6, Ä=0xC4, °=0xB0
    'X','C','V','B','N','M',';',':','_','\0','*',0,' ','\0'
};

unsigned char *scancodes_lower[2] = {scancodes_lower_en, scancodes_lower_de};
unsigned char *scancodes_upper[2] = {scancodes_upper_en, scancodes_upper_de};

void itoa(int num, char *str) {
    int i = 0, is_negative = 0;
    if (num == 0) { str[i++] = '0'; str[i] = 0; return; }
    if (num < 0)  { is_negative = 1; num = -num; }
    while (num != 0) { str[i++] = (num % 10) + '0'; num /= 10; }
    if (is_negative) str[i++] = '-';
    str[i] = 0;
    int start = 0, end = i - 1;
    while (start < end) {
        char tmp = str[start]; str[start] = str[end]; str[end] = tmp;
        start++; end--;
    }
}

void itoa_pad(int num, char *str, int width) {
    int i = 0, is_negative = 0;

    if (num == 0) {
        str[i++] = '0';
    } else {
        if (num < 0) {
            is_negative = 1;
            num = -num;
        }

        while (num != 0) {
            str[i++] = (num % 10) + '0';
            num /= 10;
        }
    }
    while (i < width) {
        str[i++] = '0';
    }

    if (is_negative) {
        str[i++] = '-';
    }

    str[i] = 0;

    int start = 0, end = i - 1;
    while (start < end) {
        char tmp = str[start];
        str[start] = str[end];
        str[end] = tmp;
        start++;
        end--;
    }
}

uint8_t atoi_uint8(const char *str) {
    uint32_t result = 0; 
    int i = 0;
    int base = 10;
    if (str[0] == '0' && (str[1] == 'x' || str[1] == 'X')) {
        base = 16;
        i = 2;
    }
    for (; str[i] != '\0'; i++) {
        char c = str[i];
        uint8_t val;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (base == 16 && c >= 'A' && c <= 'F') val = c - 'A' + 10;
        else if (base == 16 && c >= 'a' && c <= 'f') val = c - 'a' + 10;
        else break;
        result = result * base + val;
        if (result > 255) result = 255;
    }
    return (uint8_t)result;
}

int atoi(const char *str) {
    int result = 0;
    int sign = 1;
    int i = 0;
    if (str[0] == '-') {
        sign = -1;
        i = 1;
    }

    for (; str[i] != '\0'; i++) {
        char c = str[i];
        if (c >= '0' && c <= '9') {
            result = result * 10 + (c - '0');
        } else {
            break;
        }
    }

    return result * sign;
}

void itoa2(int num, char *str) {
    if (num < 10) { str[0] = '0'; str[1] = '0' + num; str[2] = 0; }
    else itoa(num, str);
}

uint8_t extended = 0;

uint16_t keyboard_read_scancode() {
    uint8_t status = inb(0x64);
    if (!(status & 1)) return 0;
    if (status & (1 << 5)) return 0;
    uint8_t sc = inb(0x60);
    if (sc == 0xE0) {
        extended = 1;
        return 0;
    }
    if (extended) {
        extended = 0;
        return 0xE000 | sc;
    }
    return sc;
}