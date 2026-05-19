#ifndef _LCOM_I8042_H_
#define _LCOM_I8042_H_

#include <lcom/lcf.h>

/* I/O port addresses */

#define KBC_STATUS_REG  0x64 
#define KBC_DATA_REG    0x60 

/* Mouse selection */

#define LEFT_BTN    BIT(0)
#define RIGHT_BTN   BIT(1)
#define MID_BTN     BIT(2)
#define ALWAYS_1    BIT(3)
#define X_SIGN      BIT(4)
#define Y_SIGN      BIT(5)
#define X_OVERFLOW  BIT(6)
#define Y_OVERFLOW  BIT(7)

/* Mouse Commands*/

#define MOUSE_COMMAND     0xD4
#define ENABLE_DR         0xF4
#define DISABLE_DR        0xF5
#define SUCCESS           0xFA
#define FAILURE           0xFE


/* KBC Status */

#define KBC_ST_OBF      BIT(0)
#define KBC_ST_IBF      BIT(1)
#define KBC_MOUSE_DATA  BIT(5)
#define KBC_TOUT_ERR    BIT(6)
#define KBC_PAR_ERR     BIT(7)

/* Keyboard Commands*/

#define RD_CMD 0x20
#define WR_CMD 0x60

/* Keyboard scancode*/

#define BREAKCODE BIT(7)

/* Scancode Constants*/
#define TWO_BYTE 0xE0

/* KBC functions*/

int (kbc_wait_input_empty)();
int (kbc_wait_output_full)(uint8_t* data);
int (kbc_enable_interrupts)();


#endif /* _LCOM_I8042_H_ */
