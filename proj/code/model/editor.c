#include "editor.h"
#include <string.h>

static char lines[MAX_LINES][MAX_COLS];
static int cursor_row = 0;
static int cursor_col = 0;
static int row_count = 1;

int editor_init() {
  memset(lines, 0, sizeof(lines));
  cursor_row = 0;
  cursor_col = 0;
  row_count = 1;
  return 0;
}

void editor_cleanup() {}

void editor_insert_char(char c) {
  if (c == '\n') {
    if (cursor_row >= MAX_LINES - 1) return;
    cursor_row++;
    cursor_col = 0;
    if (cursor_row >= row_count) row_count = cursor_row + 1;
    return;
  }

  int len = strlen(lines[cursor_row]);
  if (len >= MAX_COLS - 1) return;
  memmove(&lines[cursor_row][cursor_col + 1], &lines[cursor_row][cursor_col], len - cursor_col + 1);
  lines[cursor_row][cursor_col] = c;
  cursor_col++;
}

void editor_delete_char() {
  if (cursor_col > 0) {
    cursor_col--;
    int len = strlen(lines[cursor_row]);
    memmove(&lines[cursor_row][cursor_col], &lines[cursor_row][cursor_col + 1], len - cursor_col);
  } else if (cursor_row > 0) {
    cursor_row--;
    cursor_col = strlen(lines[cursor_row]);
  }
}

void editor_delete_word() {
  if (cursor_col == 0) return;
  int end = cursor_col;
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] == ' ') cursor_col--;
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] != ' ') cursor_col--;
  int len = strlen(lines[cursor_row]);
  memmove(&lines[cursor_row][cursor_col], &lines[cursor_row][end], len - end + 1);
}

void editor_move_left() {
  if (cursor_col > 0) cursor_col--;
  else if (cursor_row > 0) { cursor_row--; cursor_col = strlen(lines[cursor_row]); }
}

void editor_move_right() {
  int len = strlen(lines[cursor_row]);
  if (cursor_col < len) cursor_col++;
  else if (cursor_row < row_count - 1) { cursor_row++; cursor_col = 0; }
}

void editor_move_up() {
  if (cursor_row > 0) {
    cursor_row--;
    int len = strlen(lines[cursor_row]);
    if (cursor_col > len) cursor_col = len;
  }
}

void editor_move_down() {
  if (cursor_row < row_count - 1) {
    cursor_row++;
    int len = strlen(lines[cursor_row]);
    if (cursor_col > len) cursor_col = len;
  }
}

void editor_move_word_left() {
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] == ' ')
    cursor_col--;
  while (cursor_col > 0 && lines[cursor_row][cursor_col - 1] != ' ')
    cursor_col--;
}

void editor_move_word_right() {
  int len = strlen(lines[cursor_row]);
  while (cursor_col < len && lines[cursor_row][cursor_col] != ' ')
    cursor_col++;
  while (cursor_col < len && lines[cursor_row][cursor_col] == ' ')
    cursor_col++;
}

const char *editor_get_line(int row) {
  if (row < 0 || row >= MAX_LINES) return "";
  return lines[row];
}

int editor_get_row_count() { return row_count; }
int editor_get_cursor_row() { return cursor_row; }
int editor_get_cursor_col() { return cursor_col; }
