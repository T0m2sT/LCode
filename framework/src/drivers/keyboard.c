#include "fw/drivers/keyboard.h"
#include "fw/common/utils.h"

static bool error=false;
static int hook_id=1;
static uint8_t scancode;

#include <stdio.h>

static const char *scancode_to_name_table[128] = {
    [0x01] = "ESC",
    [0x02] = "1",
    [0x03] = "2",
    [0x04] = "3",
    [0x05] = "4",
    [0x06] = "5",
    [0x07] = "6",
    [0x08] = "7",
    [0x09] = "8",
    [0x0A] = "9",
    [0x0B] = "0",
    [0x0C] = "-",
    [0x0D] = "=",
    [0x0E] = "BACKSPACE",
    [0x0F] = "TAB",
    [0x10] = "q",
    [0x11] = "w",
    [0x12] = "e",
    [0x13] = "r",
    [0x14] = "t",
    [0x15] = "y",
    [0x16] = "u",
    [0x17] = "i",
    [0x18] = "o",
    [0x19] = "p",
    [0x1A] = "[",
    [0x1B] = "]",
    [0x1C] = "ENTER",
    [0x1D] = "CTRL",
    [0x1E] = "a",
    [0x1F] = "s",
    [0x20] = "d",
    [0x21] = "f",
    [0x22] = "g",
    [0x23] = "h",
    [0x24] = "j",
    [0x25] = "k",
    [0x26] = "l",
    [0x27] = "ç",
    [0x28] = "'",
    [0x29] = "`",
    [0x2A] = "LSHIFT",
    [0x2B] = "\\",
    [0x2C] = "z",
    [0x2D] = "x",
    [0x2E] = "c",
    [0x2F] = "v",
    [0x30] = "b",
    [0x31] = "n",
    [0x32] = "m",
    [0x33] = ",",
    [0x34] = ".",
    [0x35] = "/",
    [0x36] = "RSHIFT",
    [0x38] = "ALT",
    [0x39] = "SPACE",
    [0x3A] = "CAPS"
};

const char *scancode_to_char(uint8_t scancode, bool two_byte) {
  if (two_byte) {
    return NULL;
  }

  return scancode_to_name_table[scancode];
}

void print_scancode_value(packet_scancode ps) {
  uint8_t code = (ps.two_byte) ? ps.bytes[1] : ps.bytes[0];
  code &= ~BREAKCODE;

  const char *name = scancode_to_char(code, ps.two_byte);
  if (name != NULL) {
      printf(" - %s", name);
  } else {
    printf(" - Not Def");
  }
}

void set_scancode(uint8_t code){
  scancode=code;
}
uint8_t get_scancode(){
  return scancode;
}

bool get_error_keyboard(){
  return error;
}

int build_scancode(packet_scancode *ps){
  if (ps->size > 2)
    return fail(ERR_KEYBOARD, "build_scancode: invalid scancode size");
  
  uint8_t byte = get_scancode();

  if (byte == TWO_BYTE) {
    ps->two_byte = true;
    ps->bytes[0] = byte;
    return 0;
  }

  if (ps->two_byte) {
    ps->bytes[1] = byte;
    ps->size = 2;
    ps->two_byte = false;
  } else {
    ps->bytes[0] = byte;
    ps->size = 1;
  }
  
  ps->make = !(byte & BIT(7));
  return 0;
}


void (keyboard_ih)() {
  uint8_t status;
  uint8_t data;

  if (util_sys_inb(KBC_STATUS_REG, &status) != OK) {
    fail(ERR_KEYBOARD, "keyboard_ih: failed to read status register");
    return;
  }

  if (!(status & KBC_ST_OBF)) {
    fail(ERR_KEYBOARD, "keyboard_ih: output buffer not full");
    return;
  }

  if (status & (KBC_PAR_ERR | KBC_TOUT_ERR)) {
    fail(ERR_KEYBOARD, "keyboard_ih: parity or timeout error");
    return;
  }

  if (util_sys_inb(KBC_DATA_REG, &data) != OK) {
    fail(ERR_KEYBOARD, "keyboard_ih: failed to read data register");
    return;
  }

  set_scancode(data);
}

int (keyboard_subscribe_int)(uint8_t *bit_no){
  *bit_no = hook_id;

  if (sys_irqsetpolicy(KEYBOARD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) != OK) {
    return fail(ERR_KEYBOARD, "keyboard_subscribe_int: sys_irqsetpolicy failed");
  }

  return 0;
}

int (keyboard_unsubscribe_int)() {

  if (sys_irqrmpolicy(&hook_id) != OK) {
    return fail(ERR_KEYBOARD, "keyboard_unsubscribe_int: sys_irqrmpolicy failed");
  }

  return 0;
}

int (keyboard_print_scancode)(packet_scancode ps) {
    if (ps.size == 0) {
        return fail(ERR_KEYBOARD, "keyboard_print_scancode: invalid scancode size");
    }

    printf("%s code: ", ps.make ? "MAKE" : "BREAK");

    for (uint8_t i = 0; i < ps.size; i++) {
        printf("0x%02X", ps.bytes[i]);

        if (i < ps.size - 1) {
            printf(" ");
        }
    }

    print_scancode_value(ps);

    printf("\n");

    return 0;
}
