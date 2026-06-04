#pragma once

#include "controller/input/keyboard.h"
#include "controller/input/mouse.h"
#include <stdbool.h>

bool filetree_commands_try(KeyEvent ev);
bool filetree_commands_mouse(MouseEvent me);
