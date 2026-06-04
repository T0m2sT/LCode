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

static void send_packet(uint8_t* buf, int len){
  buf[0]=PKT_START_BYTE;
  for (int i=0 ; i<len ; i++){
    serial_write_char(buf[i]);
  }
}

void build_packet_serial(SerialCommand cmd, uint8_t* payload, int len, uint16_t num_lines) {
  switch(cmd) {
    case CMD_INSERT_CHAR: {
      uint8_t buf[4];
      buf[1] = CMD_INSERT_CHAR;
      buf[2] = 0x01;
      buf[3] = payload[0]; 
      send_packet(buf, 4);
      break;
    }
    case CMD_DELETE_CHAR: {
      uint8_t buf[3];
      buf[1] = CMD_DELETE_CHAR;
      buf[2] = 0x00;
      send_packet(buf, 3);
      break;
    }
    case CMD_MOVE_CURSOR: {
      uint8_t buf[7];
      buf[1] = CMD_MOVE_CURSOR;
      buf[2] = 0x04;
      for (int i=0; i<4; i++) {
        buf[i+3] = payload[i];
      }
      send_packet(buf, 7);
      break;
    }
    case CMD_REPLACE_BLOCK:{
      uint8_t buf[9];
      buf[1] = CMD_REPLACE_BLOCK;
      buf[2] = 0x06;
      for (int i=0; i<6;i++){
        buf[i+3]=payload[i];
      }
      send_packet(buf,9);
      break;
    }
    case CMD_UPDATE_LINE:{
      uint8_t buf[5+len];
      buf[1] = CMD_UPDATE_LINE;
      buf[2] = len+2;
      buf[3] = (num_lines >> 8) & 0xFF; 
      buf[4] = num_lines & 0xFF;  
      for (int i=0; i<len; i++) {
        buf[i+5] = payload[i];
      }
      send_packet(buf,5+len);
      break;
    }
    case CMD_FILE_START: {
      uint8_t buf[5 + len]; 
      buf[1] = CMD_FILE_START;
      buf[2] = len + 2; 
      buf[3] = (num_lines >> 8) & 0xFF; 
      buf[4] = num_lines & 0xFF;        
      for (int i=0; i<len; i++) {
        buf[i+5] = payload[i];
      }
      send_packet(buf, 5 + len);
      break;
    }
    case CMD_FILE_LINE: {
      uint8_t buf[3 + len];
      buf[1] = CMD_FILE_LINE;
      buf[2] = len;
      for (int i=0; i<len; i++) {
        buf[i+3] = payload[i];
      }
      send_packet(buf, 3 + len);
      break;
    }
  }
}

