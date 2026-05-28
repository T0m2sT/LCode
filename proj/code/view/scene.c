#include "scene.h"
#include "video.h"
#include "font.h"
#include "fw/drivers/video.h"
#include "render_flag.h"
#include "model/editor.h"
#include "model/command_bar.h"
#include <string.h>

#define COLOR_BG 0x1E1E1E
#define COLOR_TEXT 0xFFFFFF
#define COLOR_STATUS_BG 0x007ACC
#define COLOR_STATUS_FG 0xFFFFFF

#define EDITOR_X 10
#define EDITOR_Y 10

static SceneID current_scene = SCENE_EDITOR;
static int prev_col = 0;
static int prev_row = 0;

static void draw_cell(int col, int row) {
  int x = EDITOR_X + col * FONT_W;
  int y = EDITOR_Y + row * FONT_H;
  bb_draw_rect(x, y, FONT_W, FONT_H, COLOR_BG);
  const char *line = editor_get_line(row);
  if (line[col]) draw_char(x, y, line[col], COLOR_TEXT);
}

static void draw_cursor(int col, int row) {
  int x = EDITOR_X + col * FONT_W;
  int y = EDITOR_Y + row * FONT_H;
  bb_draw_rect(x, y, FONT_W, FONT_H, COLOR_TEXT);
}

static void flip_cell(int col, int row) {
  vg_flip_region(EDITOR_X + col * FONT_W, EDITOR_Y + row * FONT_H, FONT_W, FONT_H);
}

static void render_status_bar() {
  unsigned h_res = vg_get_h_res();
  unsigned v_res = vg_get_v_res();
  int sy = (int)v_res - FONT_H;

  bb_draw_rect(0, sy, h_res, FONT_H, COLOR_STATUS_BG);

  if (command_bar_get_mode() == MODE_COMMAND) {
    const char *input = command_bar_get_input();
    draw_string(4, sy, ":", COLOR_STATUS_FG);
    int px = 4 + FONT_W;
    draw_string(px, sy, input, COLOR_STATUS_FG);
    int cx = px + (int)strlen(input) * FONT_W;
    bb_draw_rect(cx, sy, FONT_W, FONT_H, COLOR_STATUS_FG);
  } else {
    draw_string(4, sy, command_bar_get_filename(), COLOR_STATUS_FG);
  }
}

static void flip_status_bar() {
  vg_flip_region(0, vg_get_v_res() - FONT_H, vg_get_h_res(), FONT_H);
}

int scene_init(SceneID id) {
  current_scene = id;
  set_render(RENDER_FULL);
  return 0;
}

void scene_cleanup() {}

void view_render() {
  int mode = get_render();
  clear_render();

  int col = editor_get_cursor_col();
  int row = editor_get_cursor_row();

  switch (current_scene) {
    case SCENE_EDITOR:
      switch (mode) {
        case RENDER_FULL:
          bb_clear(COLOR_BG);
          for (int r = 0; r < editor_get_row_count(); r++)
            draw_string(EDITOR_X, EDITOR_Y + r * FONT_H, editor_get_line(r), COLOR_TEXT);
          draw_cursor(col, row);
          render_status_bar();
          vg_flip_buffer();
          prev_col = col;
          prev_row = row;
          break;

        case RENDER_LINE: {
          int y = EDITOR_Y + row * FONT_H;
          unsigned line_w = vg_get_h_res() - EDITOR_X;
          bb_draw_rect(EDITOR_X, y, line_w, FONT_H, COLOR_BG);
          draw_string(EDITOR_X, y, editor_get_line(row), COLOR_TEXT);
          draw_cursor(col, row);
          vg_flip_region(EDITOR_X, y, line_w, FONT_H);
          prev_col = col;
          prev_row = row;
          break;
        }

        case RENDER_WORD: {
          int end = prev_col;
          for (int c = col; c <= end; c++) draw_cell(c, prev_row);
          draw_cursor(col, row);
          vg_flip_region(EDITOR_X + col * FONT_W, EDITOR_Y + prev_row * FONT_H,
                         (end - col + 1) * FONT_W, FONT_H);
          prev_col = col;
          prev_row = row;
          break;
        }

        case RENDER_CHAR:
          draw_cell(prev_col, prev_row);
          flip_cell(prev_col, prev_row);
          draw_cursor(col, row);
          flip_cell(col, row);
          prev_col = col;
          prev_row = row;
          break;

        case RENDER_STATUS:
          render_status_bar();
          flip_status_bar();
          break;

        default: break;
      }
      break;
  }
}
