#pragma once

#include <lcom/lcf.h>

typedef enum {
  SCENE_EDITOR
} SceneID;

int scene_init(SceneID id);
void scene_cleanup();

void view_render();
