#include "piezo.h"

void beep(uint16_t freq, uint16_t duration_ms) {
    uint16_t div = 1193180 / freq;
    outb(PIT_CTRL,  0xB6);
    outb(PIT_DATA2, div & 0xFF);
    outb(PIT_DATA2, (div >> 8) & 0xFF);
    uint8_t tmp = inb(SPEAKER_CTRL);
    if (tmp != (tmp | 3))
        outb(SPEAKER_CTRL, tmp | 3);
    delay_ms(duration_ms);
    outb(SPEAKER_CTRL, inb(SPEAKER_CTRL) & 0xFC);
}