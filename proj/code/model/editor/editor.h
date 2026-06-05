#pragma once
#include <stdbool.h>

/**
 * @file editor.h
 * @brief Handles the editor: Text buffer, cursor, scroll, selection, clipboard, and remote peer state
 */

typedef enum {
  EDITOR_OK,
  EDITOR_ERR_NO_CLIPBOARD,
  EDITOR_ERR_ALLOC_FAILED
} EditorResult;

/** @brief A single line of text with a growable buffer */
typedef struct {
  char *buf;
  int len;
  int cap;
} Line;

/**
 * @brief Initializes the editor with an empty buffer
 * @return 0 on success. Non-zero on failure
 */
int editor_init();

/**
 * @brief Releases all resources owned by the editor
 */
void editor_cleanup();

/**
 * @brief Sets the visible area so the editor can clamp scroll and cursor
 * @param rows Number of visible text rows
 * @param cols Number of visible text columns
 */
void editor_set_viewport(int rows, int cols);

/**
 * @brief Scrolls the view by a row and column delta
 * @param drow Delta rows (positive = down)
 * @param dcol Delta Columns (positive = right)
 */
void editor_scroll_by(int drow, int dcol);

/**
 * @brief Moves the cursor to an absolute position.
 * @param row Target row (0-based)
 * @param col Target column (0-based)
 */
void editor_set_cursor(int row, int col);

/**
 * @brief Returns and clears the scroll-dirty flag
 *
 * Set whenever the scroll position changes. Callers use this to trigger a full redraw
 * @return true if the scroll position changed since the last call
 */
bool editor_consume_scroll_dirty();

/**
 * @brief Inserts a character at the cursor position and advances the cursor
 * @param c Character to insert
 * @return EDITOR_OK on success; EDITOR_ERR_ALLOC_FAILED on allocation failure
 */
EditorResult editor_insert_char(char c);

/**
 * @brief Deletes the character before the cursor (backspace)
 * @return EDITOR_OK on success; EDITOR_ERR_ALLOC_FAILED on allocation failure
 */
EditorResult editor_delete_char();

/**
 * @brief Deletes the word before the cursor
 * @return EDITOR_OK on success; EDITOR_ERR_ALLOC_FAILED on allocation failure
 */
EditorResult editor_delete_word();

/** @brief Moves the cursor one column to the left, wrapping to the previous line */
void editor_move_left();

/** @brief Moves the cursor one column to the right, wrapping to the next line */
void editor_move_right();

/** @brief Moves the cursor one row up, clamped to the first row */
void editor_move_up();

/** @brief Moves the cursor one row down, clamped to the last row */
void editor_move_down();

/** @brief Moves the cursor to the start of the previous word */
void editor_move_word_left();

/** @brief Moves the cursor to the start of the next word */
void editor_move_word_right();

/** @brief Moves the cursor to column 0 of the current row */
void editor_move_home();

/** @brief Moves the cursor past the last character of the current row */
void editor_move_end();

/**
 * @brief Used to acces tetx in a line
 * @param row Line index (0-based)
 * @return Pointer to the line's character buffer. Not null-terminated
 */
const char *editor_get_line(int row);

/**
 * @brief Returns the number of characters in a line
 * @param row Line index (0-based)
 * @return Character count, excluding null terminator
 */
int editor_get_line_len(int row);

/**
 * @brief Returns the total number of lines in the buffer
 * @return Row count
 */
int editor_get_row_count();

/**
 * @brief Returns the cursor's current row
 * @return Row index (0-based)
 */
int editor_get_cursor_row();

/**
 * @brief Returns the cursor's current column
 * @return Column index (0-based)
 */
int editor_get_cursor_col();

/**
 * @brief Returns the first visible row
 * @return Scroll row offset (0-based)
 */
int editor_get_scroll_row();

/**
 * @brief Returns the first visible column
 * @return Scroll column offset (0-based)
 */
int editor_get_scroll_col();

/**
 * @brief Deletes all text covered by the active selection
 * @return EDITOR_OK on success; EDITOR_ERR_ALLOC_FAILED on allocation failure
 */
EditorResult editor_delete_selection();

/**
 * @brief Sets the selection anchor at the current cursor position
 *
 * Subsequent cursor moves extend the selection from this anchor
 */
void editor_sel_set_anchor();

/** @brief Clears the active selection without modifying the buffer */
void editor_sel_clear();

/**
 * @brief Returns whether a selection is currently active
 * @return true if a selection anchor has been set and the selection is non-empty
 */
bool editor_sel_is_active();

/**
 * @brief Returns the selection range in order (start before end)
 * @param start_row first row of the selection
 * @param start_col first column of the selection
 * @param end_row last row of the selection
 * @param end_col last column of the selection
 */
void editor_sel_get_range(int *start_row, int *start_col, int *end_row, int *end_col);

/**
 * @brief Returns and clears the selection-dirty flag.
 *
 * Set whenever the selection changes. Callers use this to trigger a redraw
 * @return true if the selection changed since the last call
 */
bool editor_consume_sel_dirty();

/** @brief Copies the active selection text to the internal clipboard */
void editor_copy_selection();

/**
 * @brief Inserts the clipboard contents at the cursor position
 * @return EDITOR_OK on success; EDITOR_ERR_NO_CLIPBOARD if the clipboard is empty;
 *         EDITOR_ERR_ALLOC_FAILED on allocation failure
 */
EditorResult editor_paste();

/**
 * @brief Appends a line to the buffer during file loading
 *
 * Must be followed by editor_load_finalize() after all lines are loaded
 * @param text Pointer to the line text (need not be null-terminated)
 * @param len Number of characters to copy
 * @return EDITOR_OK on success; EDITOR_ERR_ALLOC_FAILED on allocation failure
 */
EditorResult editor_load_line(const char *text, int len);

/**
 * @brief Finalizes a load sequence started with editor_load_line()
 *
 * Resets the cursor and scroll to the top of the file
 */
void editor_load_finalize();

/**
 * @brief Sets the remote peer's cursor position
 * @param row Row index (0-based)
 * @param col Column index (0-based)
 */
void editor_set_remote_cursor(int row, int col);

/**
 * @brief Inserts a character at the remote cursor position.
 * 
 * Manages the memory reallocation if the line needs to grow, and pushes the text right.
 * If a newline character is inserted, it splits the current line dynamically.
 * The implementation assumes that the cursor is always within the bounds of the document and it is very similar to editor_insert_char, without updating the local cursor position.
 * 
 * @param c The character to insert.
 * @return EDITOR_OK on success, or EDITOR_ERR_ALLOC_FAILED on memory error.
 */
EditorResult editor_remote_insert_char(char c);

/**
 * @brief Deletes the character immediately before the remote cursor position.
 * 
 * If the remote cursor is at the beginning of a line, it merges the current line
 * with the previous line, performing a `memmove` to collapse the rows array.
 * Works similarly to editor_delete_char but without updating the local cursor position.
 * 
 * @return EDITOR_OK on success, or EDITOR_ERR_ALLOC_FAILED on memory error.
 */
EditorResult editor_remote_delete_char();

/**
 * @brief Prepares the text buffer for a block replacement.
 * 
 * This is the core of the remote block processing algorithm. It handles the structural
 * memory shifts (`memmove` on the lines array) required when multiple lines are 
 * deleted or inserted at once. For example, during a Paste or Cut operation, it 
 * dynamically expands or shrinks the total row count of the buffer.
 * 
 * @param start_row The index of the first row to be affected.
 * @param deleted_count Number of lines to remove from the buffer.
 * @param inserted_count Number of empty lines to allocate and make room for.
 * @return EDITOR_OK on success, or EDITOR_ERR_ALLOC_FAILED on memory error.
 */
EditorResult editor_remote_replace_block(int start_row, int deleted_count, int inserted_count);

/**
 * @brief Overwrites a specific line with new text received from the network.
 * 
 * Used immediately after `editor_remote_replace_block` to fill the newly created
 * empty lines with the actual payload text. Reallocates the internal buffer if the 
 * new text is larger than the previous capacity.
 * 
 * @param row The index of the line to update.
 * @param text The new string to be copied into the line.
 * @param len The length of the new string.
 * @return EDITOR_OK on success, or EDITOR_ERR_ALLOC_FAILED on memory error.
 */
EditorResult editor_remote_update_line(int row, const char *text, int len);
int editor_get_remote_cursor_row();

/**
 * @brief Returns the remote peer's cursor column
 * @return Column index (0-based). -1 if no position has been received yet
 */
int editor_get_remote_cursor_col();
