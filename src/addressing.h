#ifndef ADDR_H_
#define ADDR_H_

#include <stdint.h>
#include <stddef.h>
#include "util.h"
enum addr_mode {
    /* Only need to lookahead one byte */
    IMM, ABS, ZPG, 
    ABS_X, ABS_Y,
    ZPG_X, ZPG_Y, 
    /* ==== */
    IMPL,                   // Implied
    IND,                    // Indirect (only for JMP)
    IDX_IND,                // Indexed Indirect (OP (zpg,X))
    IND_IDX,                // Indirect Indexed (OP (zpg),Y)
    ACC,                    // Accumulator
    REL                     // Relative
};
typedef enum addr_mode addr_mode_t;

addr_mode_t fetch_addr_mode(uint8_t b);

#endif
