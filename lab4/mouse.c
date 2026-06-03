#include <lcom/lcf.h>

#include "mouse.h"

static int     hook_id = 2;   /* distinct from keyboard hook_id=1 */
static uint8_t mouse_byte;    /* last byte read by the IH */

uint8_t mouse_get_byte(void) {
  return mouse_byte;
}

int mouse_subscribe_int(uint8_t *bit_no) {
  if (bit_no == NULL) return 1;
  *bit_no = hook_id;
  if (sys_irqsetpolicy(MOUSE_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) != OK)
    return 1;
  return 0;
}

int mouse_unsubscribe_int(void) {
  if (sys_irqrmpolicy(&hook_id) != OK)
    return 1;
  return 0;
}

/* Wait for IBF to clear, then write a byte to the given port. */
static int kbc_write(int port, uint8_t byte) {
  uint8_t st;
  int tries = 10;
  while (tries--) {
    if (util_sys_inb(KBC_STATUS_REG, &st) != OK)
      return 1;
    if (!(st & IBF)) {
      if (sys_outb(port, byte) != OK)
        return 1;
      return 0;
    }
    tickdelay(micros_to_ticks(20000));
  }
  return 1;
}

/* Wait for OBF to be set, then read one byte from 0x60. */
static int kbc_read_out(uint8_t *data) {
  uint8_t st;
  int tries = 10;
  while (tries--) {
    if (util_sys_inb(KBC_STATUS_REG, &st) != OK)
      return 1;
    if (st & OBF) {
      if (util_sys_inb(KBC_OUT_BUF, data) != OK)
        return 1;
      if (st & (PAR_ERR | TO_ERR))
        return 1;
      return 0;
    }
    tickdelay(micros_to_ticks(20000));
  }
  return 1;
}

/*
 * Send a command byte to the mouse:
 *   1. Write 0xD4 to 0x64 (tell KBC to forward next byte to mouse)
 *   2. Write the command byte to 0x60
 *   3. Read and verify the ACK (0xFA) from 0x60
 * Retry up to 3 times on NACK (0xFE) or error.
 */
int mouse_write_cmd(uint8_t cmd) {
  uint8_t ack;
  int retries = 3;
  while (retries--) {
    if (kbc_write(KBC_CMD_REG, KBC_WRITE_AUX) != 0)
      return 1;
    if (kbc_write(KBC_IN_BUF, cmd) != 0)
      return 1;
    if (kbc_read_out(&ack) != 0)
      return 1;
    if (ack == MOUSE_ACK)
      return 0;
    /* 0xFE = resend whole command; 0xFC = error, also retry */
  }
  return 1;
}

int mouse_disable_data_reporting(void) {
  return mouse_write_cmd(MOUSE_DISABLE_DR);
}

/*
 * Interrupt handler: called once per byte the i8042 places in the output buffer.
 * Read exactly one byte and store it; discard on parity/timeout error.
 */
void (mouse_ih)(void) {
  uint8_t st, data;

  if (util_sys_inb(KBC_STATUS_REG, &st) != OK)
    return;

  if (!(st & OBF))
    return;

  /* Always read to clear the buffer */
  if (util_sys_inb(KBC_OUT_BUF, &data) != OK)
    return;

  if (st & (PAR_ERR | TO_ERR))
    return;

  mouse_byte = data;
}
