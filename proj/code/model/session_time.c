#include "model/session_time.h"

#include "fw/drivers/timer.h"

static uint64_t start_ticks = 0;

void session_time_reset() {
  start_ticks = get_int_counter();
}

uint64_t session_time_get_seconds() {
  return (get_int_counter() - start_ticks) / TIMER_HZ;
}

void session_time_format(char *buf, size_t n) {
  uint64_t s = session_time_get_seconds();

  int hours = s / 3600;
  int minutes = (s % 3600) / 60;
  int seconds = s % 60;

  snprintf(buf, n, "%02d:%02d:%02d", hours, minutes, seconds);
}
