#include "fw/drivers/rtc.h"
#include "fw/common/utils.h"

static int bcd_to_bin(uint8_t bcd) {
  uint8_t ones = bcd & 0x0F;
  uint8_t tens = bcd >> 4;
  
  return tens*10 + ones;
}

static int rtc_read_reg(uint8_t reg, uint8_t *val) {
  uint32_t tmp;

  if (sys_outb(RTC_ADDR_REG, reg) != OK)
    return fail(ERR_RTC, "rtc_read_reg: failed to write RTC address register");

  if (sys_inb(RTC_DATA_REG, &tmp) != OK)
    return fail(ERR_RTC, "rtc_read_reg: failed to read RTC data register");

  *val = (uint8_t) tmp;
  return 0;
}

int rtc_read_date(rtc_date *date) {
  if (!date) return fail(ERR_RTC, "rtc_read_date: NULL date pointer");

  uint8_t regA, regB, seconds, minutes, hours, day, month, year;

  do {
    tickdelay(micros_to_ticks(50));
    if (rtc_read_reg(RTC_REG_A, &regA) != OK)
      return fail(ERR_RTC, "rtc_read_date: failed to read register A");
  } while (regA & RTC_UIP_MSK);


  if (rtc_read_reg(RTC_REG_B, &regB) != OK)
    return fail(ERR_RTC, "rtc_read_date: failed to read register B");

  bool isBCD = !(regB & RTC_DM_MSK);

  if (rtc_read_reg(RTC_REG_SECONDS, &seconds) != OK)
    return fail(ERR_RTC, "rtc_read_date: failed to read seconds");
  if (rtc_read_reg(RTC_REG_MINUTES, &minutes) != OK)
    return fail(ERR_RTC, "rtc_read_date: failed to read minutes");
  if (rtc_read_reg(RTC_REG_HOURS, &hours) != OK)
    return fail(ERR_RTC, "rtc_read_date: failed to read hours");
  if (rtc_read_reg(RTC_REG_DAY, &day) != OK)
    return fail(ERR_RTC, "rtc_read_date: failed to read day");
  if (rtc_read_reg(RTC_REG_MONTH, &month) != OK)
    return fail(ERR_RTC, "rtc_read_date: failed to read month");
  if (rtc_read_reg(RTC_REG_YEAR, &year) != OK)
    return fail(ERR_RTC, "rtc_read_date: failed to read year");

  if (isBCD) {
    seconds = bcd_to_bin(seconds);
    minutes = bcd_to_bin(minutes);
    hours = bcd_to_bin(hours);
    day = bcd_to_bin(day);
    month = bcd_to_bin(month);
    year = bcd_to_bin(year);
  }

  date->seconds = seconds;
  date->minutes = minutes;
  date->hours = hours;
  date->day = day;
  date->month = month;
  date->year = year;

  return 0;
}
