#pragma once

#include <lcom/lcf.h>

/**
 * @file font.h
 * @brief Bitmap font rendering (8x16 pixels per char).
 */

#define FONT_W 8 /**< Letter's width in pixels */
#define FONT_H 16 /**< Letter's height in pixels */

/**
 * @brief Draws a single ASCII character into the back buffer
 * @param x Pixel x coordinate of the glyph's top-left corner
 * @param y Pixel y coordinate of the glyph's top-left corner
 * @param c Character to draw. Ignored if outside the ASCII range (>= 128)
 * @param color 24-bit RGB color for the foreground pixels
 */
void draw_char(int x, int y, char c, uint32_t color);

/**
 * @brief Draws a null-terminated string into the back buffer
 * @param x Pixel x coordinate of the first glyph's top-left corner
 * @param y Pixel y coordinate of the string baseline
 * @param s Null-terminated string to draw
 * @param color 24-bit RGB color for the foreground pixels
 */
void draw_string(int x, int y, const char *s, uint32_t color);
