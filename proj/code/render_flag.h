#pragma once

#include <lcom/lcf.h>


#define RENDER_NONE   0
#define RENDER_STATUS 1
#define RENDER_CHAR   2
#define RENDER_LINE   3
#define RENDER_WORD   4
#define RENDER_FULL   5

void set_render(int mode);
int get_render();
void clear_render();
