#include "view/renderer/font.h"
#include "fw/drivers/video.h"
#include "assets/font.xpm.h"

void draw_char(int x, int y, char c, uint32_t color) {
  if ((uint8_t)c >= 128) return;
  const uint8_t *glyph = font_8x16[(uint8_t)c];

  unsigned h_res = vg_get_h_res();
  for (int row = 0; row < FONT_H; row++) {
    int py = y + row;
    if (py < 0) continue;

    uint32_t *line = bb_row_ptr((uint16_t)py);
    if (!line) break;

    uint8_t bits = glyph[row];
    
    for (int col = 0; col < FONT_W; col++) {
      int px = x + col;
      if (px < 0 || (unsigned)px >= h_res) continue;
      if (bits & (0x80 >> col)) line[px] = color;
    }
  }
}

void draw_string(int x, int y, const char *s, uint32_t color) {
  int start_x = x;
  for (; *s; s++) {
    if (*s == '\n') {
      x = start_x;
      y += FONT_H;
      continue;
    }
    draw_char(x, y, *s, color);
    x += FONT_W;
  }
}
