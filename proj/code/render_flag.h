#pragma once

#include <lcom/lcf.h>

typedef uint8_t DirtyFlags;

#define DIRTY_NONE   0
#define DIRTY_CURSOR (1 << 0)
#define DIRTY_LINE (1 << 1)
#define DIRTY_ALL (1 << 2)

void set_dirty(DirtyFlags flags);
DirtyFlags get_dirty();
void clear_dirty();
