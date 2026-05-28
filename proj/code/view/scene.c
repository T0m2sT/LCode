#include "scene.h"
#include "video.h"
#include "render_flag.h"
#include "fw/drivers/timer.h"
#include "fw/common/utils.h"

#define COLOR_BG 0x1E1E1E
#define COLOR_CURSOR 0xFFFFFF

#define CURSOR_X 10
#define CURSOR_Y 10
#define CURSOR_W 10
#define CURSOR_H 20
#define BLINK_TICKS 30

static SceneID current_scene = SCENE_EDITOR;

static void render_full() {
  bb_clear(COLOR_BG);
}

static void render_cursor() {
  bool visible = (get_int_counter() / BLINK_TICKS) % 2 == 0;
  bb_draw_rect(CURSOR_X, CURSOR_Y, CURSOR_W, CURSOR_H,
               visible ? COLOR_CURSOR : COLOR_BG);
}

static void render_lines() {
  // stub — implemented when model/editor.c exists
}

int scene_init(SceneID id) {
  current_scene = id;
  set_dirty(DIRTY_ALL);
  return 0;
}

void scene_cleanup() {}

void view_render() {
  DirtyFlags flags = get_dirty();
  clear_dirty();
  bool drew = false;

  switch (current_scene) {
    case SCENE_EDITOR:
      if (flags & DIRTY_ALL) { render_full(); drew = true; }
      if (flags & DIRTY_LINE) { render_lines(); drew = true; }
      if (flags & DIRTY_CURSOR) { render_cursor(); drew = true; }
      break;
  }

  if (drew) vg_flip_buffer();
}
