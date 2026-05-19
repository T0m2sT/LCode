#include <stdint.h>

#include "fw/hw/i8042.h"
#include "fw/common/utils.h"

#define KBC_RETRY_DELAY 20000
#define KBC_MAX_TRIES 10

int (kbc_wait_input_empty)() {
  uint8_t status;
    int tries = KBC_MAX_TRIES;

  while (tries--) {
      if (util_sys_inb(KBC_STATUS_REG, &status) != OK) 
        return fail(ERR_KBC, "kbc_wait_input_empty: failed to read status");
      if ((status & KBC_ST_IBF) == 0) return 0;
      tickdelay(micros_to_ticks(KBC_RETRY_DELAY));
  }
  return fail(ERR_KBC, "kbc_wait_input_empty: timeout");
}

int (kbc_wait_output_full)(uint8_t* data){
  uint8_t status;
  int tries = KBC_MAX_TRIES;

  while (tries--) {
      if (util_sys_inb(KBC_STATUS_REG, &status) != OK)
        return fail(ERR_KBC, "kbc_wait_output_full: failed to read status");

      if (status & KBC_ST_OBF) {
          if (util_sys_inb(KBC_DATA_REG, data) != OK) 
            return fail(ERR_KBC, "kbc_wait_output_full: failed to read data");
          return 0;
      }
      tickdelay(micros_to_ticks(KBC_RETRY_DELAY));
  }
  return fail(ERR_KBC, "kbc_wait_input_empty: timeout");

}

int (kbc_enable_interrupts)(void) {
  uint8_t cmd;

  if (kbc_wait_input_empty() != OK)
    return fail(ERR_KBC, "kbc_enable_interrupts: input buffer not empty (read cmd)");

  if (sys_outb(KBC_STATUS_REG, RD_CMD) != OK)
    return fail(ERR_KBC, "kbc_enable_interrupts: failed to send READ_CMD");

  if (kbc_wait_output_full(&cmd) != OK)
    return fail(ERR_KBC, "kbc_enable_interrupts: failed to read command byte");

  cmd |= BIT(0);

  if (kbc_wait_input_empty() != OK)
    return fail(ERR_KBC, "kbc_enable_interrupts: input buffer not empty (write cmd)");

  if (sys_outb(KBC_STATUS_REG, WR_CMD) != OK)
    return fail(ERR_KBC, "kbc_enable_interrupts: failed to send WRITE_CMD");

  if (kbc_wait_input_empty() != OK)
    return fail(ERR_KBC, "kbc_enable_interrupts: input buffer not empty (write data)");

  if (sys_outb(KBC_DATA_REG, cmd) != OK)
    return fail(ERR_KBC, "kbc_enable_interrupts: failed to write command byte");

  return 0;
}
