#include <lcom/lcf.h>
#include <stdint.h>
#include "../hw/i8042.h"


void (mouse_ih)(void);
bool get_error();
int build_packet(struct packet *pp);//devolve 1 se formou um packet

int mouse_disable_data_reporting();
int mouse_enable_data_reporting_1();//tive de _1 senão dava conflito

int (mouse_subscribe_int)(uint8_t *bit_no);
int (mouse_unsubscribe_int)();

int mouse_write_command(uint8_t cmd);

