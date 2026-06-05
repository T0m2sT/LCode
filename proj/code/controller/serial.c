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

/**
 * @brief Handles a single byte received from the UART.
 * 
 * Advances the state machine used to decode the serial protocol.
 * The states are:
 * - STATE_WAIT_START: Waits for the PKT_START_BYTE (0xFE).
 * - STATE_READ_CMD: Reads the SerialCommand ID.
 * - STATE_READ_LEN: Reads the length of the payload.
 * - STATE_READ_PAYLOAD: Accumulates the payload bytes into a buffer.
 * 
 * When a packet is fully received, it pushes an INPUT_EVENT_SERIAL to the event queue.
 * 
 * @param byte The received byte.
 */
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


/**
 * @brief Reads all available bytes from the serial port and processes them.
 * 
 * Calls the UART interrupt handler to fetch hardware FIFO data into the software FIFO,
 * then pops bytes from the software FIFO and feeds them to the state machine.
 */
void serial_process() {
  serial_ih(); 

  uint8_t data;
  while (serial_read_char(&data)) {
    serial_handle_byte(data);
  }
}

/**
 * @brief Sends a fully assembled packet over the UART.
 * 
 * Appends the PKT_START_BYTE at the beginning and transmits the array.
 * 
 * @param buf Array containing the packet (excluding the start byte, which is added at index 0).
 * @param len Total length of the packet to send.
 */
static void send_packet(uint8_t* buf, int len){
  buf[0]=PKT_START_BYTE;
  for (int i=0 ; i<len ; i++){
    serial_write_char(buf[i]);
  }
}

/**
 * @brief Constructs a specific serial packet and sends it.
 * 
 * Encapsulates the specific payload structures for each command defined in the protocol.
 * Also implements maximum payload constraints (MTU) to avoid UART FIFO overflow.
 * 
 * @param cmd The command to execute (SerialCommand).
 * @param payload Raw payload data to include.
 * @param len Length of the payload.
 * @param num_lines 16-bit integer used for row indexes or line counts (if applicable).
 */
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
      if (len>250) len=250;
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
      if (len>250) len=250;
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
      if (len>252) len=252;
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

