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
#define COLOR_SEL_BG 0x264F78
#define COLOR_STATUS_BG 0x007ACC
#define COLOR_STATUS_FG 0xFFFFFF

#define EDITOR_X 10
#define EDITOR_Y 10

static SceneID current_scene = SCENE_EDITOR;
static int prev_col = 0;
static int prev_row = 0;
static int vis_rows = MAX_LINES;
static int vis_cols = MAX_COLS;

/* Translate model coordinates to screen pixel position. */
static int model_to_px(int model_col) {
  return EDITOR_X + (model_col - editor_get_scroll_col()) * FONT_W;
}

static int model_to_py(int model_row) {
  return EDITOR_Y + (model_row - editor_get_scroll_row()) * FONT_H;
}

static void draw_cell(int model_col, int model_row) {
  int x = model_to_px(model_col);
  int y = model_to_py(model_row);
  bb_draw_rect(x, y, FONT_W, FONT_H, COLOR_BG);
  const char *line = editor_get_line(model_row);
  if (model_col < (int)strlen(line)) draw_char(x, y, line[model_col], COLOR_TEXT);
}

static void draw_cursor(int model_col, int model_row) {
  int x = model_to_px(model_col);
  int y = model_to_py(model_row);
  bb_draw_rect(x, y, FONT_W, FONT_H, COLOR_TEXT);
}

static void flip_cell(int model_col, int model_row) {
  vg_flip_region(model_to_px(model_col), model_to_py(model_row), FONT_W, FONT_H);
}

static void draw_selection_bg(int end_r) {
  int sel_start_row, sel_start_col, sel_end_row, sel_end_col;
  editor_sel_get_range(&sel_start_row, &sel_start_col, &sel_end_row, &sel_end_col);
  int scroll_row = editor_get_scroll_row();
  int scroll_col = editor_get_scroll_col();

  //Clamp visible
  int first_row = sel_start_row > scroll_row ? sel_start_row : scroll_row;
  int last_row = sel_end_row < end_r - 1 ? sel_end_row : end_r - 1;

  //Per line draw background rectangle on visible part
  for (int r = first_row; r <= last_row; r++) {
    int y = EDITOR_Y + (r - scroll_row) * FONT_H;
    int col_start = (r == sel_start_row) ? sel_start_col : 0;
    int line_len = (int)strlen(editor_get_line(r));
    int col_end = (r == sel_end_row) ? sel_end_col : line_len;
    int pixel_start = EDITOR_X + (col_start > scroll_col ? col_start - scroll_col : 0) * FONT_W;
    int pixel_end = EDITOR_X + (col_end < scroll_col + vis_cols ? col_end - scroll_col : vis_cols) * FONT_W;

    if (pixel_end > pixel_start) {
      bb_draw_rect(pixel_start, y, pixel_end - pixel_start, FONT_H, COLOR_SEL_BG);
    }
  }
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
  } else if (command_bar_get_status()[0] != '\0') {
    draw_string(4, sy, command_bar_get_status(), 0xFFCC00);
  } else {
    draw_string(4, sy, command_bar_get_filename(), COLOR_STATUS_FG);
  }
}

static void flip_status_bar() {
  vg_flip_region(0, vg_get_v_res() - FONT_H, vg_get_h_res(), FONT_H);
}

int scene_init(SceneID id) {
  current_scene = id;
  vis_rows = ((int)vg_get_v_res() - EDITOR_Y - FONT_H) / FONT_H;
  vis_cols = ((int)vg_get_h_res() - EDITOR_X) / FONT_W;
  editor_set_viewport(vis_rows, vis_cols);
  set_render(RENDER_FULL);
  return 0;
}

void scene_cleanup() {}

void view_render() {
  int mode = get_render();
  clear_render();

  int col = editor_get_cursor_col();
  int row = editor_get_cursor_row();
  int scroll_row = editor_get_scroll_row();
  int scroll_col = editor_get_scroll_col();

  switch (current_scene) {
    case SCENE_EDITOR:
      switch (mode) {
        case RENDER_FULL: {
          bb_clear(COLOR_BG);
          int end_r = scroll_row + vis_rows;
          if (end_r > editor_get_row_count()) end_r = editor_get_row_count();

          if (editor_sel_is_active()) draw_selection_bg(end_r);

          for (int r = scroll_row; r < end_r; r++) {
            int y = EDITOR_Y + (r - scroll_row) * FONT_H;
            const char *line = editor_get_line(r);
            if ((int)strlen(line) > scroll_col)
              draw_string(EDITOR_X, y, line + scroll_col, COLOR_TEXT);
          }
          draw_cursor(col, row);
          render_status_bar();
          vg_flip_buffer();
          prev_col = col;
          prev_row = row;
          break;
        }

        case RENDER_LINE: {
          int y = EDITOR_Y + (row - scroll_row) * FONT_H;
          unsigned line_w = vg_get_h_res() - EDITOR_X;
          bb_draw_rect(EDITOR_X, y, line_w, FONT_H, COLOR_BG);
          const char *line = editor_get_line(row);
          if ((int)strlen(line) > scroll_col)
            draw_string(EDITOR_X, y, line + scroll_col, COLOR_TEXT);
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
          vg_flip_region(model_to_px(col), model_to_py(prev_row),
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
