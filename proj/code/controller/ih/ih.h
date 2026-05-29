#pragma once

#include "fw/drivers/timer.h"
#include "fw/drivers/keyboard.h"
#include "fw/drivers/mouse.h"

int subscribe_interrupts();
int unsubscribe_interrupts();

void interrupts_handler(uint32_t irq_mask);

int ih_get_mouse_x();
int ih_get_mouse_y();
