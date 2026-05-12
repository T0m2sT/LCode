#include "fw/drivers/mouse.h"

static int hook_id;
static uint8_t data;
static bool error=false;
static uint8_t bytes[3];
static uint8_t count_bytes=0;

void (mouse_ih)(void) {
  uint8_t status;
  if (util_sys_inb(KBC_STATUS_REG,&status)!=0){
    error=true;
    return;
  }
  if ((status&KBC_MOUSE_DATA) && (status&KBC_ST_OBF)){
    if (util_sys_inb(KBC_DATA_REG,&data)!=0){
      error=true;
      return;
    }
    error=false;
    return;
  }
  error=true;
}

bool get_error(){
  return error;
}

int build_packet(struct packet *pp){
  if ((count_bytes==0) && (!(data&ALWAYS_1))) return 0;//não fez nenhum packet
  bytes[count_bytes]=data;
  count_bytes++;
  if (count_bytes==3){
    pp->bytes[0] = bytes[0];
    pp->bytes[1] = bytes[1];
    pp->bytes[2] = bytes[2];
              
    pp->rb = bytes[0]&RIGHT_BTN ? true : false;
    pp->mb = bytes[0]&MID_BTN ? true : false;
    pp->lb = bytes[0]&LEFT_BTN ? true : false;
    pp->x_ov = bytes[0]&X_OVERFLOW ? true : false;
    pp->y_ov = bytes[0]&Y_OVERFLOW ? true : false;

    pp->delta_x = bytes[0]&X_SIGN ? (0xFF00 | bytes[1]) : bytes[1];
    pp->delta_y = bytes[0]&Y_SIGN ? (0xFF00 | bytes[2]) : bytes[2];

    count_bytes=0;
              
    return 1;//fez um packet
  }
  return 0;//não fez nenhum packet
}


int mouse_write_command(uint8_t cmd){
  uint8_t response;
  int contador=10;

  while (contador--) {

    if (kbc_wait_input_empty() != 0) return 1;
    if (sys_outb(KBC_STATUS_REG, MOUSE_COMMAND) != OK) return 1;

    if (kbc_wait_input_empty() != 0) return 1;
    if (sys_outb(KBC_DATA_REG, cmd) != 0) return 1;

    if (kbc_wait_output_full(&response) != 0) return 1;

    if (response == SUCCESS) return 0;

  }

  return 1;
}


int mouse_disable_data_reporting(){
  if(mouse_write_command(DISABLE_DR)!=0) return 1;
  return 0;
}

int mouse_enable_data_reporting_1(){
  if(mouse_write_command(ENABLE_DR)!=0) return 1;
  return 0;
}

int (mouse_subscribe_int)(uint8_t *bit_no){
  hook_id = *bit_no;
  if (sys_irqsetpolicy(KBD_AUX_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id)!=0) return 1;
  return 0;
}

int (mouse_unsubscribe_int)() {
  if (sys_irqrmpolicy(&hook_id)!=0) return 1;
  return 0;
}
