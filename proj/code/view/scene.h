#pragma once

#include <lcom/lcf.h>

typedef enum {
  SCENE_EDITOR
} SceneID;

int scene_init(SceneID id);
void scene_cleanup();
int scene_get_vis_rows();

void view_render();

bool scene_px_to_text(int px, int py, int *out_row, int *out_col);
bool scene_click_scrollbar(int px, int py);

#include "view/syntax.h"

void scene_set_language(SyntaxLanguage lang);
