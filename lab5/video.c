#include "video.h"
#include "VBE.h"

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

  if (sys_int86(&r86) != OK) return 1;

  return 0;
}

// Function wrapper only for consistency reasons
int set_text_mode() {
  return vg_exit();
}


int vg_map_vram(uint16_t mode) {
  vbe_mode_info_t info;
  memset(&info, 0, sizeof(info));

  if (vbe_get_mode_info(mode, &info) != OK) return 1;

  bytes_per_pixel = (info.BitsPerPixel + 7)/8;
  h_res = info.XResolution;
  v_res = info.YResolution;
  frame_size = h_res * v_res * bytes_per_pixel;

  struct minix_mem_range physic_address;
  physic_address.mr_base = info.PhysBasePtr;
  physic_address.mr_limit = info.PhysBasePtr + frame_size;

  if (sys_privctl(SELF, SYS_PRIV_ADD_MEM, &physic_address) != OK)
    return 1;

  video_mem = vm_map_phys(SELF, (void*) physic_address.mr_base, frame_size);
  if (video_mem == MAP_FAILED) return 1;

  return 0;
}

int (vg_draw_pixel)(uint16_t x, uint16_t y, uint32_t color) {
  // Even though coordinates are invalid
  // It handles it by dismmissing this pixel
  if(x >= h_res || y >= v_res) return 0;

  unsigned index = (h_res * y + x) * bytes_per_pixel;

  memcpy((uint8_t *) video_mem + index, &color, bytes_per_pixel);

  return 0;
}

int (vg_draw_hline)(uint16_t x, uint16_t y, uint16_t len, uint32_t color) {
  for (unsigned i = 0; i < len; i++)
    if (vg_draw_pixel(x+i, y, color) != OK) return 1;

  return 0;
}

int (vg_draw_rectangle)(uint16_t x, uint16_t y, uint16_t width, uint16_t height, uint32_t color) {
  for(unsigned i = 0; i < height ; i++)
    if (vg_draw_hline(x, y+i, width, color) != OK) return 1;

  return 0;
}

int (print_xpm)(xpm_map_t xpm, uint16_t x, uint16_t y) {
  xpm_image_t img;

  uint8_t *colors = xpm_load(xpm, XPM_INDEXED, &img);
  if (colors == NULL) return 1;

  for (int h = 0 ; h < img.height ; h++) {
    for (int w = 0 ; w < img.width ; w++) {
      if (vg_draw_pixel(x + w, y + h, colors[w + h*img.width])) return 1;
    }
  }
  
  return 0;
}
