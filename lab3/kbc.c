#include <lcom/lcf.h>

#include "kbc.h"

static int hook_id = 1;
static uint8_t scancode_byte;  

uint8_t get_scancode_byte(void) {
  return scancode_byte;
}

void set_scancode_byte(uint8_t byte) {
  scancode_byte = byte;
}

int kbd_subscribe_int(uint8_t *bit_no) {
  *bit_no = hook_id;

  if (sys_irqsetpolicy(KEYBOARD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) != OK) {
    return 1;
  }

  return 0;
}


int kbd_unsubscribe_int() {

  if (sys_irqrmpolicy(&hook_id) != OK) {
    return 1;
  }

  return 0;
}

void (kbc_ih)() {

  uint8_t status;
  uint8_t data;
    
  // Read status register
  if (util_sys_inb(KBC_STATUS_REG, &status) != OK)
    return;

  // Check if output buffer is full
  if (!(status & OBF))
    return;

  // Check for errors
  if (status & (PAR_ERR | TO_ERR))
    return;

  // Read scancode byte
  if (util_sys_inb(KBC_OUT_BUF, &data) != OK)
    return;


  set_scancode_byte(data);
}
