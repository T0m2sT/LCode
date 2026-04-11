#include <lcom/lcf.h>

#include <lcom/lab3.h>

#include <stdbool.h>
#include <stdint.h>

#include "kbc.h"

static u64_t irq_set;
static uint8_t bytes[2];
static bool two_byte = false;

int main(int argc, char *argv[]) {
  // sets the language of LCF messages (can be either EN-US or PT-PT)
  lcf_set_language("EN-US");

  // enables to log function invocations that are being "wrapped" by LCF
  // [comment this out if you don't want/need it]
  lcf_trace_calls("/home/lcom/labs/lab3/trace.txt");

  // enables to save the output of printf function calls on a file
  // [comment this out if you don't want/need it]
  lcf_log_output("/home/lcom/labs/lab3/output.txt");

  // handles control over to LCF
  // [LCF handles command line arguments and invokes the right function]
  if (lcf_start(argc, argv))
    return 1;

  // LCF clean up tasks
  // [must be the last statement before return]
  lcf_cleanup();

  return 0;
}

int(kbd_test_scan)() {
  uint8_t bit_no;
  uint8_t byte;
  uint8_t size;
  bool make;

  int ipc_status;
  message msg;

  if (kbd_subscribe_int(&bit_no) != 0)
    return 1;

  irq_set = BIT(bit_no);

  int r;
  while (1) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != OK) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            kbc_ih();

            byte = get_scancode_byte();

            if (byte == TWO_BYTE) {
              two_byte = true;
              bytes[0] = byte;
              continue;
            }

            if (two_byte) {
              bytes[1] = byte;
              size = 2;
              two_byte = false;
            } else {
              bytes[0] = byte;
              size = 1;
            }

            make = !(byte & BIT(7));

            kbd_print_scancode(make, size, bytes);

            if (bytes[0] == ESC_BREAK) {
              kbd_unsubscribe_int();
              return 0;
            }
          }
          break;
        default:
          break;
      }
    }
  }

  // Should not reach this
  kbd_unsubscribe_int();
  return 0;
}

int(kbd_test_poll)() {

  uint8_t status, data = 0x00;

  uint8_t size;
  bool make;

  while (data != ESC_BREAK) {
    if (util_sys_inb(KBC_STATUS_REG, &status) != OK) {
      tickdelay(micros_to_ticks(20000));
      continue;
    }
    
    if (!(status & OBF)){
      tickdelay(micros_to_ticks(20000));
      continue;
    }
    
    if (status & (PAR_ERR | TO_ERR)){
      tickdelay(micros_to_ticks(20000));
      continue;
    }

    if (util_sys_inb(KBC_OUT_BUF, &data) != OK) {
      tickdelay(micros_to_ticks(20000));
      continue;
    }

    if (data == 0xE0) {
      two_byte = true;
      bytes[0] = data;
      size = 1;
      continue;
    } else {
      bytes[0] = data;
      size = 1;
    }
    
    make = !(data & BIT(7));

    kbd_print_scancode(make, size, bytes);

    tickdelay(micros_to_ticks(20000));
  }

  
  do {
    util_sys_inb(0x64, &status);
  } while (status & IBF); // wait while IBF is full

  uint8_t cmd_byte;

  // Ask to read command byte
  if (sys_outb(KBC_STATUS_REG, KBC_READ_CMD) != OK) {
    return 1;
  }

  // Reading command byte
  if (util_sys_inb(KBC_OUT_BUF, &cmd_byte) != OK) {
    return 1;
  }
  
  cmd_byte |= BIT(0);  // enable keyboard interrupts

  // Ask to write command byte
  if (sys_outb(KBC_STATUS_REG, KBC_WRITE_CMD) != OK) {
    return 1;
  }

  // Write command byte
  if (sys_outb(KBC_OUT_BUF, cmd_byte) != OK) {
    return 1;
  }

  cmd_byte |= BIT(0) ;



  return 0;
}

int(kbd_test_timed_scan)(uint8_t n) {
  uint32_t timer_counter = 0;

  uint8_t kbd_bit_no;
  uint8_t timer_bit_no;

  uint8_t byte;
  uint8_t size;
  bool make;

  int ipc_status;
  message msg;

  if (kbd_subscribe_int(&kbd_bit_no) != 0)
    return 1;

  if (timer_subscribe_int(&timer_bit_no) != 0)
    return 1;

  int kbd_irq_set = BIT(kbd_bit_no);
  int timer_irq_set = BIT(timer_bit_no);

  int r;
  while (1) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != OK) {
      printf("driver_receive failed with: %d", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & kbd_irq_set) {
            kbc_ih();

            timer_counter = 0;

            byte = get_scancode_byte();

            if (byte == TWO_BYTE) {
              two_byte = true;
              bytes[0] = byte;
              continue;
            }

            if (two_byte) {
              bytes[1] = byte;
              size = 2;
              two_byte = false;
            } else {
              bytes[0] = byte;
              size = 1;
            }

            make = !(byte & BIT(7));

            kbd_print_scancode(make, size, bytes);

            if (bytes[0] == ESC_BREAK) {
              kbd_unsubscribe_int();
              timer_unsubscribe_int();
              return 0;
            }
          }

          if (msg.m_notify.interrupts & timer_irq_set) {
            timer_int_handler();

            timer_counter++;

            if (timer_counter >= n * 60) {
              kbd_unsubscribe_int();
              timer_unsubscribe_int();
              return 0;
            }

          }
          break;
        default:
          break;
      }
    }
  }

  // Should not reach this
  kbd_unsubscribe_int();
  timer_unsubscribe_int();
  return 0;
}
