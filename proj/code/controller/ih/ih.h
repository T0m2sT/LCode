#pragma once

#include "fw/drivers/timer.h"
#include "fw/drivers/keyboard.h"
#include "fw/drivers/mouse.h"
#include "fw/drivers/serial_port.h"

/**
 * @file ih.h
 * @brief Interrupt subscription, unsubscription, and per-device interrupt handlers
 */

/**
 * @brief Subscribes timer, keyboard, mouse, and serial interrupts
 * @return 0 on success. Non-zero if any subscription fails (previously subscribed devices are unsubscribed)
 */
int subscribe_interrupts();

/**
 * @brief Unsubscribes all hardware interrupts
 * @return 0 if all unsubscriptions succeeded. Non-zero if any failed
 */
int unsubscribe_interrupts();

/**
 * @brief Handles a timer interrupt: ticks the command bar and updates the clock once per second
 */
void timer_handler();

/**
 * @brief Handles a keyboard interrupt: reads a scancode and pushes a KeyEvent if the packet is complete
 */
void keyboard_handler();

/**
 * @brief Handles a mouse interrupt: reads a packet and pushes a MouseEvent if the packet is complete
 */
void mouse_handler();

/**
 * @brief Dispatches hardware interrupts to the appropriate device handler
 * @param irq_mask Bitmask of pending interrupt lines from the MINIX IPC notification
 */
void interrupts_handler(uint32_t irq_mask);
