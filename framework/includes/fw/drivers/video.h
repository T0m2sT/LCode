#pragma once

#include <lcom/lcf.h>


unsigned vg_get_h_res();
unsigned vg_get_v_res();

int set_graphics_mode(uint16_t mode);
int set_text_mode();

int vg_map_vram(uint16_t mode);

int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color);
int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

int (print_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y);

int video_init(uint16_t mode);
void video_cleanup();
void vg_flip_buffer();
void vg_flip_region(uint16_t x, uint16_t y, uint16_t w, uint16_t h);

void bb_draw_pixel(uint16_t x, uint16_t y, uint32_t color);
uint32_t bb_get_pixel(uint16_t x, uint16_t y);
void bb_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
void bb_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
void bb_clear(uint32_t color);
