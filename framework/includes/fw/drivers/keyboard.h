#pragma once

#include <lcom/lcf.h>
#include <stdint.h>
#include "fw/hw/i8042.h"

struct packet_scancode {
  bool make;
  unsigned char size;
  unsigned char bytes[2];
};

void set_scancode(uint8_t code);
uint8_t get_scancode();
bool get_error_keyboard();

int build_scancode(struct packet_scancode *ps);
void (keyboard_ih)();

int (keyboard_subscribe_int)(uint8_t *bit_no);
int (keyboard_unsubscribe_int)();
int (keyboard_print_scancode)(bool make, uint8_t size, uint8_t *bytes);
