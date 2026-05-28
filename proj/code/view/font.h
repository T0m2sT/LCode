#pragma once

#include <lcom/lcf.h>

#define FONT_W 8
#define FONT_H 16

void draw_char(int x, int y, char c, uint32_t color);
void draw_string(int x, int y, const char *s, uint32_t color);
