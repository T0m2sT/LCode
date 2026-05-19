#pragma once

#include <lcom/utils.h>

typedef enum {
  WARN,
  ERR,
  ERR_RTC,
  ERR_TIMER,
  ERR_KEYBOARD,
  ERR_MOUSE,
  ERR_VIDEO
} ErrorType;

void print_err_type(ErrorType error_type);
int fail(ErrorType error_type, const char *msg);
