#include "controller/input/keyboard.h"
#include "controller/input/events.h"

#define SCANCODE_ESC 0x01
#define SCANCODE_BACKSPACE 0x0E
#define SCANCODE_ENTER 0x1C
#define SCANCODE_LCTRL 0x1D
#define SCANCODE_RCTRL 0x1D
#define SCANCODE_LSHIFT 0x2A
#define SCANCODE_RSHIFT 0x36
#define SCANCODE_RALT   0x38
#define SCANCODE_UP     0x48
#define SCANCODE_DOWN   0x50
#define SCANCODE_LEFT   0x4B
#define SCANCODE_RIGHT  0x4D
#define SCANCODE_HOME   0x47
#define SCANCODE_END    0x4F

static const char sc_lower[58] = {
  0,    0,   '1', '2', '3', '4', '5', '6',  /* 0x00-0x07 */
  '7', '8', '9', '0','\'',  0,    0,  '\t', /* 0x08-0x0F */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  /* 0x10-0x17 */
  'o', 'p', '+',  0,    0,    0,  'a', 's', /* 0x18-0x1F */
  'd', 'f', 'g', 'h', 'j', 'k', 'l',  0,   /* 0x20-0x27 */
   0,  '\\', 0,  '~', 'z', 'x', 'c', 'v',  /* 0x28-0x2F */
  'b', 'n', 'm', ',', '.', '-',  0,    0,   /* 0x30-0x37 */
  0,   ' '                                  /* 0x38-0x39 */
};

static const char sc_upper[58] = {
  0,    0,   '!', '"', '#', '$', '%', '&',  /* 0x00-0x07 */
  '/', '(', ')', '=', '?',  0,    0,  '\t', /* 0x08-0x0F */
  'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I',  /* 0x10-0x17 */
  'O', 'P', '*',  0,    0,    0,  'A', 'S', /* 0x18-0x1F */
  'D', 'F', 'G', 'H', 'J', 'K', 'L',  0,   /* 0x20-0x27 */
   0,  '|',  0,  '^', 'Z', 'X', 'C', 'V',  /* 0x28-0x2F */
  'B', 'N', 'M', ';', ':', '_',  0,    0,   /* 0x30-0x37 */
  0,   ' '                                  /* 0x38-0x39 */
};

static const char sc_altgr[58] = {
  0,    0,   '1', '@', '3', '4', '5', '6',  /* 0x00-0x07 */
  '{', '[', ']', '}','\'',  0,    0,  '\t', /* 0x08-0x0F */
  'q', 'w', 'e', 'r', 't', 'y', 'u', 'i',  /* 0x10-0x17 */
  'o', 'p', '+',  0,    0,    0,  'a', 's', /* 0x18-0x1F */
  'd', 'f', 'g', 'h', 'j', 'k', 'l',  0,   /* 0x20-0x27 */
   0,  '\\', 0,  '~', 'z', 'x', 'c', 'v',  /* 0x28-0x2F */
  'b', 'n', 'm', ',', '.', '-',  0,    0,   /* 0x30-0x37 */
  0,   ' '                                  /* 0x38-0x39 */
};

static bool ctrl_pressed = false;
static bool shift_pressed = false;
static bool altgr_pressed = false;

void keyboard_process(packet_scancode ps) {
  if (ps.size == 2) {
    uint8_t code2 = ps.bytes[1] & 0x7F;
    if (code2 == SCANCODE_RCTRL) { ctrl_pressed = ps.make; return; }
    if (code2 == SCANCODE_RALT) { altgr_pressed = ps.make; return; }
    if (!ps.make) return;
    KeyEvent ev = {
      .c = 0, .ctrl = ctrl_pressed, .shift = shift_pressed, .altgr = altgr_pressed,
      .backspace = false, .enter = false, .escape = false, .dir = DIR_NONE
    };
    if (code2 == SCANCODE_LEFT) ev.dir = DIR_LEFT;
    else if (code2 == SCANCODE_RIGHT) ev.dir = DIR_RIGHT;
    else if (code2 == SCANCODE_UP) ev.dir = DIR_UP;
    else if (code2 == SCANCODE_DOWN) ev.dir = DIR_DOWN;
    else if (code2 == SCANCODE_HOME) ev.dir = DIR_HOME;
    else if (code2 == SCANCODE_END) ev.dir = DIR_END;
    if (ev.dir != DIR_NONE) {
      InputEvent iev = {.type = INPUT_EVENT_KEY, .data.key = ev};
      input_event_push(iev);
    }
    return;
  }

  uint8_t code = ps.bytes[0] & 0x7F;

  if (code == SCANCODE_LCTRL) { ctrl_pressed = ps.make; return; }
  if (code == SCANCODE_LSHIFT || code == SCANCODE_RSHIFT) { shift_pressed = ps.make; return; }

  if (!ps.make) return;

  KeyEvent ev = {
    .c = 0, .ctrl = ctrl_pressed, .shift = shift_pressed,
    .backspace = false, .enter = false, .escape = false, .dir = DIR_NONE
  };

  if (code == SCANCODE_ESC) { ev.escape = true; }
  else if (code == SCANCODE_BACKSPACE) { ev.backspace = true; }
  else if (code == SCANCODE_ENTER) { ev.enter = true; }

  if (!ev.escape && !ev.backspace && !ev.enter && code < 58) {
    char c = 0;
    if (altgr_pressed)
      c = sc_altgr[code];
    else if (shift_pressed)
      c = sc_upper[code];
    else
      c = sc_lower[code];

    if (c) ev.c = c;
  }

  if (ev.escape || ev.backspace || ev.enter || ev.dir != DIR_NONE || ev.c) {
    InputEvent iev = {.type = INPUT_EVENT_KEY, .data.key = ev};
    input_event_push(iev);
  }
}
