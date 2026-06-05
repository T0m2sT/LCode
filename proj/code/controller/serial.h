#pragma once

#include <lcom/lcf.h>

/** @defgroup Serial Serial
 * @{
 * @brief Serial communication protocol and packet formatting for Collaborative Editing.
 */

/**
 * @brief Synchronization byte used to identify the start of a serial packet.
 */
#define PKT_START_BYTE   0xFE

/**
 * @brief Structure representing a fully assembled serial packet (event).
 */
typedef struct {
  uint8_t cmd;                /**< @brief Command ID (from SerialCommand enum) */
  uint8_t payload_buf[256];   /**< @brief Array containing the raw payload data */
  uint8_t payload_len;        /**< @brief Number of valid bytes in payload_buf */
} SerialEvent;

/**
 * @brief Enumeration of all supported serial commands for Delta Sync.
 *
 * This protocol avoids sending the entire file on every change. Instead, it
 * transmits precise "Deltas" (insertions, deletions, and block replacements).
 */
typedef enum {
  CMD_INSERT_CHAR   = 0x01,  /**< @brief Insert a character. Payload: 1 byte */
  CMD_DELETE_CHAR   = 0x02,  /**< @brief Delete char before cursor. Payload: 0 bytes */
  CMD_MOVE_CURSOR   = 0x03,  /**< @brief Move remote cursor. Payload: 4 bytes (2 row, 2 col) */
  CMD_REPLACE_BLOCK = 0x05,  /**< @brief Prepare memory for block edit. Payload: 6 bytes (2 start_row, rows_deleted, rows_inserted) */
  CMD_UPDATE_LINE   = 0x06,  /**< @brief Overwrite a line. Payload: 2 bytes (row_index) + text (max 250 char) */
  CMD_FILE_START    = 0x10,  /**< @brief Start file share. Payload: 2 bytes (Numlines) + filename */
  CMD_FILE_LINE     = 0x11,  /**< @brief Send file line during sync. Payload: text (max 252 char)*/
} SerialCommand;

/**
 * @brief Processes incoming bytes from the serial port.
 *
 * Implements a state machine (WAIT_START -> READ_CMD -> READ_LEN -> READ_PAYLOAD)
 * to assemble bytes into `SerialEvent` structs and pushes them to the input queue.
 */
void serial_process();

/**
 * @brief Builds and transmits a serial packet over the UART.
 *
 * Encapsulates the payload with the START_BYTE, command ID, and length.
 *
 * @param cmd The command to execute (SerialCommand).
 * @param payload Pointer to the payload data buffer.
 * @param len Size of the payload in bytes.
 * @param num_lines 16-bit integer used for row indexes or line counts.
 */
void build_packet_serial(SerialCommand cmd, uint8_t* payload, int len, uint16_t num_lines);

/** @} */

