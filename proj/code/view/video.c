#include "video.h"
#include "fw/drivers/video.h"
#include "fw/hw/vbe.h"
#include "fw/common/utils.h"

#define VIDEO_MODE VBE_864p_DC

int video_init() {
  if (vg_map_vram(VIDEO_MODE) != OK)
    return fail(ERR_VIDEO, "video_init: vg_map_vram failed");

  if (set_graphics_mode(VIDEO_MODE) != OK)
    return fail(ERR_VIDEO, "video_init: set_graphics_mode failed");

  return 0;
}

void video_cleanup() {
  set_text_mode();
}
