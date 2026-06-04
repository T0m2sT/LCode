#pragma once

#include "controller/input/keyboard.h"
#include "controller/input/mouse.h"
#include "controller/serial.h"
#include <stdbool.h>

void commands_dispatch(KeyEvent ev);
void commands_dispatch_mouse(MouseEvent me);
void commands_dispatch_serial(SerialEvent se);
void commands_open_file(const char *path);
bool get_quit();
