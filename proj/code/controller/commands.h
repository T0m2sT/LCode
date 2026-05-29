#pragma once

#include "keyboard.h"
#include <stdbool.h>

typedef struct {
  bool left_clicked;
  int click_x, click_y;
} MouseEvent;

void commands_dispatch(KeyEvent ev);
void commands_dispatch_mouse(MouseEvent me);
void commands_open_file(const char *path);
bool get_quit();
