#pragma once

#include <lcom/lcf.h>

#include "fw/drivers/mouse.h"

#define SCROLL_SPEED_MULTIPLIER 3

typedef struct {
  bool left_clicked;
  int click_x, click_y;
  int scroll;
} MouseEvent;


void mouse_process(mouse_packet pp);

int ih_get_mouse_x();
int ih_get_mouse_y();
