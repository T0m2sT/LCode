#include "view/scene.h"
#include "view/font.h"
#include "view/mouse_cursor.h"
#include "fw/drivers/video.h"
#include "model/render_state.h"
#include "model/editor.h"
#include "model/command_bar.h"
#include "model/filetree.h"
#include "controller/ih/ih.h"
#include "view/highlight_cache.h"
#include "controller/input/mouse.h"
#include "model/time/clock.h"
#include "model/time/session_time.h"
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
#define GUTTER_DIGITS_MIN 3
#define GUTTER_PAD 2
#define SCROLLBAR_W 8
#define SCROLLBAR_HANDLE_MIN 4

typedef struct { int x, y, w, h; } Rect;

static struct {
  Rect editor;
  Rect filetree;
  Rect gutter;
  Rect scrollbar;
  Rect status_bar;
} layout;

// --- Layout ---

static void scene_update_layout(void);

// --- Coordinate Mapping ---

static int  model_to_px(int model_col);
static int  model_to_py(int model_row);

// --- Draw Primitives ---

static void draw_cursor(int model_col, int model_row);
static void draw_line_colored(int x, int y, const char *line, int scroll_col, const uint32_t *colors, int line_len);

// --- Widget Draws ---

static void draw_selection_bg(int end_r);
static void draw_filetree(int vrows);
static void draw_gutter(int scroll_row, int end_r);
static void draw_scrollbar(void);
static void draw_text_lines(int scroll_row, int end_r, int scroll_col);
static void render_status_bar(void);

// --- Render & Flip Pipeline ---

static void render_editor_ui(int mode, int col, int row, int scroll_row, int scroll_col);
static void flip_editor_ui(int mode, int col, int row, int scroll_row, int scroll_col);
static void flip_status_bar(void);
static void flip_cursor_region(int x, int y);

static SceneID current_scene = SCENE_EDITOR;
static int prev_col = 0;
static int prev_row = 0;
static int vis_rows = 0;
static int vis_cols = 0;
static int gutter_w = 0;


static void scene_update_layout(void) {
  int h_res = (int)vg_get_h_res();
  int v_res = (int)vg_get_v_res();
  int fw = filetree_is_visible() ? FILETREE_W_PX : 0;

  // gutter width grows dynamically based on line count
  int digits = 1;
  for (int n = editor_get_row_count(); n >= 10; n /= 10) digits++;
  if (digits < GUTTER_DIGITS_MIN) digits = GUTTER_DIGITS_MIN;
  gutter_w = FONT_W * (digits + GUTTER_PAD);

  layout.status_bar = (Rect){0, v_res - FONT_H, h_res, FONT_H};
  layout.filetree = (Rect){0, 0, fw, v_res};
  layout.gutter = (Rect){fw, EDITOR_Y, gutter_w, v_res - EDITOR_Y - FONT_H};
  layout.scrollbar = (Rect){h_res - SCROLLBAR_W, EDITOR_Y, SCROLLBAR_W, layout.gutter.h};
  //Editor fills space available
  layout.editor = (Rect){fw + gutter_w, EDITOR_Y, layout.scrollbar.x - (fw + gutter_w), layout.gutter.h};

  vis_cols = layout.editor.w / FONT_W;
  vis_rows = layout.editor.h / FONT_H;
}

int scene_init(SceneID id) {
  current_scene = id;
  scene_update_layout();
  editor_set_viewport(vis_rows, vis_cols);
  if (mouse_cursor_init() != 0) return 1;

  //Full redraw on first frame
  set_render(RENDER_FULL);
  return 0;
}

void scene_cleanup() {
  mouse_cursor_cleanup();
  highlight_cache_cleanup();
}

int scene_get_vis_rows() { return vis_rows; }

void scene_set_language(SyntaxLanguage lang) {
  highlight_cache_set_language(lang);
}


// Coordinate mapping

static int model_to_px(int model_col) {
  return layout.editor.x + (model_col - editor_get_scroll_col()) * FONT_W;
}

static int model_to_py(int model_row) {
  return layout.editor.y + (model_row - editor_get_scroll_row()) * FONT_H;
}



// Draw

static void draw_cursor(int model_col, int model_row) {
  //dont draw cursor if filetree focused
  if (filetree_is_focused()) return;

  int x = model_to_px(model_col);
  int y = model_to_py(model_row);
  bb_draw_rect(x, y, FONT_W, FONT_H, COLOR_TEXT);
}

static void draw_remote_cursor(int model_col, int model_row) {

  if (model_row < 0 || model_col < 0) return; //if remote does not began to send cursor position
  int x = model_to_px(model_col);
  int y = model_to_py(model_row);
  
  bb_draw_rect(x, y, FONT_W, FONT_H, 0xFF0000);//draw in red color
}

// Widget draws

static void draw_selection_bg(int end_r) {
  int sel_start_row, sel_start_col, sel_end_row, sel_end_col;
  editor_sel_get_range(&sel_start_row, &sel_start_col, &sel_end_row, &sel_end_col);
  int scroll_row = editor_get_scroll_row();
  int scroll_col = editor_get_scroll_col();

  //Intersects selection with visible rows
  int first_row = sel_start_row > scroll_row ? sel_start_row : scroll_row;
  int last_row = sel_end_row < end_r - 1 ? sel_end_row : end_r - 1;

  //Per line draw background rectangle on visible part
  for (int r = first_row; r <= last_row; r++) {
    int y = layout.editor.y + (r - scroll_row) * FONT_H;
    int col_start = (r == sel_start_row) ? sel_start_col : 0;
    int line_len = editor_get_line_len(r);
    int col_end = (r == sel_end_row) ? sel_end_col : line_len;

    //Clamp edges to horizontal viewport
    int pixel_start = layout.editor.x + (col_start > scroll_col ? col_start - scroll_col : 0) * FONT_W;
    int pixel_end = layout.editor.x + (col_end < scroll_col + vis_cols ? col_end - scroll_col : vis_cols) * FONT_W;

    //Skip rows where selected is outside visible viewport
    if (pixel_end > pixel_start) {
      bb_draw_rect(pixel_start, y, pixel_end - pixel_start, FONT_H, COLOR_SEL_BG);
    }
  }
}

//Renders blue status bar at the bottom
static void render_status_bar() {
  bb_draw_rect(layout.status_bar.x, layout.status_bar.y, layout.status_bar.w, layout.status_bar.h, COLOR_STATUS_BG);
  int sy = layout.status_bar.y;

  // Priority: command bar > status message > filename
  if (command_bar_get_mode() == MODE_COMMAND) {
    const char *input = command_bar_get_input();
    draw_string(4, sy, ":", COLOR_STATUS_FG);
    int px = 4 + FONT_W;
    draw_string(px, sy, input, COLOR_STATUS_FG);

    //draw cursor after last input char
    int cx = px + (int)strlen(input) * FONT_W;
    bb_draw_rect(cx, sy, FONT_W, FONT_H, COLOR_STATUS_FG);
  }
  else if (command_bar_get_status()[0] != '\0') {
    draw_string(4, sy, command_bar_get_status(), 0xFFCC00);
  } 
  else {
    draw_string(4, sy, command_bar_get_filename(), COLOR_STATUS_FG);
  }

  const char *t = time_get_string();
  int len = strlen(t);
  int x = layout.status_bar.w - (len * FONT_W) - 4;
  draw_string(x, sy, t, COLOR_STATUS_FG);

  // Session time centered in the status bar (8 chars * FONT_W / 2 = 32px offset)
  char st[16];
  session_time_format(st, 16);
  draw_string(layout.status_bar.x + layout.status_bar.w/2 - 32, layout.status_bar.y, st, COLOR_STATUS_FG);
}

static void flip_status_bar() {
  vg_flip_region(layout.status_bar.x, layout.status_bar.y, layout.status_bar.w, layout.status_bar.h);
}

static void draw_filetree(int vrows) {
  bb_draw_rect(layout.filetree.x, layout.filetree.y, layout.filetree.w, layout.filetree.h, COLOR_FILETREE_BG);
  bb_draw_rect(layout.filetree.w - 1, 0, 1, layout.filetree.h, COLOR_FILETREE_SEP);

  int ft_cursor = filetree_get_cursor();
  int ft_scroll = filetree_get_scroll();
  int ft_count = filetree_get_count();
  bool focused = filetree_is_focused();

  for (int i = ft_scroll; i < ft_scroll + vrows && i < ft_count; i++) {
    const FileEntry *e = filetree_get_entry(i);
    int y = EDITOR_Y + (i - ft_scroll) * FONT_H;

    //dims highlight if editor has focus
    if (i == ft_cursor)
      bb_draw_rect(0, y, layout.filetree.w - 1, FONT_H, focused ? COLOR_FILETREE_SEL : COLOR_GUTTER_BG);

    /* Truncate name to fit, leaving 4px left padding. */
    char buf[FILETREE_COLS];
    strncpy(buf, e->name, FILETREE_COLS - 1);
    buf[FILETREE_COLS - 1] = '\0';

    uint32_t fg = e->is_dir ? COLOR_FILETREE_DIR : COLOR_FILETREE_FILE;
    draw_string(4, y, buf, fg);
  }
}

static void draw_gutter(int scroll_row, int end_r) {
  bb_draw_rect(layout.gutter.x, layout.gutter.y, layout.gutter.w, layout.gutter.h, COLOR_GUTTER_BG);
  int digits = layout.gutter.w / FONT_W - GUTTER_PAD;
  char buf[9];

  // pre terminate
  buf[digits] = '\0';

  for (int r = scroll_row; r < end_r; r++) {
    int y = layout.editor.y + (r - scroll_row) * FONT_H;
    /* Format line number right-to-left, then skip leading zeros. */
    int n = r + 1;
    for (int i = digits - 1; i >= 0; i--) { buf[i] = '0' + n % 10; n /= 10; }
    const char *p = buf;
    while (*p == '0' && *(p + 1)) p++;

    //gutter pad used as pixel offset
    int x = layout.gutter.x + layout.gutter.w - GUTTER_PAD - (int)strlen(p) * FONT_W;
    draw_string(x, y, p, COLOR_GUTTER_FG);
  }
}

static void draw_scrollbar() {
  int row_count = editor_get_row_count();

  bb_draw_rect(layout.scrollbar.x, layout.scrollbar.y, layout.scrollbar.w, layout.scrollbar.h, COLOR_GUTTER_BG);
  if (row_count <= 0) return;

  // Proportionality sizing
  int track_h = layout.scrollbar.h;
  int handle_h = (row_count <= vis_rows) ? track_h : vis_rows * track_h / row_count;

  //avoids invisible handle on huge files
  if (handle_h < SCROLLBAR_HANDLE_MIN) {handle_h = SCROLLBAR_HANDLE_MIN;}
  if (handle_h > track_h) {handle_h = track_h;}

  // clamp to 1 to aboid div by zero 
  int max_scroll = row_count > vis_rows ? row_count - vis_rows : 1;
  int max_handle_y = layout.scrollbar.y + track_h - handle_h;
  int handle_y = layout.scrollbar.y + editor_get_scroll_row() * (max_handle_y - layout.scrollbar.y) / max_scroll;

  bb_draw_rect(layout.scrollbar.x + 2, handle_y, layout.scrollbar.w - 4, handle_h, COLOR_GUTTER_FG);
}

static void draw_line_colored(int x, int y, const char *line, int scroll_col,
                              const uint32_t *colors, int line_len) {
  int draw_x = x;
  for (int c = scroll_col; c < line_len; c++) {
    draw_char(draw_x, y, line[c], colors[c]);
    draw_x += FONT_W;
  }
}

static void draw_text_lines(int scroll_row, int end_r, int scroll_col) {
  for (int r = scroll_row; r < end_r; r++) {
    int y = EDITOR_Y + (r - scroll_row) * FONT_H;
    int len = editor_get_line_len(r);
    if (len <= scroll_col) continue;
    const uint32_t *colors = highlight_cache_get_line(r);
    
    //Silent skip on cache allocation faulure
    if (!colors) continue;
    draw_line_colored(layout.editor.x, y, editor_get_line(r), scroll_col, colors, len);
  }
}


// Render pipeline

static void render_editor_ui(int mode, int col, int row, int scroll_row, int scroll_col) {
  int end_r = scroll_row + vis_rows;
  if (end_r > editor_get_row_count()) end_r = editor_get_row_count();

  switch (mode) {
    case RENDER_FULL:
      highlight_cache_sync(editor_get_row_count(), row);

      scene_update_layout();
      editor_set_viewport(vis_rows, vis_cols);
      bb_clear(COLOR_BG);

      // recompute end_r: scene_update_layout may have changed vis_rows
      end_r = scroll_row + vis_rows;
      if (end_r > editor_get_row_count()){end_r = editor_get_row_count();}

      //Draws
      if (filetree_is_visible()){draw_filetree(vis_rows);}
      draw_gutter(scroll_row, end_r);
      if (editor_sel_is_active()){draw_selection_bg(end_r);}
      draw_text_lines(scroll_row, end_r, scroll_col);
      draw_cursor(col, row);
      draw_remote_cursor(editor_get_remote_cursor_col(), editor_get_remote_cursor_row());
      draw_scrollbar();
      render_status_bar();
      break;

    case RENDER_LINE: {
      // free before rebuild so oversized buffer from now shorter line gets released
      highlight_cache_free_row(row);
      highlight_cache_rebuild_from(row);
      int y = layout.editor.y + (row - scroll_row) * FONT_H;

      //reset line
      bb_draw_rect(layout.editor.x, y, layout.editor.w, FONT_H, COLOR_BG);

      //redraw new line
      int len = editor_get_line_len(row);
      if (len > scroll_col) {
        const uint32_t *colors = highlight_cache_get_line(row);
        if (colors) draw_line_colored(layout.editor.x, y, editor_get_line(row), scroll_col, colors, len);
      }
      draw_cursor(col, row);
      break;
    }

    case RENDER_REMOTE_LINE:{
      int r_row = editor_get_remote_cursor_row();
      int r_col = editor_get_remote_cursor_col();
      int y = layout.editor.y + (r_row - scroll_row) * FONT_H;

      //reset line
      bb_draw_rect(layout.editor.x, y, layout.editor.w, FONT_H, COLOR_BG);

      //redraw new line
      const char *line = editor_get_line(r_row);
      if ((int)strlen(line) > scroll_col) draw_string(layout.editor.x, y, line + scroll_col, COLOR_TEXT);

      draw_remote_cursor(r_col, r_row);
      break;
    }

    case RENDER_WORD: {
      // cursor moved left: prev_col is the old position; col new one
      highlight_cache_rebuild_from(prev_row);
      const char *line = editor_get_line(prev_row);
      int len = editor_get_line_len(prev_row);
      int wy = model_to_py(prev_row);

      //clear range
      bb_draw_rect(model_to_px(col), wy, (prev_col - col + 1) * FONT_W, FONT_H, COLOR_BG);

      //tokenize once, then draw all chars in range
      const uint32_t *colors = highlight_cache_get_line(prev_row);
      if (colors) {
        for (int c = col; c <= prev_col && c < len; c++)
          draw_char(model_to_px(c), wy, line[c], colors[c]);
      }
      draw_cursor(col, row);
      break;
    }

    case RENDER_CHAR: {
      bool prev_vis = (prev_row >= scroll_row && prev_row < scroll_row + vis_rows &&
                       prev_col >= scroll_col && prev_col < scroll_col + vis_cols);
      bool curr_vis = (row >= scroll_row && row < scroll_row + vis_rows &&
                       col >= scroll_col && col < scroll_col + vis_cols);
      if (prev_vis) {
        highlight_cache_rebuild_from(prev_row);
        int cx = model_to_px(prev_col);
        int cy = model_to_py(prev_row);
        bb_draw_rect(cx, cy, FONT_W, FONT_H, COLOR_BG);
        int len = editor_get_line_len(prev_row);
        if (prev_col < len) {
          const uint32_t *colors = highlight_cache_get_line(prev_row);
          if (colors) draw_char(cx, cy, editor_get_line(prev_row)[prev_col], colors[prev_col]);
        }
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
  switch (mode) {
    case RENDER_STATUS:
      flip_status_bar();
      break;
    case RENDER_LINE:
      vg_flip_region(layout.editor.x, EDITOR_Y + (row - scroll_row) * FONT_H,
                     layout.editor.w, FONT_H);
      break;
    case RENDER_REMOTE_LINE:{
      int r_row = editor_get_remote_cursor_row();
      vg_flip_region(layout.editor.x, EDITOR_Y + (r_row - scroll_row) * FONT_H,
                     layout.editor.w, FONT_H);
      break;
    }  
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


  // Order matters: hide erases sprite from bb; show redraws it after.
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
  //checks if click is inside editor area
  if (px < layout.editor.x || px >= layout.scrollbar.x) return false;
  if (py < layout.editor.y || py >= layout.status_bar.y) return false;

  int col = (px - layout.editor.x) / FONT_W + editor_get_scroll_col();
  int row = (py - EDITOR_Y) / FONT_H + editor_get_scroll_row();

  if (row < 0) row = 0;
  if (row >= editor_get_row_count()) row = editor_get_row_count() - 1;
  if (col < 0) col = 0;

  int len = editor_get_line_len(row);
  if (col > len) col = len;
  *out_row = row;
  *out_col = col;
  return true;
}

bool scene_click_scrollbar(int px, int py) {
  //checks if click is inside scrollbar area
  if (px < layout.scrollbar.x || px >= layout.scrollbar.x + layout.scrollbar.w) return false;
  if (py < layout.scrollbar.y || py >= layout.scrollbar.y + layout.scrollbar.h) return false;

  int row_count = editor_get_row_count();
  int max_scroll = row_count > vis_rows ? row_count - vis_rows : 0;
  int target = max_scroll > 0 ? (py - layout.scrollbar.y) * max_scroll / layout.scrollbar.h : 0;
  
  editor_scroll_by(target - editor_get_scroll_row(), 0);
  if (editor_consume_scroll_dirty()) set_render(RENDER_FULL);
  return true;
}

bool scene_px_to_filetree(int px, int py, int *out_index) {
  if (!filetree_is_visible()) return false;
  if (px < 0 || px >= layout.filetree.w) return false;
  if (py < EDITOR_Y) return false;

  int index = filetree_get_scroll() + (py - EDITOR_Y) / FONT_H;
  if (index >= filetree_get_count()) {
    *out_index = -1;
    return true;
  }

  *out_index = index;
  return true;
}
