#include "fw/drivers/keyboard.h"

static bool error=false;
static int hook_id;
static uint8_t scancode;

static uint8_t size=0;
static uint8_t bytes[2];

void set_scancode(uint8_t code){
  scancode=code;
}
uint8_t get_scancode(){
  return scancode;
}

bool get_error_keyboard(){
  return error;
}

int build_scancode(struct packet_scancode *ps){
  bytes[size]=scancode;
  if (scancode==0xE0){
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
  if (util_sys_inb(KBC_STATUS_REG,&status)!=0){
    error=true;
    return;
  }

  if (status&KBC_ST_OBF){//se o buffer estiver cheio
    uint8_t data;
    if (util_sys_inb(KBC_DATA_REG,&data)!=0){//mesmo que possa haver erro, ler sempre valor
      error=true;
      return;
    }

    if (status&(KBC_PAR_ERR|KBC_TOUT_ERR)){//detetar o erro, bits 6 e 7
      error=true;
      return;
    }
    
    error=false;
    set_scancode(data);
  }
  else{
    error=true;
  }
  
}

int (keyboard_subscribe_int)(uint8_t *bit_no){
  hook_id = *bit_no;
  if (sys_irqsetpolicy(KEYBOARD_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id)!=0) return 1;
  return 0;
}

int (keyboard_unsubscribe_int)() {
  if (sys_irqrmpolicy(&hook_id)!=0) return 1;
  return 0;
}
