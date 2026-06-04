#pragma once

#include <lcom/lcf.h>


// Frequência do Transmissor
#define UART_FREQ 115200 //já com a divisão por 16

// Endereços Base e IRQs
#define COM1_BASE 0x3F8 
#define COM1_IRQ  4     

// Offsets dos Registos (DLAB = 0)
#define RBR   0 // Receiver Buffer Reg - Read 
#define THR   0 // Transmitter Holding Reg - Write
#define IER   1 // Interrupt Enable Reg - Read/Write
#define IIR   2 // Interrupt Identification Reg - Read
#define FCR   2 // FIFO Control Reg - Write
#define LCR   3 // Line Control Reg - Read/Write
#define LSR   5 // Line Status Reg - Read

// Offsets dos Registos (DLAB = 1)
#define DLL   0 // Divisor Latch LSB 
#define DLM   1 // Divisor Latch MSB 

// IER (Interrupt Enable Register)
#define IER_DATA_AVAILABLE  BIT(0) // Interrupção por dados recebidos
#define IER_THR_EMPTY       BIT(1) // Interrupção por THR vazio
#define IER_LCR_ERROR       BIT(2) // Interrupção por erro na linha de receção

// IIR (Interrupt Identification Register)
#define IIR_NO_INT          BIT(0) // Sem interrupção pendente
#define IIR_INT_ID_MASK     0x0E   // Máscara para identificar o tipo de interrupção

// FCR (FIFO Control Register)
#define FCR_ENABLE_FIFOS      BIT(0) // Habilitar FIFOs
#define FCR_CLEAR_RX_FIFO     BIT(1) // Limpar FIFO de receção
#define FCR_CLEAR_TX_FIFO     BIT(2) // Limpar FIFO de transmissão
#define FCR_TRIGGER_1_BYTE    0x00   // Trigger a 1 byte
#define FCR_TRIGGER_4_BYTES   0x40   // Trigger a 4 bytes
#define FCR_TRIGGER_8_BYTES   0x80   // Trigger a 8 bytes
#define FCR_TRIGGER_14_BYTES  0xC0   // Trigger a 14 bytes

// LCR (Line Control Register)
#define LCR_DATA_8_BITS   0x03   // Tamahno dos dados (bits 0 e 1)
#define LCR_STOP_BIT      BIT(2) // Stop bit (1 ou 2 bits)
#define LCR_PARITY_BIT    BIT(3) // Paridade 
#define LCR_BREAK_BIT     BIT(6) // Break control
#define LCR_DLAB_BIT      BIT(7) // Divisor Latch Access Bit

// LSR (Line Status Register)
#define LSR_RX_READY    BIT(0) // Há dados prontos para serem lidos (receiver ready)
#define LSR_OVERRUN_ERR BIT(1) // Buffer de receção cheio (overrun error) 
#define LSR_PARITY_ERR  BIT(2) // Bit de paridade errado (parity error)
#define LSR_FRAME_ERR   BIT(3) // Stop bit errado (framing error) 
#define LSR_BREAK_INT   BIT(4) // Break interrupt
#define LSR_TX_READY    BIT(5) // Pronto para enviar dados (THR vazio)
#define LSR_TX_EMPTY    BIT(6) // Acabou o envio de dados (transmissor vazio) 
#define LSR_ERR_FIFO    BIT(7) // Pelo menos um erro na FIFO de receção 




