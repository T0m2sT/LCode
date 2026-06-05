#include <lcom/lcf.h>

#include "proj.h"
#include "controller/ih/ih.h"
#include "fw/common/utils.h"
#include "controller/input/keyboard.h"
#include "controller/input/mouse.h"
#include "controller/commands.h"
#include "controller/serial.h"
#include "model/command_bar.h"
#include "fw/drivers/video.h"
#include "model/render_state.h"
#include "model/clock.h"

static uint8_t irq_timer = 0, irq_keyboard = 0, irq_mouse = 0, irq_serial = 0;
packet_scancode ps = {
  .two_byte = false,
  .make = false,
  .size = 0,
  .bytes = {0, 0}
};

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
  if (mouse_enable_wheel_mode() != OK) {
    timer_unsubscribe_int();
    keyboard_unsubscribe_int();
    mouse_unsubscribe_int();
    return fail(ERR_MOUSE, "subscribe_interrupts: unable to enable wheel mode");
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
    my_mouse_disable_data_reporting();
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

void timer_handler() {
  timer_int_handler();
  if (command_bar_tick()) set_render(RENDER_STATUS);

  if (get_int_counter() % TIMER_HZ == 0) {
    time_update();
    set_render(RENDER_STATUS);
  }
}

void keyboard_handler() {
  keyboard_ih();
            
  if (build_scancode(&ps) != OK) {
      fail(ERR_KEYBOARD, "keyboard_handler: unable to build packet");
    return;
  }

  if (ps.two_byte) {
    return;
  }

  keyboard_process(ps);
}

void mouse_handler() {
  mouse_ih();

  if (is_packet_ready()) {
    mouse_packet pp;
  
    if (build_packet(&pp) != OK) {
      fail(ERR_MOUSE, "mouse_handler: unable to build packet");
      return;
    }

    mouse_process(pp);
  }
}

void interrupts_handler(uint32_t irq_mask) {
  if (irq_mask & irq_timer) timer_handler();
  if (irq_mask & irq_keyboard) keyboard_handler();
  if (irq_mask & irq_serial) serial_process(); 
  if (irq_mask & irq_mouse) mouse_handler();
}
