#include "editor.h"
#include <string.h>
#include <stdbool.h>
#include <stdlib.h>

static char lines[MAX_LINES][MAX_COLS];
static int cursor_row = 0;
static int cursor_col = 0;
static int row_count = 1;

static int scroll_row = 0;
static int scroll_col = 0;
static int visible_rows = MAX_LINES;
static int visible_cols = MAX_COLS;
static bool scroll_dirty = false;

static bool sel_active = false;
static int sel_anchor_row = 0;
static int sel_anchor_col = 0;
static bool sel_dirty = false;

static char *clipboard = NULL;

int editor_init() {
  memset(lines, 0, sizeof(lines));
  cursor_row = 0;
  cursor_col = 0;
  row_count = 1;
  scroll_row = 0;
  scroll_col = 0;
  scroll_dirty = false;
  sel_active = false;
  sel_dirty = false;
  return 0;
}

void editor_cleanup() {
  if (clipboard != NULL) { free(clipboard); clipboard = NULL; }
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
  int len = strlen(lines[row]);
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

void editor_insert_char(char c) {
  if (c == '\n') {
    if (row_count >= MAX_LINES) return;
    /* Shift all rows below the current one down by one to open a slot,
     * then copy from cursor onwards into the new slot and end the
     * current line at the split point. */
    memmove(lines[cursor_row + 2], lines[cursor_row + 1], (row_count - cursor_row - 1) * MAX_COLS);
    int split = cursor_col;
    int tail = strlen(lines[cursor_row]) - split;
    memcpy(lines[cursor_row + 1], &lines[cursor_row][split], tail + 1);
    lines[cursor_row][split] = '\0';
    row_count++;
    cursor_row++;
    cursor_col = 0;
    clamp_scroll();
    return;
  }

  int len = strlen(lines[cursor_row]);
  if (len >= MAX_COLS - 1) return;
  memmove(&lines[cursor_row][cursor_col + 1], &lines[cursor_row][cursor_col], len - cursor_col + 1);
  lines[cursor_row][cursor_col] = c;
  cursor_col++;
  clamp_scroll();
}

void editor_delete_char() {
  if (cursor_col > 0) {
    cursor_col--;
    int len = strlen(lines[cursor_row]);
    memmove(&lines[cursor_row][cursor_col], &lines[cursor_row][cursor_col + 1], len - cursor_col);
  } else if (cursor_row > 0) {
    /* Merge current line into the end of the previous one, then shift
     * all rows below up by one to close the gap. */
    int prev_len = strlen(lines[cursor_row - 1]);
    int curr_len = strlen(lines[cursor_row]);
    if (prev_len + curr_len >= MAX_COLS) return;
    memcpy(&lines[cursor_row - 1][prev_len], lines[cursor_row], curr_len + 1);
    memmove(lines[cursor_row], lines[cursor_row + 1], (row_count - cursor_row - 1) * MAX_COLS);
    memset(lines[row_count - 1], 0, MAX_COLS);
    row_count--;
    cursor_row--;
    cursor_col = prev_len;
  }
  clamp_scroll();
}

void editor_delete_word() {
  if (cursor_col == 0) return;
  int end = cursor_col;
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] == ' ') cursor_col--;
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] != ' ') cursor_col--;
  int len = strlen(lines[cursor_row]);
  memmove(&lines[cursor_row][cursor_col], &lines[cursor_row][end], len - end + 1);
  clamp_scroll();
}

void editor_move_left() {
  if (cursor_col > 0) cursor_col--;
  else if (cursor_row > 0) { cursor_row--; cursor_col = strlen(lines[cursor_row]); }
  clamp_scroll();
}

void editor_move_right() {
  int len = strlen(lines[cursor_row]);
  if (cursor_col < len) cursor_col++;
  else if (cursor_row < row_count - 1) { cursor_row++; cursor_col = 0; }
  clamp_scroll();
}

void editor_move_up() {
  if (cursor_row > 0) {
    cursor_row--;
    int len = strlen(lines[cursor_row]);
    if (cursor_col > len) cursor_col = len;
  }
  clamp_scroll();
}

void editor_move_down() {
  if (cursor_row < row_count - 1) {
    cursor_row++;
    int len = strlen(lines[cursor_row]);
    if (cursor_col > len) cursor_col = len;
  }
  clamp_scroll();
}

void editor_move_word_left() {
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] == ' ')
    cursor_col--;
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] != ' ')
    cursor_col--;
  clamp_scroll();
}

void editor_move_word_right() {
  int len = strlen(lines[cursor_row]);
  while (cursor_col < len && lines[cursor_row][cursor_col] != ' ')
    cursor_col++;
  while (cursor_col < len && lines[cursor_row][cursor_col] == ' ')
    cursor_col++;
  clamp_scroll();
}

void editor_move_home() {
  cursor_col = 0;
  clamp_scroll();
}

void editor_move_end() {
  cursor_col = strlen(lines[cursor_row]);
  clamp_scroll();
}

const char *editor_get_line(int row) {
  if (row < 0 || row >= MAX_LINES) return "";
  return lines[row];
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

void editor_delete_selection() {
  if (!sel_active) return;
  int start_row, start_col, end_row, end_col;
  editor_sel_get_range(&start_row, &start_col, &end_row, &end_col);

  if (start_row == end_row) {
    int len = strlen(lines[start_row]);
    memmove(&lines[start_row][start_col], &lines[start_row][end_col], len - end_col + 1);
  } 
  else {
    int tail_len = strlen(lines[end_row]) - end_col;
    if (start_col + tail_len < MAX_COLS){
      memcpy(&lines[start_row][start_col], &lines[end_row][end_col], tail_len + 1);
    }
    else {
      lines[start_row][start_col] = '\0';
    }
    int removed = end_row - start_row;
    memmove(lines[start_row + 1], lines[end_row + 1], (row_count - end_row - 1) * MAX_COLS);
    memset(lines[row_count - removed], 0, removed * MAX_COLS);
    row_count -= removed;
  }

  cursor_row = start_row;
  cursor_col = start_col;
  sel_active = false;
  sel_dirty = true;
  clamp_scroll();
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

  int size = 0;
  for (int r = start_row; r <= end_row; r++) {
    int col_start = (r == start_row) ? start_col : 0;
    int col_end = (r == end_row) ? end_col : (int)strlen(lines[r]);
    size += (col_end - col_start) + (r < end_row ? 1 : 0); /* +1 for '\n' between rows */
  }
  size++; /* '\0' */

  if (clipboard != NULL) free(clipboard);
  clipboard = malloc(size);
  if (clipboard == NULL) return;

  int idx = 0;
  for (int r = start_row; r <= end_row; r++) {
    int col_start = (r == start_row) ? start_col : 0;
    int col_end = (r == end_row) ? end_col : (int)strlen(lines[r]);
    int len = col_end - col_start;
    memcpy(&clipboard[idx], &lines[r][col_start], len);
    idx += len;
    if (r < end_row) clipboard[idx++] = '\n'; /* no trailing newline after last row */
  }
  clipboard[idx] = '\0';
}

EditorResult editor_paste() {
  if (clipboard == NULL) return EDITOR_ERR_NO_CLIPBOARD;

  int tail_len = strlen(lines[cursor_row]) - cursor_col;

  /* Count rows */
  int rows_needed = 1;
  int col = cursor_col;
  for (int i = 0; clipboard[i]; i++) {
    if (clipboard[i] == '\n') {
      rows_needed++; col = 0;
    }
    else if (++col >= MAX_COLS - 1) {
      rows_needed++; col = 0;
    }
  }
  /* col now holds the final dest_col after pass 2; if the tail won't fit
   * there, reserve one extra row so it never gets dropped. */
  if (col + tail_len >= MAX_COLS) rows_needed++;

  if (row_count + rows_needed - 1 > MAX_LINES) return EDITOR_ERR_DOCUMENT_FULL;

  /* Save text after cursor of current line */
  char tail[MAX_COLS];
  memcpy(tail, &lines[cursor_row][cursor_col], tail_len + 1);
  lines[cursor_row][cursor_col] = '\0';

  //Opens rows_needed -1 lines of free space right after the cursor, by moving lines after cursor down
  //Zeroes out new lines. 
  if (rows_needed > 1) {
    memmove(lines[cursor_row + rows_needed], lines[cursor_row + 1], (row_count - cursor_row - 1) * MAX_COLS);
    memset(lines[cursor_row + 1], 0, (rows_needed - 1) * MAX_COLS);
  }


  int dest_row = cursor_row;
  int dest_col = cursor_col;
  const char *p = clipboard;

  //Divide em segmentos - Ou até maximizar a linha ou então '\n'
  while (*p) {
    const char *seg_end = p;
    while (*seg_end && *seg_end != '\n') seg_end++;
    int seg_len = (int)(seg_end - p);

    while (seg_len > 0) {
      int space = MAX_COLS - 1 - dest_col;
      int copy_len = seg_len < space ? seg_len : space;
      memcpy(&lines[dest_row][dest_col], p, copy_len);
      lines[dest_row][dest_col + copy_len] = '\0';
      dest_col += copy_len;
      p += copy_len;
      seg_len -= copy_len;
      if (seg_len > 0) { dest_row++; dest_col = 0; } /* overflow split */
    }

    if (*p == '\n') { dest_row++; dest_col = 0; p++; }
  }

  /* Append the saved tail; overflow to the reserved row if it doesn't fit. */
  if (dest_col + tail_len >= MAX_COLS) { dest_row++; dest_col = 0; }
  memcpy(&lines[dest_row][dest_col], tail, tail_len + 1);

  cursor_row = dest_row;
  cursor_col = dest_col;
  row_count += rows_needed - 1;
  clamp_scroll();
  return EDITOR_OK;
}
