#pragma once
#include <stdbool.h>

#define FILETREE_MAX_ENTRIES 256
#define FILETREE_NAME_LEN 64

typedef enum {
  FILETREE_OK,    /* operation succeeded */
  FILETREE_FILE,  /* a file is selected — caller should open it */
  FILETREE_ERR    /* operation failed — call filetree_get_error() for reason */
} FiletreeResult;

typedef struct {
  char name[FILETREE_NAME_LEN];
  bool is_dir;
} FileEntry;

void filetree_init();
FiletreeResult filetree_refresh();
const char *filetree_get_error();

bool filetree_is_visible();
void filetree_set_visible(bool v);
bool filetree_is_focused();
void filetree_set_focused(bool f);

const char *filetree_get_cwd();
int filetree_get_count();
const FileEntry *filetree_get_entry(int i);
int filetree_get_cursor();
int filetree_get_scroll();

void filetree_move_up();
void filetree_move_down(int vis_rows);
FiletreeResult filetree_go_parent();
FiletreeResult filetree_enter_selected();
void filetree_get_selected_path(char *buf, int size);
