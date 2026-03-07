#include "keyboard.h"

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

void itoa2(int num, char *str) {
    if (num < 10) { str[0] = '0'; str[1] = '0' + num; str[2] = 0; }
    else itoa(num, str);
}

uint8_t keyboard_read_scancode(void) {
    while (!(inb(KEYBOARD_STATUS) & 1));
    return inb(KEYBOARD_DATA);
}