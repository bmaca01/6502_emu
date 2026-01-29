#ifndef UTIL_H_
#define UTIL_H_

#include <stdint.h>
#include <stdlib.h>
#include "opcodes.h"
#include "addressing.h"

struct Decoded {
    uint8_t aaa;
    uint8_t bbb;
    uint8_t cc;
    uint8_t lo;
    uint8_t hi;
};

typedef struct Decoded decoded_t;

uint8_t random_byte();
char* byte_to_bits(uint8_t b);
decoded_t decode_byte(uint8_t b);
uint8_t encode_op(opcode_t op, addr_mode_t am);

#endif
