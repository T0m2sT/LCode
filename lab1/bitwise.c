#include "bitwise.h"
#include <stdarg.h>
#define TODO return 255

uint8_t clear(uint8_t msk, int pos) {
    return (msk & ( ~(1 << pos)));
}

uint8_t set(uint8_t msk, int pos) {
    return ((1 << pos) | msk); 
}

bool is_set(uint8_t msk, int pos) {
    return ((msk | (1 << pos)) == msk); 
}

uint8_t lsb(uint16_t wide_msk) {
    uint8_t res = 0x00;
    res |= wide_msk;
    return res;
}

uint8_t msb(uint16_t wide_msk) {
    uint8_t res = wide_msk >> 8;
    return res;
}

uint8_t mask(int pos, ...) {
    va_list args;
    va_start(args, pos);

    uint8_t msk = 0x0;
    int p = pos;
    while (p != MSK_END) {
        msk |= (1 << p);
        p = va_arg(args,int);
    }
    va_end(args);
    return msk;
}
