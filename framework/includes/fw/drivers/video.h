#pragma once

#include <lcom/lcf.h>


int set_graphics_mode(uint16_t mode);
int set_text_mode();

int vg_map_vram(uint16_t mode);

// Getters for projects buffer
void*    vg_get_video_mem();
unsigned vg_get_h_res();
unsigned vg_get_v_res();
unsigned vg_get_bytes_per_pixel();
unsigned vg_get_frame_size();

int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color);
int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color);
int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color);

int (print_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y);
