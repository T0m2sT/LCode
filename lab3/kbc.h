#ifndef KBC_H
#define KBC_H

#include <lcom/lcf.h>

// REGISTERS
#define KBC_STATUS_REG 0x64
#define KBC_OUT_BUF    0x60

// STATUS
#define OBF     BIT(0)
#define IBF     BIT(0)
#define PAR_ERR BIT(7)
#define TO_ERR  BIT(6)

// SCANCODE CONSTANTS
#define TWO_BYTE 0xE0
#define ESC_BREAK 0x81

// COMMAND BYTE
#define KBC_READ_CMD   0x20
#define KBC_WRITE_CMD  0x60

uint8_t get_scancode_byte();
void set_scancode_byte(uint8_t byte);

int kbd_subscribe_int(uint8_t *bit_no);
int kbd_unsubscribe_int();

void (kbc_ih)();


#endif
