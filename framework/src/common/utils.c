#include <lcom/lcf.h>

#include <stdint.h>

#include "fw/common/utils.h"

// Reads value from port
int (util_sys_inb)(int port, uint8_t *value) {
  
  uint32_t temp;

  if (sys_inb(port, &temp) != OK)
    return 1;
  
  *value = (uint8_t) temp;

  return 0;
}

void print_err_type(ErrorType error_type) {
  switch (error_type)
  {
    case WARN:
      fprintf(stderr, "\nWarning\n");
      break;

    case ERR:
      fprintf(stderr, "\nError\n");
      break;
    
    case ERR_RTC:
      fprintf(stderr, "\nRTC Error\n");
      break;
    
    case ERR_TIMER:
      fprintf(stderr, "\nTimer Error\n");
      break;
    
    case ERR_KEYBOARD:
      fprintf(stderr, "\nKeyboard Error\n");
      break;
    
    case ERR_MOUSE:
      fprintf(stderr, "\nMouse Error\n");
      break;
    
    case ERR_VIDEO:
      fprintf(stderr, "\nVideo Error\n");
      break;

    default:
      break;
  }
}

// Prints message before fail
int fail(ErrorType error_type, const char *msg) {
  print_err_type(error_type);
  fprintf(stderr, "%s\n", msg);
  return 1;
}

