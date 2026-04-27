// IMPORTANT: you must include the following line in all your C files
#include <lcom/lcf.h>

#include <lcom/lab5.h>

#include <stdint.h>
#include <stdio.h>

#include "../lab3/kbc.h"
#include "video.h"
#include "VBE.h"

// Any header files included below this line should have been created by you

int main(int argc, char *argv[]) {
    // sets the language of LCF messages (can be either EN-US or PT-PT)
    lcf_set_language("EN-US");

    // enables to log function invocations that are being "wrapped" by LCF
    // [comment this out if you don't want/need it]
    lcf_trace_calls("/home/lcom/labs/lab5/trace.txt");

    // enables to save the output of printf function calls on a file
    // [comment this out if you don't want/need it]
    lcf_log_output("/home/lcom/labs/lab5/output.txt");

    // handles control over to LCF
    // [LCF handles command line arguments and invokes the right function]
    if (lcf_start(argc, argv))
        return 1;

    // LCF clean up tasks
    // [must be the last statement before return]
    lcf_cleanup();

    return 0;
}

int waiting_esc_key() {
  uint8_t bit_no;
  int ipc_status;
  message msg;

  unsigned char byte;
  bool two_byte = false;
  uint8_t bytes[2];

  if (kbd_subscribe_int(&bit_no) != 0)
    return 1;

  u64_t irq_set = BIT(bit_no);

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
              two_byte = false;
            } else {
              bytes[0] = byte;
            }

            if (bytes[0] == ESC_BREAK) {
              if (kbd_unsubscribe_int() != OK) return 1;
              return 0;
            }
          }
          break;
        default:
          break;
      }
    }
  }
  if (kbd_unsubscribe_int() != OK) return 1;
  return 0;
}


int(video_test_init)(uint16_t mode, uint8_t delay) {
  
  if (set_graphics_mode(mode) != OK) return 1;

  sleep(delay);

  if (set_text_mode() != OK) return 1;

  return 0;
}

int(video_test_rectangle)(uint16_t mode, uint16_t x, uint16_t y,
                          uint16_t width, uint16_t height, uint32_t color) {

  if (vg_map_vram(mode) != OK) return 1;

  if (set_graphics_mode(mode) != OK) return 1;

  if (vg_draw_rectangle(x, y, width, height, color) != OK) return 1;

  if (waiting_esc_key() != OK) {
    set_text_mode();
    return 1;
  }

  if (set_text_mode() != OK) return 1;

  return 0;
}

int(video_test_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {

  if (vg_map_vram(VBE_768p_INDEXED) != OK) return 1;

  if (set_graphics_mode(VBE_768p_INDEXED) != OK) return 1;

  if (print_xpm(xpm, x, y) != OK) {
    set_text_mode();
    return 1;
  }

  if (waiting_esc_key() != OK) {
    set_text_mode();
    return 1;
  }

  if (set_text_mode() != OK) return 1;

  return 0;
}
