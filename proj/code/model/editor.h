#pragma once
#include <stdbool.h>

typedef enum {
  EDITOR_OK,
  EDITOR_ERR_NO_CLIPBOARD,
  EDITOR_ERR_DOCUMENT_FULL
} EditorResult;

#define MAX_LINES 500
#define MAX_COLS 256

int editor_init();
void editor_cleanup();

void editor_set_viewport(int rows, int cols);
void editor_scroll_by(int drow, int dcol);
void editor_set_cursor(int row, int col);
bool editor_consume_scroll_dirty();

void editor_insert_char(char c);
void editor_delete_char();
void editor_delete_word();

void editor_move_left();
void editor_move_right();
void editor_move_up();
void editor_move_down();
void editor_move_word_left();
void editor_move_word_right();
void editor_move_home();
void editor_move_end();

const char *editor_get_line(int row);
int editor_get_row_count();

int editor_get_cursor_row();
int editor_get_cursor_col();

int editor_get_scroll_row();
int editor_get_scroll_col();

void editor_delete_selection();
void editor_sel_set_anchor();
void editor_sel_clear();
bool editor_sel_is_active();
void editor_sel_get_range(int *start_row, int *start_col, int *end_row, int *end_col);
bool editor_consume_sel_dirty();

void editor_copy_selection();
EditorResult editor_paste();

// Remote 

void editor_set_remote_cursor(int row, int col);
void editor_remote_insert_char(char c);
void editor_remote_delete_char();
int editor_get_remote_cursor_row();
int editor_get_remote_cursor_col();
