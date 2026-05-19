#include "fw/drivers/keyboard.h"

static bool error=false;
static int hook_id=1;
static uint8_t scancode;

static uint8_t size=0;
static uint8_t bytes[2];

void set_scancode(uint8_t code){
  scancode=code;
}
uint8_t get_scancode(){
  return scancode;
}

bool did_error_occur(){
  return error;
}

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


void (keyboard_ih)(){
  uint8_t status;
  uint8_t data;
    
  // Read status register
  if (util_sys_inb(KBC_STATUS_REG, &status) != OK) {
    error = true;
    return;
  }

  // Check if output buffer is full
  if (!(status & KBC_ST_OBF)){
    error = true;
    return;
  }

  // Check for errors
  if (status & (KBC_PAR_ERR | KBC_TOUT_ERR)){
    error = true;
    return;
  }

  // Read scancode byte
  if (util_sys_inb(KBC_DATA_REG, &data) != OK){
    error = true;
    return;
  }


  set_scancode_byte(data);
  
}

int (keyboard_subscribe_int)(uint8_t *bit_no){
  *bit_no = hook_id;

  if (sys_irqsetpolicy(KEYBOARD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) != OK) {
    return 1;
  }

  return 0;
}

int (keyboard_unsubscribe_int)() {

  if (sys_irqrmpolicy(&hook_id) != OK) {
    return 1;
  }

  return 0;
}
