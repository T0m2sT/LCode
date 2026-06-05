#include "view/renderer/mouse_cursor.h"
#include "assets/cursor.xpm.h"
#include "fw/drivers/video.h"
#include <stdlib.h>

static xpm_image_t img;
static uint32_t *bg_buf = NULL;
static int prev_x = -1, prev_y = -1;

int mouse_cursor_init() {
  uint8_t *pixels = xpm_load((xpm_map_t)cursor_xpm, XPM_8_8_8_8, &img);
  if (!pixels) return 1;

  //saves background for efficiency
  bg_buf = malloc(img.width * img.height * sizeof(uint32_t));
  if (!bg_buf) return 1;

  return 0;
}

void mouse_cursor_cleanup() {
  free(bg_buf);
  bg_buf = NULL;
  free(img.bytes);
  img.bytes = NULL;
}


void mouse_cursor_hide() {
  // Restores the previous background (from buffer) before cursor appeared, effectively hiding the cursor.
  
  // show was never called
  if (prev_x < 0 || !bg_buf) return;

  int h_res = (int)vg_get_h_res(), v_res = (int)vg_get_v_res();
  for (int j = 0; j < img.height; j++)
    for (int i = 0; i < img.width; i++) {
      int px = prev_x + i, py = prev_y + j;
      if (px >= 0 && px < h_res && py >= 0 && py < v_res)
        bb_draw_pixel(px, py, bg_buf[j * img.width + i]);
    }
}

void mouse_cursor_show(int x, int y) {
  if (!bg_buf) return;
  int h_res = (int)vg_get_h_res(), v_res = (int)vg_get_v_res();
  uint32_t transparent = xpm_transparency_color(XPM_8_8_8_8);
  uint32_t *sprite = (uint32_t *)img.bytes;

  for (int j = 0; j < img.height; j++)
    for (int i = 0; i < img.width; i++) {
      int px = x + i, py = y + j;
      uint32_t sprite_px = sprite[j * img.width + i];

      //out of bound. zero the slot. Hide wont draw it either
      if (px < 0 || px >= h_res || py < 0 || py >= v_res) {
        bg_buf[j * img.width + i] = 0;
        continue;
      }

      bg_buf[j * img.width + i] = bb_get_pixel(px, py);

      //mask alpha
      if (sprite_px != transparent)
        bb_draw_pixel(px, py, sprite_px & 0x00FFFFFF);
    }
    

  prev_x = x;
  prev_y = y;
}

int mouse_cursor_prev_x() {return prev_x; }
int mouse_cursor_prev_y() {return prev_y;}
int mouse_cursor_width() {return img.width; }
int mouse_cursor_height() {return img.height;}
