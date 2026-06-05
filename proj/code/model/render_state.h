#pragma once

#include <lcom/lcf.h>


#define RENDER_NONE         0
#define RENDER_MOUSE        1
#define RENDER_STATUS       2
#define RENDER_CHAR         3
#define RENDER_REMOTE_LINE  4
#define RENDER_LINE         5
#define RENDER_WORD         6
#define RENDER_FULL         7

void set_render(int mode);
int get_render();
void clear_render();
