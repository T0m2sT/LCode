#include <lcom/lcf.h>

#include "ih.h"
#include "fw/common/utils.h"
#include "controller/keyboard.h"
#include "controller/commands.h"
#include "controller/serial.h"
#include "model/command_bar.h"
#include "fw/drivers/video.h"
#include "render_flag.h"

static uint8_t irq_timer = 0, irq_keyboard = 0, irq_mouse = 0, irq_serial = 0;
static int mouse_x = 0, mouse_y = 0;
static bool mouse_initialized = false;
static bool prev_lb = false;

int subscribe_interrupts() {
  uint8_t bit_no;

  if (timer_subscribe_int(&bit_no) != OK)
    return fail(ERR_TIMER, "subscribe_interrupts: unable to subscribe timer interrupt");
  irq_timer = BIT(bit_no);

  if (keyboard_subscribe_int(&bit_no) != OK) {
    timer_unsubscribe_int();
    return fail(ERR_KEYBOARD, "subscribe_interrupts: unable to subscribe keyboard interrupt");
  }
  irq_keyboard = BIT(bit_no);

  if (mouse_subscribe_int(&bit_no) != OK) {
    timer_unsubscribe_int();
    keyboard_unsubscribe_int();
    return fail(ERR_MOUSE, "subscribe_interrupts: unable to subscribe mouse interrupt");
  }
  if (my_mouse_enable_data_reporting() != OK) {
    timer_unsubscribe_int();
    keyboard_unsubscribe_int();
    mouse_unsubscribe_int();
    return fail(ERR_MOUSE, "subscribe_interrupts: unable to enable data reporting");
  }
  irq_mouse = BIT(bit_no);
  
  if (serial_subscribe_int(&bit_no) != OK) {
    timer_unsubscribe_int();
    keyboard_unsubscribe_int();
    mouse_unsubscribe_int();
    return fail(ERR_SERIAL, "subscribe_interrupts: unable to subscribe serial interrupt");
  }
  irq_serial = BIT(bit_no);

  return 0;
}

int unsubscribe_interrupts() {
  int errors = 0;

  if (mouse_unsubscribe_int() != OK) {
    errors = 1;
    fail(ERR_MOUSE, "unsubscribe_interrupts: unable to unsubscribe mouse interrupt");
  }
  if (keyboard_unsubscribe_int() != OK) {
    errors = 1;
    fail(ERR_KEYBOARD, "unsubscribe_interrupts: unable to unsubscribe keyboard interrupt");
  }
  if (timer_unsubscribe_int() != OK) {
    errors = 1;
    fail(ERR_TIMER, "unsubscribe_interrupts: unable to unsubscribe timer interrupt");
  }
  if (my_mouse_disable_data_reporting() != OK) {
    errors = 1;
    fail(ERR_MOUSE, "unsubscribe_interrupts: unable to disable mouse data reporting");
  }

  if (serial_unsubscribe_int() != OK) {
    errors = 1;
    fail(ERR_SERIAL, "unsubscribe_interrupts: unable to unsubscribe serial interrupt");
  }

  return errors;
}

void interrupts_handler(uint32_t irq_mask) {
  if (irq_mask & irq_timer) {
    timer_int_handler();
    if (command_bar_tick()) set_render(RENDER_STATUS);
  }
  if (irq_mask & irq_keyboard) keyboard_process();
  if (irq_mask & irq_serial) serial_process(); 
  if (irq_mask & irq_mouse) {
    mouse_ih();
    if (is_packet_ready()) {
      struct packet pp;
      if (build_packet(&pp) != OK) return;

      if (!mouse_initialized) {
        mouse_x = (int)vg_get_h_res() / 2;
        mouse_y = (int)vg_get_v_res() / 2;
        mouse_initialized = true;
      }

      bool moved = (pp.delta_x != 0 || pp.delta_y != 0);
      mouse_x += pp.delta_x;
      mouse_y -= pp.delta_y;
      if (mouse_x < 0) mouse_x = 0;
      if (mouse_y < 0) mouse_y = 0;
      if (mouse_x >= (int)vg_get_h_res()) mouse_x = (int)vg_get_h_res() - 1;
      if (mouse_y >= (int)vg_get_v_res()) mouse_y = (int)vg_get_v_res() - 1;

      if (moved) set_render(RENDER_MOUSE);

      if (pp.lb && !prev_lb) {
        MouseEvent me = {.left_clicked = true, .click_x = mouse_x, .click_y = mouse_y};
        commands_dispatch_mouse(me);
      }
      prev_lb = pp.lb;
    }
  }
}

int ih_get_mouse_x() { return mouse_x; }
int ih_get_mouse_y() { return mouse_y; }
