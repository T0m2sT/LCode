#include "fw/hw/i8254.h"
#include "fw/drivers/timer.h"
#include "fw/common/utils.h"

static uint32_t int_counter = 0;
static int hook_id = 0;

uint32_t get_int_counter() {
  return int_counter;
}

int(timer_set_frequency)(uint8_t timer, uint32_t freq) {
  if (freq == 0 || freq > TIMER_FREQ)
    return fail(ERR_TIMER, "timer_set_frequency: invalid frequency");

  uint8_t st;

  if (timer_get_conf(timer, &st) != OK)
    return fail(ERR_TIMER, "timer_set_frequency: failed to get timer config");

  uint16_t counter = TIMER_FREQ / freq;
  uint8_t control = (st & 0x0F) | TIMER_LSB_MSB;

  switch (timer)
  {
    case 0:
      control |= TIMER_SEL0;
      break;
    case 1:
      control |= TIMER_SEL1;
      break;
    case 2:
      control |= TIMER_SEL2;
      break;
    default:
      return fail(ERR_TIMER, "timer_set_frequency: invalid timer id");
  }

  uint8_t lsb, msb;

  if (util_get_LSB(counter, &lsb) != OK)
    return fail(ERR_TIMER, "timer_set_frequency: failed to get LSB");

  if (util_get_MSB(counter, &msb) != OK)
    return fail(ERR_TIMER, "timer_set_frequency: failed to get MSB");

  if (sys_outb(TIMER_CTRL, control) != OK)
    return fail(ERR_TIMER, "timer_set_frequency: failed to write control word");

  if (sys_outb(TIMER_0 + timer, lsb) != OK)
    return fail(ERR_TIMER, "timer_set_frequency: failed to write LSB");

  if (sys_outb(TIMER_0 + timer, msb) != OK)
    return fail(ERR_TIMER, "timer_set_frequency: failed to write MSB");

  return 0;
}

int(timer_subscribe_int)(uint8_t *bit_no) {
  if (!bit_no)
    return fail(ERR_TIMER, "timer_subscribe_int: NULL bit_no pointer");

  *bit_no = hook_id;

  if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id) != OK)
    return fail(ERR_TIMER, "timer_subscribe_int: irqsetpolicy failed");

  return 0;
}

int(timer_unsubscribe_int)() {

  if (sys_irqrmpolicy(&hook_id) != OK)
    return fail(ERR_TIMER, "timer_unsubscribe_int: irqrmpolicy failed");

  return 0;
}

void(timer_int_handler)() {
  int_counter++;
}

int(timer_get_conf)(uint8_t timer, uint8_t *st) {

  uint8_t cmd = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);

  if (sys_outb(TIMER_CTRL, cmd) != OK)
    return fail(ERR_TIMER, "timer_get_conf: failed to send read-back command");

  if (util_sys_inb(TIMER_0 + timer, st) != OK)
    return fail(ERR_TIMER, "timer_get_conf: failed to read timer status");

  return 0;
}

int(timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {

  union timer_status_field_val status_val;

  switch(field) {
    case tsf_all:
      status_val.byte = st;
      break;

    case tsf_initial:
      status_val.in_mode = (st >> 4) & 0x03;
      break;

    case tsf_mode:
      status_val.count_mode = (st >> 1) & 0x07;
      break;

    case tsf_base:
      status_val.bcd = st & 0x01;
      break;
  }

  timer_print_config(timer, field, status_val);

  return 0;
}
