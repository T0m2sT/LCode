#pragma once

#include "controller/input/keyboard.h"
#include "controller/input/mouse.h"
#include "controller/serial.h"
#include <stdbool.h>

/**
 * @file commands.h
 * @brief Top-level command dispatcher: routes input events to editor or file tree actions
 */

/**
 * @brief Dispatches a keyboard event to the appropriate command handler
 *
 * Routes to filetree_commands if the file tree is focused, otherwise to editor commands
 * @param ev Decoded keyboard event to dispatch
 */
void commands_dispatch(KeyEvent ev);

/**
 * @brief Dispatches a mouse event to the appropriate command handler
 * @param me Mouse event to dispatch
 */
void commands_dispatch_mouse(MouseEvent me);

/**
 * @brief Dispatches a serial event from the remote peer
 * @param se Serial event to dispatch
 */
void commands_dispatch_serial(SerialEvent se);

/**
 * @brief Opens a file by path: loads its contents into the editor and updates the status bar
 * @param path Null-terminated absolute path to the file
 */
void commands_open_file(const char *path);

/**
 * @brief Returns whether the quit flag has been set
 * @return true if the application should exit the main loop
 */
bool get_quit();
