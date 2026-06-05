#include "controller/filetree_commands.h"
#include "controller/commands.h"
#include "model/filetree.h"
#include "model/command_bar.h"
#include "model/time/session_time.h"
#include "view/scene.h"
#include "model/render_state.h"
#include <string.h>

static void show_filetree_error() {
  const char *err = filetree_get_error();
  if (err) command_bar_set_status(err);
}

static void open_filetree_file() {
  FiletreeResult r = filetree_enter_selected();

  if (r == FILETREE_FILE) {
    // reset session timer when a new file is opened
    session_time_reset();

    char path[PATH_MAX];
    filetree_get_selected_path(path, sizeof(path));
    commands_open_file(path);
    // return focus to editor after opening
    filetree_set_focused(false);

  } else if (r == FILETREE_ERR) {
    show_filetree_error();
  }

  set_render(RENDER_FULL);
}

static void dispatch_filetree_mode(KeyEvent ev) {
  if (ev.escape) {
    filetree_set_focused(false);
    set_render(RENDER_FULL);
    return;
  }
  if (ev.dir == DIR_UP) {
    filetree_move_up(scene_get_vis_rows());
    set_render(RENDER_FULL);
    return;
  }
  if (ev.dir == DIR_DOWN) {
    filetree_move_down(scene_get_vis_rows());
    set_render(RENDER_FULL);
    return;
  }
  if (ev.backspace || ev.dir == DIR_LEFT) {
    if (filetree_go_parent() == FILETREE_ERR) show_filetree_error();
    set_render(RENDER_FULL);
    return;
  }
  if (ev.enter) {
    open_filetree_file();
  }
}

bool filetree_commands_try(KeyEvent ev) {
  // ctrl+b toggles filetree visibility and focus together
  if (ev.ctrl && ev.c == 'b') {
    bool now_visible = !filetree_is_visible();
    filetree_set_visible(now_visible);
    filetree_set_focused(now_visible);
    set_render(RENDER_FULL);
    return true;
  }

  // consume all events while filetree is focused
  if (filetree_is_focused()) {
    dispatch_filetree_mode(ev);
    return true;
  }

  return false;
}

bool filetree_commands_mouse(MouseEvent me) {

  int idx;
  // reset focus state; re-set below if click lands inside the panel
  filetree_set_focused(false);

  if (me.scroll != 0 && scene_px_to_filetree(me.click_x, me.click_y, &idx)) {
    filetree_scroll_by(me.scroll, scene_get_vis_rows());
    filetree_set_focused(true);
    set_render(RENDER_FULL);
    return true;
  }

  if (scene_px_to_filetree(me.click_x, me.click_y, &idx)) {

    filetree_set_focused(true);

    // clicked below all entries: just focus the panel
    if (idx == -1) {
      set_render(RENDER_FULL);
      return true;
    }

    // second click on same entry opens it (double-click behaviour)
    bool same = (idx == filetree_get_cursor());

    filetree_set_cursor(idx, scene_get_vis_rows());

    if (same) {
      open_filetree_file();
    }

    set_render(RENDER_FULL);
    return true;
  }

  set_render(RENDER_FULL);
  return false;
}
