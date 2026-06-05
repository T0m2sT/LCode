#include <lcom/lcf.h>

#include "controller/input/mouse.h"
#include "controller/input/events.h"
#include "fw/drivers/video.h"
#include "model/render_state.h"

static int mouse_x = 0, mouse_y = 0;
static bool mouse_initialized = false;
static bool prev_lb = false;

void mouse_process(mouse_packet pp) {
  // lazy init: position starts at screen center on first packet
  if (!mouse_initialized) {
    mouse_x = (int)vg_get_h_res() / 2;
    mouse_y = (int)vg_get_v_res() / 2;
    mouse_initialized = true;
  }

  bool moved = (pp.delta_x != 0 || pp.delta_y != 0);
  mouse_x += pp.delta_x;
  // delta_y is inverted: mouse up = positive delta, screen y grows downward
  mouse_y -= pp.delta_y;
  if (mouse_x < 0) mouse_x = 0;
  if (mouse_y < 0) mouse_y = 0;
  if (mouse_x >= (int)vg_get_h_res()) mouse_x = (int)vg_get_h_res() - 1;
  if (mouse_y >= (int)vg_get_v_res()) mouse_y = (int)vg_get_v_res() - 1;

  if (moved) set_render(RENDER_MOUSE);

  // rising edge: first frame the button is pressed
  if (pp.lb && !prev_lb) {
    MouseEvent me = { .left_clicked = true, .left_holding = false , .click_x = mouse_x, .click_y = mouse_y, .scroll = pp.delta_z};

    InputEvent iev = {
      .type = INPUT_EVENT_MOUSE,
      .data.mouse = me
    };

    input_event_push(iev);
  // button held: subsequent frames while still pressed
  } else if (pp.lb) {
    MouseEvent me = { .left_clicked = false, .left_holding = true , .click_x = mouse_x, .click_y = mouse_y, .scroll = pp.delta_z};

    InputEvent iev = {
      .type = INPUT_EVENT_MOUSE,
      .data.mouse = me
    };

    input_event_push(iev);
  }

  // scroll is pushed independently, can coexist with a click or hold
  if (pp.delta_z != 0) {
    MouseEvent me = { .left_clicked = false, .click_x = mouse_x, .click_y = mouse_y, .scroll = pp.delta_z };

    InputEvent iev = {
      .type = INPUT_EVENT_MOUSE,
      .data.mouse = me
    };

    input_event_push(iev);
  }

  prev_lb = pp.lb;
}

int ih_get_mouse_x() { return mouse_x; }
int ih_get_mouse_y() { return mouse_y; }
