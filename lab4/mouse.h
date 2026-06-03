#ifndef MOUSE_H
#define MOUSE_H

#include <lcom/lcf.h>

/* ---- i8042 I/O ports (same controller as keyboard) ---- */
#define KBC_STATUS_REG  0x64
#define KBC_CMD_REG     0x64
#define KBC_OUT_BUF     0x60
#define KBC_IN_BUF      0x60

/* ---- KBC status bits ---- */
#define OBF     BIT(0)  /* Output Buffer Full: safe to read 0x60 */
#define IBF     BIT(1)  /* Input Buffer Full: do NOT write yet  */
#define AUX_OBF BIT(5)  /* Data in OBF came from the mouse      */
#define PAR_ERR BIT(7)
#define TO_ERR  BIT(6)

/* ---- KBC commands ---- */
#define KBC_WRITE_AUX 0xD4  /* Forward next byte to mouse */

/* ---- Mouse commands ---- */
#define MOUSE_ENABLE_DR  0xF4  /* Enable data reporting (stream mode) */
#define MOUSE_DISABLE_DR 0xF5  /* Disable data reporting              */
#define MOUSE_RESET      0xFF

/* ---- Mouse response bytes ---- */
#define MOUSE_ACK  0xFA
#define MOUSE_NACK 0xFE
#define MOUSE_ERR  0xFC

/* ---- Packet byte-1 bits ---- */
#define PKT_LB      BIT(0)  /* Left button   */
#define PKT_RB      BIT(1)  /* Right button  */
#define PKT_MB      BIT(2)  /* Middle button */
#define PKT_ALWAYS1 BIT(3)  /* Always 1 in byte 1 - used for sync */
#define PKT_X_SIGN  BIT(4)  /* X sign bit    */
#define PKT_Y_SIGN  BIT(5)  /* Y sign bit    */
#define PKT_X_OVF   BIT(6)  /* X overflow    */
#define PKT_Y_OVF   BIT(7)  /* Y overflow    */

/* ---- IRQ ---- */
#define MOUSE_IRQ 12

int mouse_subscribe_int(uint8_t *bit_no);
int mouse_unsubscribe_int(void);

int mouse_write_cmd(uint8_t cmd);
int mouse_disable_data_reporting(void);

void (mouse_ih)(void);

uint8_t mouse_get_byte(void);

#endif /* MOUSE_H */
