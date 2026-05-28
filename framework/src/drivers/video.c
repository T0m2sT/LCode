#include "fw/drivers/video.h"
#include "fw/hw/vbe.h"
#include "fw/common/utils.h"

static void *video_mem;
static unsigned h_res;
static unsigned v_res;
static unsigned bytes_per_pixel;
static unsigned frame_size;

int set_graphics_mode(uint16_t mode) {
  reg86_t r86;
  memset(&r86, 0, sizeof(r86));

  r86.intno = BIOS_VIDEOCARD_SERV;
  r86.ax = VBE_MODE_SET;
  r86.bx = mode | VBE_LINEAR_FB;

  if (sys_int86(&r86) != OK)
    return fail(ERR_VIDEO, "set_graphics_mode: sys_int86 failed");

  return 0;
}

// Function wrapper only for consistency reasons
int set_text_mode() {
  if (vg_exit() != OK)
    return fail(ERR_VIDEO, "set_text_mode: vg_exit failed");

  return 0;
}

int vg_map_vram(uint16_t mode) {
  vbe_mode_info_t info;
  memset(&info, 0, sizeof(info));

  if (vbe_get_mode_info(mode, &info) != OK)
    return fail(ERR_VIDEO, "vg_map_vram: vbe_get_mode_info failed");

  bytes_per_pixel = (info.BitsPerPixel + 7) / 8;
  h_res = info.XResolution;
  v_res = info.YResolution;
  frame_size = h_res * v_res * bytes_per_pixel;

  struct minix_mem_range physic_address;

  physic_address.mr_base = info.PhysBasePtr;
  physic_address.mr_limit = info.PhysBasePtr + frame_size;

  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &physic_address) != OK)
    return fail(ERR_VIDEO, "vg_map_vram: sys_privctl failed");

  video_mem = vm_map_phys(SELF,
                          (void *) physic_address.mr_base,
                          frame_size);

  if (video_mem == MAP_FAILED)
    return fail(ERR_VIDEO, "vg_map_vram: vm_map_phys failed");

  return 0;
}

void* vg_get_video_mem() { return video_mem; }
unsigned vg_get_h_res() { return h_res; }
unsigned vg_get_v_res(){ return v_res; }
unsigned vg_get_bytes_per_pixel() { return bytes_per_pixel; }
unsigned vg_get_frame_size() { return frame_size; }

int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color) {

  // Out of bounds pixels are ignored intentionally
  if (x >= h_res || y >= v_res)
    return 0;

  unsigned index = (h_res * y + x) * bytes_per_pixel;

  memcpy((uint8_t *) video_mem + index,
         &color,
         bytes_per_pixel);

  return 0;
}

int (vg_draw_hline)(uint16_t x,
                    uint16_t y,
                    uint16_t len,
                    uint32_t color) {

  for (unsigned i = 0; i < len; i++) {
    if (vg_draw_pixel(x + i, y, color) != OK)
      return fail(ERR_VIDEO, "vg_draw_hline: vg_draw_pixel failed");
  }

  return 0;
}

int (vg_draw_rectangle)(uint16_t x,
                        uint16_t y,
                        uint16_t width,
                        uint16_t height,
                        uint32_t color) {

  for (unsigned i = 0; i < height; i++) {
    if (vg_draw_hline(x, y + i, width, color) != OK)
      return fail(ERR_VIDEO, "vg_draw_rectangle: vg_draw_hline failed");
  }

  return 0;
}

int (print_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
  xpm_image_t img;
  enum xpm_image_type type;
  switch (bytes_per_pixel) {
    case 1:  type = XPM_INDEXED; break;  // indexed / palette mode
    case 3:  type = XPM_8_8_8;   break;  // 24-bit direct color
    case 4:  type = XPM_8_8_8_8; break;  // 32-bit direct color
    default:
      return fail(ERR_VIDEO, "print_xpm: unsupported bytes_per_pixel");
  }

  uint8_t *raw = xpm_load(xpm, type, &img);

  if (raw == NULL)
    return fail(ERR_VIDEO, "print_xpm: xpm_load failed");


  uint32_t *colors = (uint32_t *) raw;

  for (int h = 0; h < img.height; h++) {
    for (int w = 0; w < img.width; w++) {
      int idx = w + h * img.width;
      uint32_t color = (type == XPM_INDEXED) ? raw[idx] : colors[idx];

      if (vg_draw_pixel(x + w, y + h, color) != OK)
        return fail(ERR_VIDEO, "print_xpm: vg_draw_pixel failed");
    }
  }

  return 0;
}
