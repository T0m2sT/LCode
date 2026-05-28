#include "commands.h"
#include "model/editor.h"
#include "render_flag.h"

static bool quit_flag = false;

bool get_quit() { return quit_flag; }

void commands_dispatch(KeyEvent ev) {
  if (ev.escape || (ev.ctrl && ev.c == 'q')) {
    quit_flag = true;
    return;
  }

  if (ev.backspace) {
    if (ev.ctrl) {
      if (editor_get_cursor_col() == 0) {
        editor_delete_char();
        set_render(RENDER_CHAR);
      } else {
        editor_delete_word();
        set_render(RENDER_WORD);
      }
    } else {
      editor_delete_char();
      set_render(RENDER_CHAR);
    }
    return;
  }

  if (ev.dir != DIR_NONE) {
    switch (ev.dir) {
      case DIR_LEFT:  ev.ctrl ? editor_move_word_left()  : editor_move_left();  break;
      case DIR_RIGHT: ev.ctrl ? editor_move_word_right() : editor_move_right(); break;
      case DIR_UP:    editor_move_up();   break;
      case DIR_DOWN:  editor_move_down(); break;
      default: break;
    }
    set_render(RENDER_CHAR);
    return;
  }

  if (ev.ctrl) return;

  if (ev.enter) {
    editor_insert_char('\n');
    set_render(RENDER_FULL);
    return;
  }

  if (ev.c) {
    editor_insert_char(ev.c);
    set_render(RENDER_CHAR);
  }
}
