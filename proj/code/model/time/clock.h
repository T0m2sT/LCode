#pragma once

#include "fw/drivers/rtc.h"

/**
 * @file time.h
 * @brief Real-time clock: periodic update and formatted time string
 */

/**
 * @brief Reads the current time from the RTC and updates the internal string
 *
 * Should be called once per RTC interrupt
 */
void time_update();

/**
 * @brief Returns the current time as a formatted string
 * @return Null-terminated string in HH:MM:SS format
 */
const char *time_get_string();
