#include "rtc.h"

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
  uint8_t ones = bcd & 0x0F;
  uint8_t tens = bcd >> 4;
  
  return tens*10 + ones;
}

static int rtc_read_reg(uint8_t reg, uint8_t *val) {
  uint32_t tmp;

  if (sys_outb(RTC_ADDR_REG, reg) != 0)
    return 1;

  if (sys_inb(RTC_DATA_REG, &tmp) != 0)
    return 1;

  *val = (uint8_t) tmp;
  return 0;
}

int rtc_read_date(rtc_date *date) {
  if (!date) return 1;

  uint8_t regB, day, month, year;

  do {
    tickdelay(micros_to_ticks(50));
    if (rtc_read_reg(RTC_REG_A, &regB) != 0)
      return 1;
  } while (regB & RTC_UIP_MSK);

  if (rtc_read_reg(RTC_REG_B, &regB) != 0)
    return 1;

  bool isBCD = !(regB & RTC_DM_MSK);

  if (rtc_read_reg(RTC_REG_DAY, &day) != 0) return 1;
  if (rtc_read_reg(RTC_REG_MONTH, &month) != 0) return 1;
  if (rtc_read_reg(RTC_REG_YEAR, &year) != 0) return 1;

  if (isBCD) {
    day = bcd_to_bin(day);
    month = bcd_to_bin(month);
    year = bcd_to_bin(year);
  }

  date->day = day;
  date->month = month;
  date->year = year;

  return 0;
}