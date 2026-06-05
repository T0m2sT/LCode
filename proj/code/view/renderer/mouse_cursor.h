#pragma once

#include <lcom/lcf.h>

/**
 * @file mouse_cursor.h
 * @brief Mouse cursor rendering over the back buffer.
 */


/**
 * @brief Loads the cursor sprite and initializes the back-buffer save area
 * @return 0 on success. Non-zero on failure
 */
int mouse_cursor_init();

/**
 * @brief Releases all resources owned by the cursor
 */
void mouse_cursor_cleanup();

/**
 * @brief Restores the pixels under the cursor's previous position
 */
void mouse_cursor_hide();

/**
 * @brief Saves the pixels under the cursor and draws at (x, y)
 * @param x Pixel x coordinate
 * @param y Pixel y coordinate
 */
void mouse_cursor_show(int x, int y);

/**
 * @brief Returns the x coordinate where the cursor was last shown
 * @return Previous x coordinate. -1 if the cursor has not been shown yet
 */
int mouse_cursor_prev_x();

/**
 * @brief Returns the y coordinate where the cursor was last shown
 * @return Previous y coordinate. -1 if the cursor has not been shown yet
 */
int mouse_cursor_prev_y();

/**
 * @brief Returns the cursor sprite width in pixels
 * @return Cursor width
 */
int mouse_cursor_width();

/**
 * @brief Returns the cursor sprite height in pixels
 * @return Cursor height
 */
int mouse_cursor_height();
