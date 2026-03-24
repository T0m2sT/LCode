#include <lcom/lcf.h>
#include <lcom/timer.h>

#include <stdint.h>

#include "i8254.h"

uint32_t int_counter = 0;
int hook_id = 0;

int(timer_set_frequency)(uint8_t timer, uint32_t freq) {
  if (freq == 0 || freq > TIMER_FREQ) return 1;

  uint8_t st;

  if (timer_get_conf(timer, &st) != 0)
    return 1;
  
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
  }

  uint8_t lsb, msb;

  if (util_get_LSB(counter, &lsb) != 0) return 1;
  if (util_get_MSB(counter, &msb) != 0) return 1;


  if (sys_outb(TIMER_CTRL, control) != 0) return 1;

  if (sys_outb(TIMER_0 + timer, lsb) != 0) return 1;
  if (sys_outb(TIMER_0 + timer, msb) != 0) return 1;

  return 0;
}

int(timer_subscribe_int)(uint8_t *bit_no) {
  
  *bit_no = hook_id;

  if (sys_irqsetpolicy(TIMER0_IRQ, IRQ_REENABLE, &hook_id) != 0)
    return 1;

  return 0;
}

int(timer_unsubscribe_int)() {
  
  if (sys_irqrmpolicy(&hook_id) != 0)
    return 1;

  return 0;
}

void(timer_int_handler)() {
  int_counter++;
}

int(timer_get_conf)(uint8_t timer, uint8_t *st) {

  // TIMER_RB_CMD -> Tells that this is a READ-BACK command
  // TIMER_RB_COUNT_ -> Tells that we dont want the count
  // TIMER_RB_SEL(timer) -> Selects timer
  uint8_t cmd = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);

  if (sys_outb(TIMER_CTRL, cmd) != 0)
    return 1;

  if (util_sys_inb(TIMER_0 + timer, st) != 0)
    return 1;
  

  return 0;
}

int(timer_display_conf)(uint8_t timer, uint8_t st, enum timer_status_field field) {

  union timer_status_field_val status_val;

  switch(field) {
    case tsf_all:
      status_val.byte = st;
      break;

    case tsf_initial: {
      status_val.in_mode = (st >> 4) & 0x03;
      break;
    }

    case tsf_mode: {
      uint8_t mode = (st >> 1) & 0x07;
      status_val.count_mode = mode;
      break;
    }

    case tsf_base: {
      uint8_t base = st & 0x01;
      status_val.bcd = base;
      break;
    }
  }

  timer_print_config(timer, field, status_val);

  return 0;
}
