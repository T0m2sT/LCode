#include <stdint.h>
#include "fw/hw/i8042.h"

int (kbc_wait_input_empty)() {
  uint8_t status;
  int contador=10;

  while (contador--) {
    util_sys_inb(KBC_STATUS_REG, &status);
    if ((status & KBC_ST_IBF) == 0) return 0;
    tickdelay(micros_to_ticks(20000));
  }
  return 1;
}

int (kbc_wait_output_full)(uint8_t* data){
  uint8_t status;
  int contador = 10;

  while (contador--) {
    util_sys_inb(KBC_STATUS_REG, &status);
    if ((status & KBC_TOUT_ERR ) || (status & KBC_PAR_ERR)){
      continue;
    }
    if (status & KBC_ST_OBF) {
      util_sys_inb(KBC_DATA_REG, data);
      return 0;
    }
    tickdelay(micros_to_ticks(20000));
    }
  return 1;

}

int (kbc_enable_interrupts)(){
  uint8_t cmd;
    
  if (kbc_wait_input_empty() != 0) return 1;
  if (sys_outb(KBC_STATUS_REG, RD_CMD)!=0)return 1;
        
  if (kbc_wait_output_full(&cmd) != 0) return 1;
  
  cmd = cmd | BIT(0);
        
  if (kbc_wait_input_empty() != 0) return 1;
  if(sys_outb(KBC_STATUS_REG, WR_CMD)!=0)return 1;
    
  if (kbc_wait_input_empty() != 0) return 1;
  if(sys_outb(KBC_DATA_REG, cmd)!=0)return 1;
    
  return 0;
}
