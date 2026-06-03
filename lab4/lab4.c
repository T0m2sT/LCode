#include <lcom/lcf.h>
#include <lcom/lab4.h>

#include <stdbool.h>
#include <stdint.h>

#include "mouse.h"

int main(int argc, char *argv[]) {
  lcf_set_language("EN-US");
  lcf_trace_calls("/home/lcom/labs/lab4/trace.txt");
  lcf_log_output("/home/lcom/labs/lab4/output.txt");
  if (lcf_start(argc, argv))
    return 1;
  lcf_cleanup();
  return 0;
}

/* ---------------------------------------------------------------------------
 * Packet assembly state shared between mouse_test_packet and mouse_test_async.
 * The IH delivers one byte at a time; we accumulate into a 3-byte buffer and
 * only call mouse_print_packet when a full packet is ready.
 * ---------------------------------------------------------------------------*/
static uint8_t pkt_buf[3];
static uint8_t pkt_idx = 0;

/*
 * Process a single byte from the mouse IH into the packet assembler.
 * Returns true when a complete 3-byte packet has been assembled and printed.
 *
 * Synchronisation strategy (from the guide):
 *   Bit 3 of byte 1 is always 1.  If we are expecting byte 0 and the incoming
 *   byte does not have bit 3 set, it cannot be the first byte of a packet, so
 *   we discard it and stay at index 0.
 */
static bool process_byte(uint8_t byte) {
  /* Sync: skip bytes until we see a valid first byte */
  if (pkt_idx == 0 && !(byte & PKT_ALWAYS1))
    return false;

  pkt_buf[pkt_idx++] = byte;

  if (pkt_idx < 3)
    return false;

  /* Full packet received */
  pkt_idx = 0;

  struct packet pp;
  pp.bytes[0] = pkt_buf[0];
  pp.bytes[1] = pkt_buf[1];
  pp.bytes[2] = pkt_buf[2];

  pp.lb     = (pkt_buf[0] & PKT_LB) != 0;
  pp.rb     = (pkt_buf[0] & PKT_RB) != 0;
  pp.mb     = (pkt_buf[0] & PKT_MB) != 0;
  pp.x_ov   = (pkt_buf[0] & PKT_X_OVF) != 0;
  pp.y_ov   = (pkt_buf[0] & PKT_Y_OVF) != 0;

  /* Two's-complement delta: extend sign bit to full int16 */
  pp.delta_x = pkt_buf[1];
  if (pkt_buf[0] & PKT_X_SIGN)
    pp.delta_x |= 0xFF00;  /* sign-extend: value is negative */

  pp.delta_y = pkt_buf[2];
  if (pkt_buf[0] & PKT_Y_SIGN)
    pp.delta_y |= 0xFF00;

  mouse_print_packet(&pp);
  return true;
}

/* ---------------------------------------------------------------------------
 * mouse_test_packet -- read and display `cnt` PS/2 mouse packets.
 *
 * Steps:
 *   1. Subscribe mouse IRQ 12 (exclusive)
 *   2. Enable data reporting  (provided helper: mouse_enable_data_reporting)
 *   3. Event loop: call mouse_ih on each interrupt, assemble 3-byte packets
 *   4. After `cnt` packets: disable data reporting, unsubscribe
 * ---------------------------------------------------------------------------*/
int (mouse_test_packet)(uint32_t cnt) {
  uint8_t bit_no;
  if (mouse_subscribe_int(&bit_no) != 0)
    return 1;

  if (mouse_enable_data_reporting() != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  uint32_t irq_set = BIT(bit_no);
  uint32_t received = 0;
  pkt_idx = 0;

  int ipc_status;
  message msg;
  int r;

  while (received < cnt) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != OK) {
      printf("driver_receive failed: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & irq_set) {
            mouse_ih();
            if (process_byte(mouse_get_byte()))
              received++;
          }
          break;
        default:
          break;
      }
    }
  }

  mouse_disable_data_reporting();
  mouse_unsubscribe_int();
  return 0;
}

/* ---------------------------------------------------------------------------
 * mouse_test_async -- display mouse packets until `idle_time` seconds pass
 *                     with no mouse activity.
 *
 * Differences from mouse_test_packet:
 *   - Exit condition: inactivity timeout instead of packet count
 *   - Uses timer IRQ 0 to measure idle time
 *   - Must implement mouse_enable_data_reporting itself using 0xD4/0xF4
 *     (the guide forbids reusing the provided helper here)
 *
 * Timer 0 runs at sys_hz() Hz (typically 60 Hz in MINIX).
 * We count ticks; any mouse byte resets the counter to 0.
 * ---------------------------------------------------------------------------*/
int (mouse_test_async)(uint8_t idle_time) {
  uint8_t mouse_bit_no, timer_bit_no;

  if (mouse_subscribe_int(&mouse_bit_no) != 0)
    return 1;

  if (timer_subscribe_int(&timer_bit_no) != 0) {
    mouse_unsubscribe_int();
    return 1;
  }

  /* Enable data reporting using 0xD4 + 0xF4 (same as mouse_write_cmd) */
  if (mouse_write_cmd(MOUSE_ENABLE_DR) != 0) {
    mouse_unsubscribe_int();
    timer_unsubscribe_int();
    return 1;
  }

  uint32_t mouse_irq = BIT(mouse_bit_no);
  uint32_t timer_irq = BIT(timer_bit_no);

  uint32_t hz = sys_hz();           /* timer frequency; do not hardcode 60 */
  uint32_t idle_ticks = 0;
  uint32_t threshold  = (uint32_t)idle_time * hz;

  pkt_idx = 0;

  int ipc_status;
  message msg;
  int r;

  while (idle_ticks < threshold) {
    if ((r = driver_receive(ANY, &msg, &ipc_status)) != OK) {
      printf("driver_receive failed: %d\n", r);
      continue;
    }

    if (is_ipc_notify(ipc_status)) {
      switch (_ENDPOINT_P(msg.m_source)) {
        case HARDWARE:
          if (msg.m_notify.interrupts & mouse_irq) {
            mouse_ih();
            process_byte(mouse_get_byte());
            idle_ticks = 0;   /* any mouse activity resets the idle counter */
          }

          if (msg.m_notify.interrupts & timer_irq) {
            timer_int_handler();
            idle_ticks++;
          }
          break;
        default:
          break;
      }
    }
  }

  mouse_disable_data_reporting();
  mouse_unsubscribe_int();
  timer_unsubscribe_int();
  return 0;
}
