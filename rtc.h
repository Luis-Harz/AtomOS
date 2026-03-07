#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "timer.h"
#include "keyboard.h"
#define CMOS_ADDRESS 0x70
#define CMOS_DATA    0x71

static inline uint8_t cmos_read(uint8_t reg)
{
    outb(CMOS_ADDRESS, reg);
    return inb(CMOS_DATA);
}

static inline int rtc_update_in_progress()
{
    outb(CMOS_ADDRESS, 0x0A);
    return (inb(CMOS_DATA) & 0x80);
}

static inline int bcd_to_int(uint8_t bcd)
{
    return ((bcd >> 4) * 10) + (bcd & 0x0F);
}

static inline void rtc_get_time(int* hour, int* minute, int* second)
{
    uint8_t h, m, s, regB;

    while (rtc_update_in_progress());

    s = cmos_read(0x00);
    m = cmos_read(0x02);
    h = cmos_read(0x04);

    regB = cmos_read(0x0B);

    if (!(regB & 0x04)) {
        s = bcd_to_int(s);
        m = bcd_to_int(m);
        h = bcd_to_int(h);
    }

    if (!(regB & 0x02) && (h & 0x80)) {
        h = ((h & 0x7F) + 12) % 24;
    }

    *hour = h;
    *minute = m;
    *second = s;
}

static inline void rtc_get_date(int* day, int* month, int* year)
{
    uint8_t d, mo, y, regB;

    while (rtc_update_in_progress());

    d = cmos_read(0x07);
    mo = cmos_read(0x08);
    y = cmos_read(0x09);

    regB = cmos_read(0x0B);

    if (!(regB & 0x04)) {
        d = bcd_to_int(d);
        mo = bcd_to_int(mo);
        y = bcd_to_int(y);
    }

    *day = d;
    *month = mo;
    *year = 2000 + y;
}

#endif