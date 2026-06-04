#include "view/scene.h"
#include "view/font.h"
#include "view/mouse_cursor.h"
#include "fw/drivers/video.h"
#include "render_flag.h"
#include "model/editor.h"
#include "model/command_bar.h"
#include "model/filetree.h"
#include "controller/ih/ih.h"
#include "view/syntax.h"
#include "controller/input/mouse.h"
#include <string.h>

#define COLOR_BG 0x282C34
#define COLOR_TEXT 0xABB2BF
#define COLOR_SEL_BG 0x3E4451
#define COLOR_STATUS_BG 0x21252B
#define COLOR_STATUS_FG 0xABB2BF
#define COLOR_GUTTER_BG 0x282C34
#define COLOR_GUTTER_FG 0x4B5263
#define COLOR_FILETREE_BG 0x21252B
#define COLOR_FILETREE_SEL 0x2C313C
#define COLOR_FILETREE_DIR 0x61AFEF
#define COLOR_FILETREE_FILE 0xABB2BF
#define COLOR_FILETREE_SEP 0x181A1F

#define FILETREE_COLS 20
#define FILETREE_W_PX (FILETREE_COLS * FONT_W)
#define EDITOR_Y 10
#define GUTTER_DIGITS 3
#define GUTTER_PAD 2
#define SCROLLBAR_W 8
#define SCROLLBAR_HANDLE_MIN 4

static void draw_gutter(int scroll_row, int end_r);
static void draw_filetree(int vrows);
static void draw_scrollbar();
static void draw_text_lines(int scroll_row, int end_r, int scroll_col);
static void draw_line_colored(int x, int y, const char *line, int scroll_col, const uint32_t *colors, int line_len);
static bool scan_block_comment_state(int up_to_row);
static void flip_cursor_region(int x, int y);

static SceneID current_scene = SCENE_EDITOR;
static int prev_col = 0;
static int prev_row = 0;
static int vis_rows = 0;
static int vis_cols = 0;
static int filetree_w = 0;
static int gutter_w = 0;
static int editor_x = 0;
static SyntaxLanguage current_lang = SYNTAX_LANG_NONE;

// Public API 

int scene_init(SceneID id) {
  current_scene = id;
  gutter_w = FONT_W * (GUTTER_DIGITS + GUTTER_PAD);
  editor_x = filetree_w + gutter_w;
  vis_rows = ((int)vg_get_v_res() - EDITOR_Y - FONT_H) / FONT_H;
  vis_cols = ((int)vg_get_h_res() - editor_x - SCROLLBAR_W) / FONT_W;
  editor_set_viewport(vis_rows, vis_cols);
  if (mouse_cursor_init() != 0) return 1;
  set_render(RENDER_FULL);
  return 0;
}

void scene_cleanup() {
  mouse_cursor_cleanup();
}

int scene_get_vis_rows() { return vis_rows; }

void scene_set_language(SyntaxLanguage lang) { current_lang = lang; }




// Coordinate mapping

static int model_to_px(int model_col) {
  return editor_x + (model_col - editor_get_scroll_col()) * FONT_W;
}

static int model_to_py(int model_row) {
  return EDITOR_Y + (model_row - editor_get_scroll_row()) * FONT_H;
}

// Draw primitives

static void draw_cell(int model_col, int model_row, bool in_bc) {
  int x = model_to_px(model_col);
  int y = model_to_py(model_row);
  bb_draw_rect(x, y, FONT_W, FONT_H, COLOR_BG);
  const char *line = editor_get_line(model_row);
  int len = (int)strlen(line);
  if (model_col < len) {
    static uint32_t colors[MAX_COLS];
    bool out_bc;
    syntax_highlight_line(line, len, in_bc, current_lang, colors, &out_bc);
    draw_char(x, y, line[model_col], colors[model_col]);
  }
}

static void draw_cursor(int model_col, int model_row) {
  int x = model_to_px(model_col);
  int y = model_to_py(model_row);
  bb_draw_rect(x, y, FONT_W, FONT_H, COLOR_TEXT);
}

// Widget draws

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
    int pixel_start = editor_x + (col_start > scroll_col ? col_start - scroll_col : 0) * FONT_W;
    int pixel_end = editor_x + (col_end < scroll_col + vis_cols ? col_end - scroll_col : vis_cols) * FONT_W;

    if (pixel_end > pixel_start) {
      bb_draw_rect(pixel_start, y, pixel_end - pixel_start, FONT_H, COLOR_SEL_BG);
    }
  }
}

//Blue status bar at the bottom
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

static void draw_filetree(int vrows) {
  unsigned v_res = vg_get_v_res();
  bb_draw_rect(0, 0, filetree_w, (int)v_res, COLOR_FILETREE_BG);
  bb_draw_rect(filetree_w - 1, 0, 1, (int)v_res, COLOR_FILETREE_SEP);

  int ft_cursor = filetree_get_cursor();
  int ft_scroll = filetree_get_scroll();
  int ft_count = filetree_get_count();
  bool focused = filetree_is_focused();

  for (int i = ft_scroll; i < ft_scroll + vrows && i < ft_count; i++) {
    const FileEntry *e = filetree_get_entry(i);
    int y = EDITOR_Y + (i - ft_scroll) * FONT_H;

    if (i == ft_cursor)
      bb_draw_rect(0, y, filetree_w - 1, FONT_H, focused ? COLOR_FILETREE_SEL : COLOR_GUTTER_BG);

    /* Truncate name to fit, leaving 1 char of left padding. */
    char buf[FILETREE_COLS];
    strncpy(buf, e->name, FILETREE_COLS - 1);
    buf[FILETREE_COLS - 1] = '\0';

    uint32_t fg = e->is_dir ? COLOR_FILETREE_DIR : COLOR_FILETREE_FILE;
    draw_string(4, y, buf, fg);
  }
}

static void draw_gutter(int scroll_row, int end_r) {
  bb_draw_rect(filetree_w, EDITOR_Y, gutter_w, vis_rows * FONT_H, COLOR_GUTTER_BG);
  char buf[GUTTER_DIGITS + 1];
  buf[GUTTER_DIGITS] = '\0';
  for (int r = scroll_row; r < end_r; r++) {
    int y = EDITOR_Y + (r - scroll_row) * FONT_H;
    /* Format line number right-to-left, then skip leading zeros. */
    int n = r + 1;
    for (int i = GUTTER_DIGITS - 1; i >= 0; i--) { buf[i] = '0' + n % 10; n /= 10; }
    const char *p = buf;
    while (*p == '0' && *(p + 1)) p++;
    int x = filetree_w + gutter_w - GUTTER_PAD - (int)strlen(p) * FONT_W;
    draw_string(x, y, p, COLOR_GUTTER_FG);
  }
}

static void draw_scrollbar() {
  int h_res = (int)vg_get_h_res();
  int v_res = (int)vg_get_v_res();
  int track_y = EDITOR_Y;
  int track_h = v_res - FONT_H - track_y;
  int row_count = editor_get_row_count();

  bb_draw_rect(h_res - SCROLLBAR_W, track_y, SCROLLBAR_W, track_h, COLOR_GUTTER_BG);
  if (row_count <= 0) return;

  // Proportionality sizing
  int handle_h = (row_count <= vis_rows) ? track_h : vis_rows * track_h / row_count;
  if (handle_h < SCROLLBAR_HANDLE_MIN) {handle_h = SCROLLBAR_HANDLE_MIN;}
  if (handle_h > track_h) {handle_h = track_h;}

  int max_scroll = row_count > vis_rows ? row_count - vis_rows : 1;
  int max_handle_y = track_y + track_h - handle_h;
  int handle_y = track_y + editor_get_scroll_row() * (max_handle_y - track_y) / max_scroll;

  bb_draw_rect(h_res - SCROLLBAR_W + 2, handle_y, SCROLLBAR_W - 4, handle_h, COLOR_GUTTER_FG);
}

static void draw_line_colored(int x, int y, const char *line, int scroll_col,
                              const uint32_t *colors, int line_len) {
  int draw_x = x;
  for (int c = scroll_col; c < line_len; c++) {
    draw_char(draw_x, y, line[c], colors[c]);
    draw_x += FONT_W;
  }
}

static bool scan_block_comment_state(int up_to_row) {
  bool in_bc = false;
  static uint32_t dummy[MAX_COLS];
  for (int r = 0; r < up_to_row; r++) {
    const char *line = editor_get_line(r);
    int len = (int)strlen(line);
    bool out_bc;
    syntax_highlight_line(line, len, in_bc, current_lang, dummy, &out_bc);
    in_bc = out_bc;
  }
  return in_bc;
}

static void draw_text_lines(int scroll_row, int end_r, int scroll_col) {
  uint32_t colors[MAX_COLS];
  bool in_bc = scan_block_comment_state(scroll_row);
  for (int r = scroll_row; r < end_r; r++) {
    int y = EDITOR_Y + (r - scroll_row) * FONT_H;
    const char *line = editor_get_line(r);
    int len = (int)strlen(line);
    bool out_bc;
    syntax_highlight_line(line, len, in_bc, current_lang, colors, &out_bc);
    in_bc = out_bc;
    if (len > scroll_col)
      draw_line_colored(editor_x, y, line, scroll_col, colors, len);
  }
}


// Render pipeline

static void render_editor_ui(int mode, int col, int row, int scroll_row, int scroll_col) {
  int h_res = (int)vg_get_h_res();
  int end_r = scroll_row + vis_rows;
  if (end_r > editor_get_row_count()) end_r = editor_get_row_count();

  switch (mode) {
    case RENDER_FULL:
      filetree_w = filetree_is_visible() ? FILETREE_W_PX : 0;
      editor_x = filetree_w + gutter_w;
      vis_cols = (h_res - editor_x - SCROLLBAR_W) / FONT_W;

      editor_set_viewport(vis_rows, vis_cols);
      bb_clear(COLOR_BG);

      end_r = scroll_row + vis_rows;
      if (end_r > editor_get_row_count()){end_r = editor_get_row_count();}

      //Draws
      if (filetree_is_visible()){draw_filetree(vis_rows);}
      draw_gutter(scroll_row, end_r);
      if (editor_sel_is_active()){draw_selection_bg(end_r);}
      draw_text_lines(scroll_row, end_r, scroll_col);
      draw_cursor(col, row);
      draw_scrollbar();
      render_status_bar();
      break;

    case RENDER_LINE: {
      int y = EDITOR_Y + (row - scroll_row) * FONT_H;
      int line_w = h_res - editor_x - SCROLLBAR_W;

      //reset line
      bb_draw_rect(editor_x, y, line_w, FONT_H, COLOR_BG);

      //redraw new line
      const char *line = editor_get_line(row);
      int len = (int)strlen(line);
      if (len > scroll_col) {
        uint32_t colors[MAX_COLS];
        bool in_bc = scan_block_comment_state(row);
        bool out_bc;
        syntax_highlight_line(line, len, in_bc, current_lang, colors, &out_bc);
        draw_line_colored(editor_x, y, line, scroll_col, colors, len);
      }
      draw_cursor(col, row);
      break;
    }

    case RENDER_WORD: {
      bool in_bc = scan_block_comment_state(prev_row);
      for (int c = col; c <= prev_col; c++) draw_cell(c, prev_row, in_bc);
      draw_cursor(col, row);
      break;
    }

    case RENDER_CHAR: {
      bool prev_vis = (prev_row >= scroll_row && prev_row < scroll_row + vis_rows &&
                       prev_col >= scroll_col && prev_col < scroll_col + vis_cols);
      bool curr_vis = (row >= scroll_row && row < scroll_row + vis_rows &&
                       col >= scroll_col && col < scroll_col + vis_cols);
      if (prev_vis) {
        bool in_bc = scan_block_comment_state(prev_row);
        draw_cell(prev_col, prev_row, in_bc);
      }
      if (curr_vis) draw_cursor(col, row);
      break;
    }

    case RENDER_STATUS:
      render_status_bar();
      break;

    default: break;
  }
}

static void flip_editor_ui(int mode, int col, int row, int scroll_row, int scroll_col) {
  int h_res = (int)vg_get_h_res();

  switch (mode) {
    case RENDER_STATUS:
      flip_status_bar();
      break;
    case RENDER_LINE:
      vg_flip_region(editor_x, EDITOR_Y + (row - scroll_row) * FONT_H,
                     h_res - editor_x - SCROLLBAR_W, FONT_H);
      break;
    case RENDER_CHAR: {
      bool prev_vis = (prev_row >= scroll_row && prev_row < scroll_row + vis_rows &&
                       prev_col >= scroll_col && prev_col < scroll_col + vis_cols);
      bool curr_vis = (row >= scroll_row && row < scroll_row + vis_rows &&
                       col >= scroll_col && col < scroll_col + vis_cols);
      if (prev_vis) vg_flip_region(model_to_px(prev_col), model_to_py(prev_row), FONT_W, FONT_H);
      if (curr_vis) vg_flip_region(model_to_px(col), model_to_py(row), FONT_W, FONT_H);
      break;
    }
    case RENDER_WORD:
      vg_flip_region(model_to_px(col), model_to_py(prev_row),
                     (prev_col - col + 1) * FONT_W, FONT_H);
      break;
    default: break;
  }
}

void view_render() {
  //Snapshot of current state
  int mode = get_render();
  clear_render();
  int mx = ih_get_mouse_x(), my = ih_get_mouse_y();
  int old_mx = mouse_cursor_prev_x(), old_my = mouse_cursor_prev_y();
  int col = editor_get_cursor_col(), row = editor_get_cursor_row();
  int scroll_row = editor_get_scroll_row(), scroll_col = editor_get_scroll_col();

  mouse_cursor_hide();

  if (current_scene == SCENE_EDITOR)
    render_editor_ui(mode, col, row, scroll_row, scroll_col);

  mouse_cursor_show(mx, my);

  if (mode == RENDER_FULL) {
    vg_flip_buffer();
  } 
  else {
    if (old_mx >= 0) {flip_cursor_region(old_mx, old_my);}
    flip_cursor_region(mx, my);
    if (current_scene == SCENE_EDITOR) {flip_editor_ui(mode, col, row, scroll_row, scroll_col);}
  }

  if (mode != RENDER_MOUSE && mode != RENDER_STATUS) {
    prev_col = col;
    prev_row = row;
  }
}

/* ── Mouse ──────────────────────────────────────────────────────────────── */

static void flip_cursor_region(int x, int y) {
  int h_res = (int)vg_get_h_res(), v_res = (int)vg_get_v_res();
  int w = mouse_cursor_width(), h = mouse_cursor_height();

  if (x < 0) x = 0;
  if (y < 0) y = 0;

  if (x + w > h_res) w = h_res - x;
  if (y + h > v_res) h = v_res - y;

  if (w > 0 && h > 0) vg_flip_region(x, y, w, h);
}

bool scene_px_to_text(int px, int py, int *out_row, int *out_col) {
  int status_y = (int)vg_get_v_res() - FONT_H;

  //checks if click is inside editor area
  if (px < editor_x || px >= (int)vg_get_h_res() - SCROLLBAR_W) return false;
  if (py < EDITOR_Y || py >= status_y) return false;


  int col = (px - editor_x) / FONT_W + editor_get_scroll_col();
  int row = (py - EDITOR_Y) / FONT_H + editor_get_scroll_row();

  if (row < 0) row = 0;
  if (row >= editor_get_row_count()) row = editor_get_row_count() - 1;
  if (col < 0) col = 0;

  int len = (int)strlen(editor_get_line(row));
  if (col > len) col = len;
  *out_row = row;
  *out_col = col;
  return true;
}

bool scene_click_scrollbar(int px, int py) {
  int h_res = (int)vg_get_h_res(), v_res = (int)vg_get_v_res();
  int track_y = EDITOR_Y, track_h = v_res - FONT_H - track_y;

  //checks if click is inside scrollbar area
  if (px < h_res - SCROLLBAR_W || px >= h_res) return false;
  if (py < track_y || py >= track_y + track_h) return false;

  int row_count = editor_get_row_count();
  int max_scroll = row_count > vis_rows ? row_count - vis_rows : 0;
  int target = max_scroll > 0 ? (py - track_y) * max_scroll / track_h : 0;
  
  editor_scroll_by(target - editor_get_scroll_row(), 0);
  if (editor_consume_scroll_dirty()) set_render(RENDER_FULL);
  return true;
}
