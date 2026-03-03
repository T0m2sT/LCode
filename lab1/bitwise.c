#include "bitwise.h"

#define TODO return 255

uint8_t clear(uint8_t msk, int pos) {
  uint8_t clear_mask = (uint8_t) ~(1u << pos);

  return msk & clear_mask;
}

uint8_t set(uint8_t msk, int pos) {
  uint8_t set_mask = (uint8_t) (1u << pos);

  return msk | set_mask;
}

bool is_set(uint8_t msk, int pos) {
  uint8_t is_set_mask = (uint8_t) (1u << pos);

  return (msk & is_set_mask) != 0;
}

uint8_t lsb(uint16_t wide_msk) {
  return (uint8_t) wide_msk;
}

uint8_t msb(uint16_t wide_msk) {
  return (uint8_t) (wide_msk >> 8);
}

uint8_t mask(int pos, ...) {
  uint8_t msk = 0; 
  va_list args;
  va_start(args, pos);

  while (pos != MSK_END) {
    if (pos >= 0 && pos < 8) {
     msk |= (uint8_t) (1u << pos); 
    }

    pos  = va_arg(args, int);
  }

  va_end(args);
  return msk;
}
