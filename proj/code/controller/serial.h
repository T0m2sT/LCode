#pragma once

#include <lcom/lcf.h>

#define PKT_START_BYTE   0xFE

typedef struct {
  uint8_t cmd;
  uint8_t payload_buf[256];
  uint8_t payload_len; 
} SerialEvent;

typedef enum {
  CMD_INSERT_CHAR   = 0x01,  // Payload: 1 byte
  CMD_DELETE_CHAR   = 0x02,  // Payload: 0 bytes
  CMD_MOVE_CURSOR   = 0x03,  // Payload: 4 bytes (coordinates)
  CMD_REPLACE_BLOCK = 0x05,  // Payload: 6 bytes (2 start, 2 deleted, 2 inserted)
  CMD_UPDATE_LINE   = 0x06,  // Payload: 2 bytes (row_index) + String (line content)
  CMD_FILE_START   = 0x10,   // Payload: 2 bytes (number of lines) + String (filename)
  CMD_FILE_LINE    = 0x11,   // Payload: String (line content)
} SerialCommand;

void serial_process();
void build_packet_serial(SerialCommand cmd, uint8_t* payload, int len, uint16_t num_lines);

