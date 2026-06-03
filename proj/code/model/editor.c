#include "model/editor.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

#define LINE_BUF_INIT_CAP 32
#define LINES_INIT_CAP 16

static Line *lines = NULL;
static int row_cap = 0;
static int cursor_row = 0;
static int cursor_col = 0;
static int row_count = 1;

static int scroll_row = 0;
static int scroll_col = 0;
static int visible_rows = 0;
static int visible_cols = 0;
static bool scroll_dirty = false;

static bool sel_active = false;
static int sel_anchor_row = 0;
static int sel_anchor_col = 0;
static bool sel_dirty = false;

static char *clipboard = NULL;

static int line_ensure_cap(Line *ln, int needed) {
  // already enough linesize, nothing to do
  if (ln->cap > needed) return 0;

  // new needed size calc
  int new_cap = ln->cap ? ln->cap * 2 : LINE_BUF_INIT_CAP;
  while (new_cap <= needed) new_cap *= 2;

  char *new_buf = malloc(new_cap);
  if (!new_buf) return -1;
  if (ln->buf) {
    memcpy(new_buf, ln->buf, ln->len + 1);
  } 
  else {
    new_buf[0] = '\0';
  }
  free(ln->buf);
  ln->buf = new_buf;
  ln->cap = new_cap;
  return 0;
}


static int lines_ensure_cap(int needed) {
  // already enough lines, nothing to do
  if (row_cap > needed) return 0;

  // new needed total lines calc
  int new_cap = row_cap ? row_cap * 2 : LINES_INIT_CAP;
  while (new_cap <= needed) new_cap *= 2;
  Line *new_arr = malloc(new_cap * sizeof(Line));
  if (!new_arr) return -1;
  if (lines) {
    memcpy(new_arr, lines, row_count * sizeof(Line));
  }
  free(lines);
  lines = new_arr;
  row_cap = new_cap;
  return 0;
}

void editor_cleanup() {
  if (lines){
    for (int i = 0; i < row_count; i++) free(lines[i].buf);
    free(lines);
    lines = NULL;
    row_cap = 0;
  }
  if (clipboard){
    free(clipboard);
    clipboard = NULL;
  }
}

int editor_init() {
  editor_cleanup();

  row_count = 1;
  if (lines_ensure_cap(LINES_INIT_CAP) != 0) return -1;
  lines[0] = (Line){NULL, 0, 0};
  if (line_ensure_cap(&lines[0], 0) != 0) return -1;

  cursor_row = 0;
  cursor_col = 0;
  scroll_row = 0;
  scroll_col = 0;
  scroll_dirty = false;
  sel_active = false;
  sel_dirty = false;
  return 0;
}

void editor_set_viewport(int rows, int cols) {
  visible_rows = rows;
  visible_cols = cols;
}

/* Adjusts scroll offsets to keep the cursor inside the visible viewport.
 * Called after every cursor move; sets scroll_dirty if anything changed. */
static void clamp_scroll() {
  if (cursor_row < scroll_row) {
    scroll_row = cursor_row;
    scroll_dirty = true;
  } else if (cursor_row >= scroll_row + visible_rows) {
    scroll_row = cursor_row - visible_rows + 1;
    scroll_dirty = true;
  }

  if (cursor_col < scroll_col) {
    scroll_col = cursor_col;
    scroll_dirty = true;
  } else if (cursor_col >= scroll_col + visible_cols) {
    scroll_col = cursor_col - visible_cols + 1;
    scroll_dirty = true;
  }

  if (sel_active) sel_dirty = true;
}

void editor_scroll_by(int drow, int dcol) {
  int new_row = scroll_row + drow;
  int new_col = scroll_col + dcol;

  if (new_row < 0) new_row = 0;
  if (new_row > row_count - 1) new_row = row_count - 1;
  if (new_col < 0) new_col = 0;

  if (new_row != scroll_row || new_col != scroll_col) {
    scroll_row = new_row;
    scroll_col = new_col;
    scroll_dirty = true;
  }
}

void editor_set_cursor(int row, int col) {
  if (row < 0) row = 0;
  if (row >= row_count) row = row_count - 1;
  int len = lines[row].len;
  if (col < 0) col = 0;
  if (col > len) col = len;
  cursor_row = row;
  cursor_col = col;
  clamp_scroll();
}

bool editor_consume_scroll_dirty() {
  bool dirty = scroll_dirty;
  scroll_dirty = false;
  return dirty;
}

EditorResult editor_insert_char(char c) {
  if (c == '\n') {
    //add new line allocation if needed
    if (lines_ensure_cap(row_count + 1) != 0){
      return EDITOR_ERR_ALLOC_FAILED;
    }

    //  Shifts all Line structs below the cursor down by one slot to open a gap at cursor_row + 1.
    memmove(lines + cursor_row + 2, lines + cursor_row + 1, (row_count - cursor_row - 1) * sizeof(Line));

    int split = cursor_col;
    int tail_len = lines[cursor_row].len - split;

    Line *new_line = &lines[cursor_row + 1];
    *new_line = (Line){NULL, 0, 0};

    //make sure newline can hold the tail
    if (line_ensure_cap(new_line, tail_len) != 0){
      return EDITOR_ERR_ALLOC_FAILED;
    }
    memcpy(new_line->buf, lines[cursor_row].buf + split, tail_len);
    new_line->buf[tail_len] = '\0';
    new_line->len = tail_len;

    lines[cursor_row].buf[split] = '\0';
    lines[cursor_row].len = split;

    row_count++;
    cursor_row++; cursor_col = 0;
    clamp_scroll();
    return EDITOR_OK;
  }

  //not newline char
  if (line_ensure_cap(&lines[cursor_row], lines[cursor_row].len + 2) != 0){
    return EDITOR_ERR_ALLOC_FAILED;
  }

  //move chars right
  memmove(&lines[cursor_row].buf[cursor_col + 1], &lines[cursor_row].buf[cursor_col], lines[cursor_row].len - cursor_col + 1);

  //add char
  lines[cursor_row].buf[cursor_col] = c;
  cursor_col++;
  lines[cursor_row].len++;
  clamp_scroll();
  return EDITOR_OK;
}

EditorResult editor_delete_char() {
  if (cursor_col > 0) {
    cursor_col--;
    memmove(&lines[cursor_row].buf[cursor_col], &lines[cursor_row].buf[cursor_col + 1],
            lines[cursor_row].len - cursor_col);
    lines[cursor_row].len--;
  } 

  //Cursor at col 0 not on first row - line merge
  else if (cursor_row > 0) {
    int prev_len = lines[cursor_row - 1].len;
    int curr_len = lines[cursor_row].len;

    //grow prev line for merged content
    if (line_ensure_cap(&lines[cursor_row - 1], prev_len + curr_len + 1) != 0){
      return EDITOR_ERR_ALLOC_FAILED;
    }

    //append line
    memcpy(lines[cursor_row - 1].buf + prev_len, lines[cursor_row].buf, curr_len + 1);
    lines[cursor_row - 1].len = prev_len + curr_len;

    free(lines[cursor_row].buf);

    //shift lines back
    memmove(lines + cursor_row, lines + cursor_row + 1, (row_count - cursor_row - 1) * sizeof(Line));

    //zero out last line and update counts
    lines[row_count - 1] = (Line){NULL, 0, 0};
    row_count--;
    cursor_row--; 
    cursor_col = prev_len;
  }


  clamp_scroll();
  return EDITOR_OK;
}

EditorResult editor_delete_word() {
  if (cursor_col == 0) {
    return EDITOR_OK;
  }

  int end = cursor_col;
  while (cursor_col > 0 && lines[cursor_row].buf[cursor_col - 1] == ' ') cursor_col--;
  while (cursor_col > 0 && lines[cursor_row].buf[cursor_col - 1] != ' ') cursor_col--;

  //shift from end (right bound) to cursor_col (left bound)
  memmove(&lines[cursor_row].buf[cursor_col], &lines[cursor_row].buf[end], lines[cursor_row].len - end + 1);
  
  lines[cursor_row].len -= end - cursor_col;
  clamp_scroll();
  return EDITOR_OK;
}

void editor_move_left() {
  if (cursor_col > 0){
    cursor_col--;
  }
  else if (cursor_row > 0){
    cursor_row--; 
    cursor_col = lines[cursor_row].len;
  }
  clamp_scroll();
}

void editor_move_right() {
  int len = lines[cursor_row].len;
  if (cursor_col < len) cursor_col++;
  else if (cursor_row < row_count - 1) { cursor_row++; cursor_col = 0; }
  clamp_scroll();
}

void editor_move_up() {
  if (cursor_row > 0) {
    cursor_row--;
    if (cursor_col > lines[cursor_row].len) {
      cursor_col = lines[cursor_row].len;
    }
  }
  clamp_scroll();
}

void editor_move_down() {
  if (cursor_row < row_count - 1) {
    cursor_row++;
    if (cursor_col > lines[cursor_row].len) {
      cursor_col = lines[cursor_row].len;
    }
  }
  clamp_scroll();
}

void editor_move_word_left() {
  while (cursor_col > 0 && lines[cursor_row].buf[cursor_col - 1] == ' '){
    cursor_col--;
  }
  while (cursor_col > 0 && lines[cursor_row].buf[cursor_col - 1] != ' '){
    cursor_col--;
  }
  clamp_scroll();
}

void editor_move_word_right() {
  int len = lines[cursor_row].len;
  while (cursor_col < len && lines[cursor_row].buf[cursor_col] != ' '){
    cursor_col++;
  }
  while (cursor_col < len && lines[cursor_row].buf[cursor_col] == ' '){
    cursor_col++;
  }
  clamp_scroll();
}

void editor_move_home() {
  cursor_col = 0;
  clamp_scroll();
}

void editor_move_end() {
  cursor_col = lines[cursor_row].len;
  clamp_scroll();
}

const char *editor_get_line(int row) {
  if (row < 0 || row >= row_count) return "";
  return lines[row].buf;
}

int editor_get_row_count() { return row_count; }
int editor_get_cursor_row() { return cursor_row; }
int editor_get_cursor_col() { return cursor_col; }
int editor_get_scroll_row() { return scroll_row; }
int editor_get_scroll_col() { return scroll_col; }

//Standardizes range so it starts before it ends
void editor_sel_get_range(int *start_row, int *start_col, int *end_row, int *end_col) {
  bool cursor_after = (cursor_row > sel_anchor_row) || (cursor_row == sel_anchor_row && cursor_col >= sel_anchor_col);
  if (cursor_after) {
    *start_row = sel_anchor_row; 
    *start_col = sel_anchor_col;
    *end_row = cursor_row; 
    *end_col = cursor_col;
  } 
  else {
    *start_row = cursor_row; 
    *start_col = cursor_col;
    *end_row = sel_anchor_row; 
    *end_col = sel_anchor_col;
  }
}

EditorResult editor_delete_selection() {
  if (!sel_active) return EDITOR_OK;
  int start_row, start_col, end_row, end_col;
  editor_sel_get_range(&start_row, &start_col, &end_row, &end_col);

  //Single line selection
  if (start_row == end_row) {
    int len = lines[start_row].len;
    memmove(&lines[start_row].buf[start_col], &lines[start_row].buf[end_col], len - end_col + 1);
    lines[start_row].len -= end_col - start_col;
  } 
  //Multi-line selection 
  else {
    int tail_len = lines[end_row].len - end_col;

    //Grow line (if needed) to merge tail
    if (line_ensure_cap(&lines[start_row], start_col + tail_len + 1) != 0){
      return EDITOR_ERR_ALLOC_FAILED;
    }
    //append tail
    memcpy(lines[start_row].buf + start_col, lines[end_row].buf + end_col, tail_len + 1);
    lines[start_row].len = start_col + tail_len;


    int removed = end_row - start_row;
    for (int r = start_row + 1; r <= end_row; r++) {
      free(lines[r].buf);
    }
    memmove(lines + start_row + 1, lines + end_row + 1, (row_count - end_row - 1) * sizeof(Line));

    //Zero out now-unused lines at the end
    memset(lines + row_count - removed, 0, removed * sizeof(Line));
    row_count -= removed;
  }

  cursor_row = start_row;
  cursor_col = start_col;
  sel_active = false;
  sel_dirty = true;
  clamp_scroll();
  return EDITOR_OK;
}

void editor_sel_set_anchor() {
  sel_anchor_row = cursor_row;
  sel_anchor_col = cursor_col;
  sel_active = true;
  sel_dirty = true;
}

void editor_sel_clear() {
  if (sel_active) sel_dirty = true;
  sel_active = false;
}

bool editor_sel_is_active() { return sel_active; }



bool editor_consume_sel_dirty() {
  bool d = sel_dirty;
  sel_dirty = false;
  return d;
}


/* Both loops use the same logic: the first row starts at
  * start_col, the last row ends at end_col, middle rows span the full
  * line. */

void editor_copy_selection() {
  if (!sel_active) return;
  int start_row, start_col, end_row, end_col;
  editor_sel_get_range(&start_row, &start_col, &end_row, &end_col);

  //measure how many bytes to allocate
  int size = 0;
  for (int r = start_row; r <= end_row; r++) {
    int col_start = (r == start_row) ? start_col : 0;
    int col_end = (r == end_row) ? end_col : lines[r].len;
    size += (col_end - col_start) + (r < end_row ? 1 : 0); /* +1 for '\n' between rows */
  }
  // \0 terminator
  size++; 

  if (clipboard != NULL) free(clipboard);
  clipboard = malloc(size);
  if (clipboard == NULL) return;

  //Fill buffer
  int idx = 0;
  for (int r = start_row; r <= end_row; r++) {
    int col_start = (r == start_row) ? start_col : 0;
    int col_end = (r == end_row) ? end_col : lines[r].len;
    int len = col_end - col_start;
    memcpy(&clipboard[idx], lines[r].buf + col_start, len);
    idx += len;
    // no \n in last line
    if (r < end_row) clipboard[idx++] = '\n';
  }
  clipboard[idx] = '\0';
}

EditorResult editor_paste() {
  if (clipboard == NULL) return EDITOR_ERR_NO_CLIPBOARD;

  // save tail
  int tail_len = lines[cursor_row].len - cursor_col;
  char *tail = malloc(tail_len + 1);
  if (!tail) return EDITOR_ERR_ALLOC_FAILED;

  memcpy(tail, lines[cursor_row].buf + cursor_col, tail_len + 1);
  lines[cursor_row].buf[cursor_col] = '\0';
  lines[cursor_row].len = cursor_col;

  int dest_row = cursor_row;
  int dest_col = cursor_col;
  const char *p = clipboard;

  while (*p) {
    if (*p == '\n') {
      //Make sure newline can be added
      if (lines_ensure_cap(row_count + 1) != 0) {
        free(tail);
        return EDITOR_ERR_ALLOC_FAILED;
      }

      //Shift down
      memmove(lines + dest_row + 2, lines + dest_row + 1, (row_count - dest_row - 1) * sizeof(Line));

      Line *new_line = &lines[dest_row + 1];
      *new_line = (Line){NULL, 0, 0};
      if (line_ensure_cap(new_line, 0) != 0) {
        free(tail);
        return EDITOR_ERR_ALLOC_FAILED;
      }

      row_count++;
      dest_row++; dest_col = 0;
      p++;
    } 

    else {
      const char *seg_end = p;

      //Find next new line
      while (*seg_end && *seg_end != '\n') seg_end++;
      int seg_len = (int)(seg_end - p);

      //ensure entire line fits
      if (line_ensure_cap(&lines[dest_row], dest_col + seg_len + 1) != 0) {
        free(tail); return EDITOR_ERR_ALLOC_FAILED;
      }
      //move segment
      memcpy(lines[dest_row].buf + dest_col, p, seg_len);
      dest_col += seg_len;
      lines[dest_row].buf[dest_col] = '\0';
      lines[dest_row].len = dest_col;
      p = seg_end;
    }
  }

  // Append tail
  if (line_ensure_cap(&lines[dest_row], dest_col + tail_len + 1) != 0) {
    free(tail); 
    return EDITOR_ERR_ALLOC_FAILED;
  }
  memcpy(lines[dest_row].buf + dest_col, tail, tail_len + 1);
  lines[dest_row].len = dest_col + tail_len;

  free(tail);
  cursor_row = dest_row;
  cursor_col = dest_col;
  clamp_scroll();
  return EDITOR_OK;
}

EditorResult editor_load_line(const char *text, int len) {
  //grows last line to fit new text
  if (line_ensure_cap(&lines[row_count - 1], len) != 0) return EDITOR_ERR_ALLOC_FAILED;

  memcpy(lines[row_count - 1].buf, text, len + 1);
  lines[row_count - 1].len = len;

  //Ensures theres an empty line at EOF
  if (lines_ensure_cap(row_count + 1) != 0) return EDITOR_ERR_ALLOC_FAILED;
  lines[row_count] = (Line){NULL, 0, 0};
  row_count++;
  return EDITOR_OK;
}

void editor_load_finalize() {
  if (row_count > 1 && lines[row_count - 1].len == 0)
    row_count--;
}
