#include "controller/serial.h"
#include "controller/commands.h"
#include "controller/input/events.h"
#include "fw/drivers/serial_port.h"
#include "model/editor.h"
#include "render_flag.h"

typedef enum {
  STATE_WAIT_START,
  STATE_READ_CMD,
  STATE_READ_LEN,
  STATE_READ_PAYLOAD
} ProtocolState;

static ProtocolState state = STATE_WAIT_START;
static uint8_t current_cmd;
static uint8_t payload_len;
static uint8_t payload_buf[256];
static int payload_idx = 0;

static void serial_handle_byte(uint8_t byte) {
  switch (state) {
    
    case STATE_WAIT_START:
      if (byte == PKT_START_BYTE) {
        state = STATE_READ_CMD;
      }
      break;

    case STATE_READ_CMD:
      current_cmd = byte;
      state = STATE_READ_LEN;
      break;

    case STATE_READ_LEN:
      payload_len = byte;
      payload_idx = 0;
      if (payload_len == 0) {
        InputEvent ev;
        ev.type = INPUT_EVENT_SERIAL;
        ev.data.serial.cmd = current_cmd;
        ev.data.serial.payload_len = payload_len;
      
        input_event_push(ev);
        
        state = STATE_WAIT_START;
      } 
      else {
        state = STATE_READ_PAYLOAD;
      }
      break;

    case STATE_READ_PAYLOAD:
      payload_buf[payload_idx] = byte;
      payload_idx++;
      if (payload_idx >= payload_len) {
        InputEvent ev;
        ev.type = INPUT_EVENT_SERIAL;
        ev.data.serial.cmd = current_cmd;
        ev.data.serial.payload_len = payload_len;
        
        memcpy(ev.data.serial.payload_buf, payload_buf, payload_len);
        
        input_event_push(ev);
        
        state = STATE_WAIT_START;
      }
      break;
  }
}


void serial_process() {
  serial_ih(); 

  uint8_t data;
  while (serial_read_char(&data)) {
    serial_handle_byte(data);
  }
}
