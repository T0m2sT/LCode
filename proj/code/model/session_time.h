#pragma once

#include <stdio.h>

#include "lcom/lcf.h"

void session_time_reset();
uint64_t session_time_get_seconds();
void session_time_format(char *buf, size_t n);
