#include "render_flag.h"

static DirtyFlags dirty = DIRTY_ALL;

void set_dirty(DirtyFlags flags) { dirty |= flags; }
DirtyFlags get_dirty() { return dirty; }
void clear_dirty() { dirty = DIRTY_NONE; }
