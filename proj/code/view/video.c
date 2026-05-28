#include "video.h"
#include "fw/drivers/video.h"
#include "fw/hw/vbe.h"
#include "fw/common/utils.h"

#include <stdlib.h>
#include <string.h>

#define VIDEO_MODE VBE_864p_DC

static uint8_t *back_buffer = NULL;

int video_init() {
  if (vg_map_vram(VIDEO_MODE) != OK)
    return fail(ERR_VIDEO, "video_init: vg_map_vram failed");

  if (set_graphics_mode(VIDEO_MODE) != OK)
    return fail(ERR_VIDEO, "video_init: set_graphics_mode failed");

  back_buffer = malloc(vg_get_frame_size());
  if (back_buffer == NULL)
    return fail(ERR_VIDEO, "video_init: malloc back_buffer failed");

  memset(back_buffer, 0, vg_get_frame_size());

  return 0;
}

void vg_flip_buffer() {
  memcpy(vg_get_video_mem(), back_buffer, vg_get_frame_size());
}

void video_cleanup() {
  free(back_buffer);
  back_buffer = NULL;
  set_text_mode();
}


//Buffer

void bb_draw_pixel(uint16_t x, uint16_t y, uint32_t color) {
  if (x >= vg_get_h_res() || y >= vg_get_v_res()) return;

  unsigned bpp = vg_get_bytes_per_pixel();
  unsigned index = (vg_get_h_res() * y + x) * bpp;
  memcpy(back_buffer + index, &color, bpp);
}

void bb_draw_hline(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (uint16_t i = 0; i < len; i++) {
    bb_draw_pixel(x + i, y, color);
  }
}

void bb_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
  for (uint16_t row = 0; row < h; row++){
    bb_draw_hline(x, y + row, w, color);
  }
}

void bb_clear(uint32_t color) {
  if (color == 0) {
    memset(back_buffer, 0, vg_get_frame_size());
    return;
  }

  unsigned bpp = vg_get_bytes_per_pixel();
  unsigned pixels = vg_get_h_res() * vg_get_v_res();
  for (unsigned i = 0; i < pixels; i++) {
    memcpy(back_buffer + i * bpp, &color, bpp);
  }
}
