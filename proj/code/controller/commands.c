#include "commands.h"
#include "filetree_commands.h"
#include "model/editor.h"
#include "model/command_bar.h"
#include "view/scene.h"
#include "render_flag.h"
#include <stdio.h>
#include <string.h>

static bool quit_flag = false;

/* Use instead of set_render when the operation might have scrolled the
 * viewport — upgrades to RENDER_FULL if the scroll offset changed. */
static void set_render_ex(int mode) {
  bool needs_full = editor_consume_scroll_dirty() || editor_consume_sel_dirty() || editor_sel_is_active();
  set_render(needs_full ? RENDER_FULL : mode);
}

bool get_quit() { return quit_flag; }

static void execute_save(const char *name) {
  if (!name || name[0] == '\0') return;
  FILE *f = fopen(name, "w");
  if (!f) return;
  for (int r = 0; r < editor_get_row_count(); r++)
    fprintf(f, "%s\n", editor_get_line(r));
  fclose(f);
  command_bar_set_filename(name);
}

static void execute_open(const char *name) {
  if (!name || name[0] == '\0') return;
  FILE *f = fopen(name, "r");
  if (!f) return;
  char line[MAX_COLS];
  editor_init();
  bool first = true;
  while (fgets(line, MAX_COLS, f)) {
    int len = strlen(line);
    if (len > 0 && line[len - 1] == '\n') line[--len] = '\0';
    if (!first) editor_insert_char('\n');
    first = false;
    for (int i = 0; line[i]; i++) editor_insert_char(line[i]);
  }
  fclose(f);
  command_bar_set_filename(name);
}

static void cmd_save(const char *args) {
  const char *name = (args && args[0]) ? args : command_bar_get_filename();
  execute_save(name);
}

static void cmd_open(const char *args) {
  execute_open(args);
}

static void cmd_quit(const char *args) {
  (void)args;
  quit_flag = true;
}

typedef void (*CommandHandler)(const char *args);
typedef struct { const char *name; CommandHandler handler; } CommandEntry;

static const CommandEntry command_table[] = {
  { "save", cmd_save },
  { "w",    cmd_save },
  { "open", cmd_open },
  { "q",    cmd_quit },
  { NULL,   NULL }
};

static void execute_command(const char *raw) {
  char name[CMD_BUF_SIZE];
  const char *args = "";
  int i = 0;
  while (raw[i] && raw[i] != ' ') i++;
  strncpy(name, raw, i);
  name[i] = '\0';
  if (raw[i] == ' ') args = raw + i + 1;
  for (int j = 0; command_table[j].name; j++) {
    if (strcmp(command_table[j].name, name) == 0) {
      command_table[j].handler(args);
      return;
    }
  }
}

static void dispatch_command_mode(KeyEvent ev) {
  if (ev.escape) {
    command_bar_cancel();
    set_render(RENDER_FULL);
    return;
  }
  if (ev.enter) {
    const char *input = command_bar_commit();
    execute_command(input);
    set_render(RENDER_FULL);
    return;
  }
  if (ev.backspace && !ev.ctrl) {
    command_bar_delete();
    set_render(RENDER_STATUS);
    return;
  }
  if (ev.c) {
    command_bar_insert(ev.c);
    set_render(RENDER_STATUS);
  }
}

void commands_open_file(const char *path) {
  execute_open(path);
}

void commands_dispatch(KeyEvent ev) {
  if (filetree_commands_try(ev)) return;

  if (command_bar_get_mode() == MODE_COMMAND) {
    dispatch_command_mode(ev);
    return;
  }

  if (ev.escape || (ev.ctrl && ev.c == 'q')) {
    quit_flag = true;
    return;
  }

  if (ev.backspace) {
    if (editor_sel_is_active()) {
      editor_delete_selection();
      set_render_ex(RENDER_FULL);
    } else if (ev.ctrl) {
      if (editor_get_cursor_col() == 0) {
        editor_delete_char();
        set_render_ex(RENDER_FULL);
      } else {
        editor_delete_word();
        set_render_ex(RENDER_LINE);
      }
    } else {
      bool mid_line = (editor_get_cursor_col() > 0);
      editor_delete_char();
      set_render_ex(mid_line ? RENDER_LINE : RENDER_FULL);
    }
    return;
  }

  if (ev.dir != DIR_NONE) {
    if (ev.shift) {
      if (!editor_sel_is_active()) editor_sel_set_anchor();
    } else {
      editor_sel_clear();
    }
    switch (ev.dir) {
      case DIR_LEFT: ev.ctrl ? editor_move_word_left() : editor_move_left(); break;
      case DIR_RIGHT: ev.ctrl ? editor_move_word_right() : editor_move_right(); break;
      case DIR_UP: editor_move_up(); break;
      case DIR_DOWN: editor_move_down(); break;
      case DIR_HOME: editor_move_home(); break;
      case DIR_END: editor_move_end(); break;
      default: break;
    }
    set_render_ex(RENDER_CHAR);
    return;
  }

  if (ev.ctrl && ev.c == 'c') {
    editor_copy_selection();
    return;
  }
  if (ev.ctrl && ev.c == 'x') {
    editor_copy_selection();
    editor_delete_selection();
    set_render_ex(RENDER_FULL);
    return;
  }
  if (ev.ctrl && ev.c == 'v') {
    if (editor_sel_is_active()) editor_delete_selection();
    EditorResult result = editor_paste();
    switch (result) {
      case EDITOR_OK: set_render_ex(RENDER_FULL); break;
      case EDITOR_ERR_NO_CLIPBOARD: command_bar_set_status("Nothing to paste"); set_render_ex(RENDER_STATUS); break;
      case EDITOR_ERR_DOCUMENT_FULL: command_bar_set_status("Document full - paste aborted"); set_render_ex(RENDER_STATUS); break;
    }
    return;
  }
  if (ev.ctrl && ev.c == 's') {
    if (strcmp(command_bar_get_filename(), "untitled") == 0)
      command_bar_start_prefill("save ");
    else
      execute_save(command_bar_get_filename());
    set_render(RENDER_FULL);
    return;
  }
  if (ev.ctrl && ev.c == 'o') {
    command_bar_start_prefill("open ");
    set_render(RENDER_FULL);
    return;
  }
  if (ev.ctrl) return;

  if (ev.c == ':') {
    command_bar_start();
    set_render(RENDER_FULL);
    return;
  }

  if (ev.enter) {
    if (editor_sel_is_active()) editor_delete_selection();
    editor_insert_char('\n');
    set_render(RENDER_FULL);
    return;
  }

  if (ev.c) {
    if (editor_sel_is_active()) editor_delete_selection();
    editor_insert_char(ev.c);
    set_render_ex(RENDER_LINE);
  }
}

void commands_dispatch_mouse(MouseEvent me) {
  if (!me.left_clicked) return;
  if (scene_click_scrollbar(me.click_x, me.click_y)) return;
  int row, col;
  if (scene_px_to_text(me.click_x, me.click_y, &row, &col)) {
    editor_sel_clear();
    editor_set_cursor(row, col);
    set_render_ex(RENDER_CHAR);
  }
}
