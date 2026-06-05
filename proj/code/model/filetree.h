#pragma once
#include <stdbool.h>

/**
 * @file filetree.h
 * @brief File tree panel: directory listing, navigation, and entry selection
 */


/** @brief Maximum number of entries shown in the file tree */
#define FILETREE_MAX_ENTRIES 256

/** @brief Maximum length of an entry name including the null terminator */
#define FILETREE_NAME_LEN 64

typedef enum {
  FILETREE_OK, /* < Operation succeeded. */
  FILETREE_FILE, /* < A file is selected — caller should open it. */
  FILETREE_ERR /* < Operation failed — call filetree_get_error() for reason. */
} FiletreeResult;

/** @brief A single entry in the file tree listing */
typedef struct {
  char name[FILETREE_NAME_LEN];
  bool is_dir;
} FileEntry;

/**
 * @brief Initializes the file tree at the current working directory
 */
void filetree_init();

/**
 * @brief Reloads the file tree from the current working directory
 * @return FILETREE_OK on success; FILETREE_ERR on failure
 */
FiletreeResult filetree_refresh();

/**
 * @brief Returns a description of the last error
 * @return Null-terminated error string. Valid until the next filetree call
 */
const char *filetree_get_error();

/**
 * @brief Returns whether the file tree panel is currently visible
 */
bool filetree_is_visible();

/**
 * @brief Shows or hides the file tree panel
 */
void filetree_set_visible(bool v);

/**
 * @brief Returns whether the file tree has keyboard focus
 */
bool filetree_is_focused();

/**
 * @brief Gives or removes keyboard focus from the file tree.
 */
void filetree_set_focused(bool f);

/**
 * @brief Returns the current working directory path
 * @return Null-terminated absolute path string
 */
const char *filetree_get_cwd();

/**
 * @brief Returns the number of entries in the current listing
 * @return Entry count. At most FILETREE_MAX_ENTRIES
 */
int filetree_get_count();

/**
 * @brief Returns an entry from the current listing
 * @param i Entry index (0-based)
 * @return Pointer to the FileEntry; NULL if out of range
 */
const FileEntry *filetree_get_entry(int i);

/**
 * @brief Returns the index of the highlighted entry
 * @return Cursor index (0-based)
 */
int filetree_get_cursor();

/**
 * @brief Returns the scroll offset of the file tree
 * @return Index of the first visible entry
 */
int filetree_get_scroll();

/**
 * @brief Scrolls the file tree by a delta, clamped to the valid range
 * @param delta Number of entries to scroll (positive = down)
 * @param vis_rows Number of visible rows, used to clamp the scroll position
 */
void filetree_scroll_by(int delta, int vis_rows);

/**
 * @brief Moves the cursor one entry up and scrolls if needed
 * @param vis_rows Number of visible rows
 */
void filetree_move_up(int vis_rows);

/**
 * @brief Moves the cursor one entry down and scrolls if needed
 * @param vis_rows Number of visible rows
 */
void filetree_move_down(int vis_rows);

/**
 * @brief Moves the cursor to a specific entry and scrolls to keep it visible
 * @param idx Target entry index (0-based)
 * @param vis_rows Number of visible rows
 */
void filetree_set_cursor(int idx, int vis_rows);

/**
 * @brief Navigates to the parent directory and refreshes the listing
 * @return FILETREE_OK on success; FILETREE_ERR if already at the root or on failure
 */
FiletreeResult filetree_go_parent();

/**
 * @brief Enters the selected directory or signals that a file was selected
 * @return FILETREE_OK if a directory was entered; FILETREE_FILE if a file is selected;
 *         FILETREE_ERR on failure.
 */
FiletreeResult filetree_enter_selected();

/**
 * @brief Writes the full path of the selected entry into a buffer
 * @param buf Output buffer
 * @param size Size of the output buffer in bytes
 */
void filetree_get_selected_path(char *buf, int size);
