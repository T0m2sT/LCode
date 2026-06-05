#include "model/time.h"

static char time_buf[16];
static int last_second = -1;

void time_update() {
  rtc_date date = {0, 0, 0, 0, 0, 0};
  rtc_read_date(&date);

  if (date.seconds == last_second) return;

  snprintf(time_buf, sizeof(time_buf), "%02d:%02d:%02d", date.hours, date.minutes, date.seconds);
}


const char *time_get_string() {
  return time_buf;
}
