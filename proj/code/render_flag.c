#include "render_flag.h"

static int render_mode = RENDER_NONE;

void set_render(int mode) {
  if ((render_mode == RENDER_LINE && mode == RENDER_REMOTE_LINE) ||
      (render_mode == RENDER_REMOTE_LINE && mode == RENDER_LINE)) {
      render_mode = RENDER_FULL;
  }//if two lines is needed, then render full
  if (mode > render_mode) render_mode = mode;
}

int get_render() { return render_mode; }
void clear_render() { render_mode = RENDER_NONE; }
