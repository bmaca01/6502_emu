#include "addressing.h"
#include "util.h"

addr_mode_t fetch_addr_mode(uint8_t b) {
    decoded_t decoded = decode_byte(b);
    switch (decoded.bbb) {
        case 0:
            if (decoded.cc % 2 == 1)    return IDX_IND;
            else if (b == 0x20)         return ABS;
            else if (decoded.aaa > 3)   return IMM;
            else                        return IMPL;
            break;
        case 1: return ZPG; break;
        case 2:
            if (decoded.cc % 2 == 1)    return IMM;
            else {
                if (decoded.lo == 0xa && decoded.hi < 0x8) 
                    return ACC;
                return IMPL;
            }
            break;
        case 3: 
            if (b == 0x6C)  return IND;
            else            return ABS; 
            break;
        case 4:
            if (decoded.cc == 0) return REL;
            else return IND_IDX;
            break;
        case 5: 
            switch (b) {
                case 0x96: case 0xB6: case 0x97: case 0xB7:
                    return ZPG_Y;
                    break;
                default: 
                    return ZPG_X;
                    break;
            } break;
        case 6: 
            if (decoded.cc % 2 == 0) return IMPL;
            else return ABS_Y;
            break;
        case 7: 
            switch (b) {
                case 0x9E: case 0xBE: case 0x9F: case 0xBF:
                    return ABS_Y;
                    break;
                default:
                    return ABS_X;
                    break;
            } break;
        default:
            return IMM;
            break;
    }
    return IMM;
}
