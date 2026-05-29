#pragma once

#include "keyboard.h"

void commands_dispatch(KeyEvent ev);
void commands_open_file(const char *path);
bool get_quit();
