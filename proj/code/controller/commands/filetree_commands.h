#pragma once

#include "controller/input/keyboard.h"
#include "controller/input/mouse.h"
#include <stdbool.h>

/**
 * @file filetree_commands.h
 * @brief Keyboard and mouse command handlers for when the file tree has focus
 */

/**
 * @brief Tries to handle a keyboard event in the file tree context
 * @param ev Keyboard event to handle
 * @return true if the event was consumed, false if it should fall through to the editor
 */
bool filetree_commands_try(KeyEvent ev);

/**
 * @brief Handles a mouse event in the file tree panel
 * @param me Mouse event to handle
 * @return true if the event was consumed by the file tree
 */
bool filetree_commands_mouse(MouseEvent me);
