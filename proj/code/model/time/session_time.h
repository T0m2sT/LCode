#pragma once

#include <stdio.h>
#include "lcom/lcf.h"

/**
 * @file session_time.h
 * @brief Session timer: elapsed time since last reset, formatted as HH:MM:SS
 */

/**
 * @brief Resets the session timer to the current tick count
 *
 * Should be called when a new file is opened or the session starts
 */
void session_time_reset();

/**
 * @brief Returns the number of seconds elapsed since the last reset
 * @return Elapsed seconds as a 64-bit unsigned integer
 */
uint64_t session_time_get_seconds();

/**
 * @brief Formats the elapsed session time into a buffer as HH:MM:SS
 * @param buf Output buffer
 * @param n Size of the output buffer in bytes
 */
void session_time_format(char *buf, size_t n);
