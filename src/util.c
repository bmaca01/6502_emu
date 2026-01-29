#include "util.h"

uint8_t random_byte() {
    return (uint8_t)(rand() % 256);
}

char* byte_to_bits(uint8_t b) {
    static char buf[10];  // "bbbb bbbb\0"
    for (int i = 0; i < 4; i++) {
        buf[i] = (b & (0x80 >> i)) ? '1' : '0';
    }
    buf[4] = ' ';
    for (int i = 0; i < 4; i++) {
        buf[5 + i] = (b & (0x08 >> i)) ? '1' : '0';
    }
    buf[9] = '\0';
    return buf;
}

decoded_t decode_byte(uint8_t b) {
    decoded_t rtn;
    rtn.aaa = (b & 0xE0)>>5;
    rtn.bbb = (b & 0x1c)>>2;
    rtn.cc = b & 0x03;
    rtn.lo = b & 0x0F;
    rtn.hi = (b & 0xF0)>>4;
    return rtn;
}
