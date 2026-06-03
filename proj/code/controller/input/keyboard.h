#pragma once

#include <lcom/lcf.h>

#include "fw/drivers/keyboard.h"

typedef enum { DIR_NONE, DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_HOME, DIR_END } Direction;

typedef struct {
  char c;
  bool ctrl;
  bool shift;
  bool altgr;
  bool backspace;
  bool enter;
  bool escape;
  Direction dir;
} KeyEvent;

void keyboard_process(packet_scancode ps);
