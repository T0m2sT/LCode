#include "fw/drivers/keyboard.h"
#include "fw/common/utils.h"

static bool error=false;
static int hook_id=1;
static uint8_t scancode;

//static uint8_t size=0;
//static uint8_t bytes[2];

void set_scancode(uint8_t code){
  scancode=code;
}
uint8_t get_scancode(){
  return scancode;
}

bool did_error_occur(){
  return error;
}

/* REFACTOR LATER
int build_scancode(struct packet_scancode *ps){
  bytes[size]=scancode;
  if (scancode==TWO_BYTE){
    size++;
    return 1;
  }
  ps->make = scancode&BREAKCODE ? false : true;
  ps->size=size+1;
  ps->bytes[0]=bytes[0];
  ps->bytes[1]=bytes[1];
  size=0;
  return 0;
}
*/

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

int (keyboard_print_scancode)(bool make, uint8_t size, uint8_t *bytes) {
    if (bytes == NULL || size == 0) {
        return fail(ERR_KEYBOARD, "Invalid scancode bytes or size");
    }

    printf("%s code: ", make ? "MAKE" : "BREAK");

    for (uint8_t i = 0; i < size; i++) {
        printf("0x%02X", bytes[i]);

        if (i < size - 1) {
            printf(" ");
        }
    }

    printf("\n");

    return 0;
}
