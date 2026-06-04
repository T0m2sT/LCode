#include "model/filetree.h"
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <limits.h>

#define CWD_MAX PATH_MAX

static char cwd[CWD_MAX];
static FileEntry entries[FILETREE_MAX_ENTRIES];
static int entry_count = 0;
static int cursor = 0;
static int scroll = 0;
static bool visible = false;
static bool focused = false;
static char error_msg[128] = {0};

static int compare_entries(const void *a, const void *b) {
  const FileEntry *ea = (const FileEntry *)a;
  const FileEntry *eb = (const FileEntry *)b;
  /* ".." always first */
  if (strcmp(ea->name, "..") == 0) return -1;
  if (strcmp(eb->name, "..") == 0) return 1;

  /* dirs before files */
  if (ea->is_dir != eb->is_dir) return ea->is_dir ? -1 : 1;

  return strcmp(ea->name, eb->name);
}

const char *filetree_get_error() { return error_msg[0] ? error_msg : NULL; }

FiletreeResult filetree_refresh() {
  entry_count = 0;
  DIR *dp = opendir(cwd);
  if (!dp) {
    //Writes message to the error_msg buffer
    snprintf(error_msg, sizeof(error_msg), "Cannot open '%s': %s", cwd, strerror(errno));
    return FILETREE_ERR;
  }

  error_msg[0] = '\0';
  struct dirent *ep;
  while ((ep = readdir(dp)) != NULL) {
    //Assume no more than 256 files in dir, keeping it simple. Worth noting that can be improved
    if (entry_count >= FILETREE_MAX_ENTRIES) break;

    if (strcmp(ep->d_name, ".") == 0) continue;
    strncpy(entries[entry_count].name, ep->d_name, FILETREE_NAME_LEN - 1);
    entries[entry_count].name[FILETREE_NAME_LEN - 1] = '\0';

    char full[CWD_MAX + FILETREE_NAME_LEN + 1];
    snprintf(full, sizeof(full), "%s/%s", cwd, ep->d_name);

    struct stat st;
    entries[entry_count].is_dir = (stat(full, &st) == 0 && S_ISDIR(st.st_mode));
    entry_count++;
  }
  closedir(dp);
  qsort(entries, entry_count, sizeof(FileEntry), compare_entries);
  if (cursor >= entry_count) cursor = entry_count > 0 ? entry_count - 1 : 0;
  return FILETREE_OK;
}

void filetree_init() {
  if (!getcwd(cwd, sizeof(cwd))) {
    snprintf(error_msg, sizeof(error_msg), "Cannot get working directory: %s", strerror(errno));
    strncpy(cwd, "/", sizeof(cwd));
  }
  filetree_refresh();
}

bool filetree_is_visible() { return visible;}
void filetree_set_visible(bool v) { visible = v;}
bool filetree_is_focused() { return focused;}
void filetree_set_focused(bool f) { focused = f;}

const char *filetree_get_cwd() { return cwd;}
int filetree_get_count() { return entry_count;}
int filetree_get_cursor() { return cursor;}
int filetree_get_scroll() { return scroll;}

const FileEntry *filetree_get_entry(int i) {
  if (i < 0 || i >= entry_count) return NULL;
  return &entries[i];
}

void filetree_move_up() {
  if (cursor > 0) cursor--;
  if (cursor < scroll) scroll = cursor;
}

void filetree_move_down(int vis_rows) {
  if (cursor < entry_count - 1) cursor++;
  if (cursor >= scroll + vis_rows) scroll = cursor - vis_rows + 1;
}

void filetree_set_cursor(int idx, int vis_rows) {
  if (entry_count == 0) {
    cursor = 0;
    scroll = 0;
    return;
  }

  if (idx < 0)
    idx = 0;

  if (idx >= entry_count)
    idx = entry_count - 1;

  cursor = idx;

  if (cursor < scroll)
    scroll = cursor;

  if (cursor >= scroll + vis_rows)
    scroll = cursor - vis_rows + 1;
}

FiletreeResult filetree_go_parent() {
  if (strcmp(cwd, "/") == 0) return FILETREE_OK;

  char *last = strrchr(cwd, '/');
  if (last && last != cwd){
    *last = '\0';
  }
  else {
    strncpy(cwd, "/", sizeof(cwd));
  }
  cursor = 0;
  scroll = 0;
  return filetree_refresh();
}

FiletreeResult filetree_enter_selected() {
  if (entry_count == 0) return FILETREE_ERR;

  FileEntry *e = &entries[cursor];
  if (!e->is_dir) return FILETREE_FILE;

  if (strcmp(e->name, "..") == 0) return filetree_go_parent();

  // Guard against path overflow before modifying cwd
  int new_len = (int)strlen(cwd) + 1 + (int)strlen(e->name);
  if (new_len >= CWD_MAX) {
    snprintf(error_msg, sizeof(error_msg), "Path too long");
    return FILETREE_ERR;
  }

  if (strlen(cwd) == 1 && cwd[0] == '/')
    snprintf(cwd, sizeof(cwd), "/%s", e->name);
  else {
    strncat(cwd, "/", sizeof(cwd) - strlen(cwd) - 1);
    strncat(cwd, e->name, sizeof(cwd) - strlen(cwd) - 1);
  }
  cursor = 0;
  scroll = 0;
  return filetree_refresh();
}

void filetree_get_selected_path(char *buf, int size) {
  if (entry_count == 0){
    buf[0] = '\0'; return;
  }
  snprintf(buf, size, "%s/%s", cwd, entries[cursor].name);
}

void filetree_scroll_by(int delta, int vis_rows) {
  scroll += delta;

  if (scroll < 0) scroll = 0;

  int max_scroll = entry_count - vis_rows;
  if (max_scroll < 0) max_scroll = 0;

  if (scroll > max_scroll)
    scroll = max_scroll;
}
