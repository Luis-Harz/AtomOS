#ifndef PIEZO_H
#define PIEZO_H

#include <stdint.h>
#include "timer.h"

#define PIT_CTRL     0x43
#define PIT_DATA2    0x42
#define SPEAKER_CTRL 0x61

void beep(uint16_t freq, uint16_t duration_ms);

#endif