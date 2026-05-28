#pragma once

#include <lcom/lcf.h>

int  video_init();
void video_cleanup();


void vg_flip_buffer();

// Buffer drawing functions
void bb_draw_pixel(uint16_t x, uint16_t y, uint32_t color);
void bb_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
void bb_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void bb_clear(uint32_t color);
