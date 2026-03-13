#include "rtc.h"
#include <lcom/lcf.h>

#define TODO return -1

#define RTC_ADDR_REG 0x70
#define RTC_DATA_REG 0x71
#define RTC_REG_A 0x0A
#define RTC_REG_B 0x0B
#define RTC_REG_DAY 0x07
#define RTC_REG_MONTH 0x08
#define RTC_REG_YEAR 0x09
#define RTC_UIP_MSK (1 << 7)
#define RTC_DM_MSK (1 << 2)

static int bcd_to_bin(uint8_t bcd) {
    return ((bcd >> 4) * 10 + (bcd & 0x0F));
}

int rtc_read_date(rtc_date *date) {
    uint32_t regA, regB;
    do {
        sys_outb(RTC_ADDR_REG,RTC_REG_A);
        sys_inb(RTC_DATA_REG, &regA);
    } while (regA & RTC_UIP_MSK);

    sys_outb(RTC_ADDR_REG, RTC_REG_B);
    sys_inb(RTC_DATA_REG, &regB);
    bool is_binary = regB & RTC_DM_MSK;

    uint32_t day, month, year;
    sys_outb(RTC_ADDR_REG, RTC_REG_DAY); sys_inb(RTC_DATA_REG, &day);
    sys_outb(RTC_ADDR_REG, RTC_REG_MONTH); sys_inb(RTC_DATA_REG, &month);
    sys_outb(RTC_ADDR_REG, RTC_REG_YEAR); sys_inb(RTC_DATA_REG, &year);
    date->day = is_binary ? day : bcd_to_bin(day);
    date->month = is_binary ? month : bcd_to_bin(month);
    date->year = is_binary ? year : bcd_to_bin(year);
    return 0;
}
