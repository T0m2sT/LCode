#include "fw/drivers/serial_port.h"
#include "fw/common/utils.h"
#include "fw/hw/uart.h"
#include "fw/common/queue.h"

#define BIT_RATE     115200
#define CONF_FCR     FCR_ENABLE_FIFOS | FCR_CLEAR_RX_FIFO | FCR_CLEAR_TX_FIFO | FCR_TRIGGER_8_BYTES 
#define CONF_LCR     LCR_DATA_8_BITS
#define CONF_IER     IER_DATA_AVAILABLE


static int hook_id = 4;
static queue_t rx_queue;
static bool serial_initialized = false;


void(serial_ih)() {
  uint8_t iir_val, lsr_val, data;

  if (util_sys_inb(COM1_BASE + IIR, &iir_val) != OK) return;

  if ((iir_val & IIR_NO_INT) == 0) {

    uint8_t int_origin = iir_val & IIR_INT_ID_MASK;

    // Data Ready (010) or FIFO Time-out (110) 
    if (int_origin == BIT(1) || int_origin == (BIT(1) | BIT(2))) {

      if (util_sys_inb(COM1_BASE + LSR, &lsr_val) != OK) return;

      // Hardware FIFO -> Software FIFO
      while ((lsr_val & LSR_RX_READY) && !queue_is_full(&rx_queue)) { 

        if ((lsr_val & (LSR_OVERRUN_ERR | LSR_PARITY_ERR | LSR_FRAME_ERR)) == 0) {

          util_sys_inb(COM1_BASE + RBR, &data);

          // put the data into software FIFO
          queue_push(&rx_queue, data);
        } else {
          
          util_sys_inb(COM1_BASE + RBR, &data);
        }

        util_sys_inb(COM1_BASE + LSR, &lsr_val);
      }
    }
  }
}


bool (serial_read_char)(uint8_t *data) {
  if (!serial_initialized) return false;
  return queue_pop(&rx_queue, data);
}

int (serial_write_char)(uint8_t data) {
  uint8_t lsr_val;
  
  if (util_sys_inb(COM1_BASE + LSR, &lsr_val) != OK) return 1;
  
  if (lsr_val & LSR_TX_READY) {
    if (sys_outb(COM1_BASE + THR, data) != OK) return 1;
    return 0;
  }
  
  return 1; 
}



int (serial_init)() {
  if (serial_initialized) return 0;

  queue_init(&rx_queue);

  uint32_t divisor = UART_FREQ / BIT_RATE;
  
  if (sys_outb(COM1_BASE + LCR, LCR_DLAB_BIT) != OK) return 1;
  
  if (sys_outb(COM1_BASE + DLL, divisor & 0xFF) != OK) return 1;
  if (sys_outb(COM1_BASE + DLM, (divisor >> 8) & 0xFF) != OK) return 1;
  
  if (sys_outb(COM1_BASE + LCR, CONF_LCR) != OK) return 1;
  if (sys_outb(COM1_BASE + FCR, CONF_FCR) != OK) return 1;
  if (sys_outb(COM1_BASE + IER, CONF_IER) != OK) return 1;

  serial_initialized = true;
  return 0;
}


int(serial_subscribe_int)(uint8_t *bit_no) {
  *bit_no = hook_id;

  if (sys_irqsetpolicy(COM1_IRQ, IRQ_REENABLE | IRQ_EXCLUSIVE, &hook_id) !=
      OK) {
    return fail(ERR_SERIAL, "serial_subscribe_int: sys_irqsetpolicy failed");
  }

  return 0;
}

int(serial_unsubscribe_int)() {
  if (sys_irqrmpolicy(&hook_id) != OK) {
    return fail(ERR_SERIAL, "serial_unsubscribe_int: sys_irqrmpolicy failed");
  }

  return 0;
}
