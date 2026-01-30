#include "util.h"

#define OP_CNT JAM+1
#define ADDR_CNT REL+1

/* Only focusing on legal instructions */
static const uint8_t opcode_to_byte[ADDR_CNT][OP_CNT] = {
    [IMM]     = { [LDY] = 0xA0, [CPY] = 0xC0, [CPX] = 0xE0, 
                  [LDX] = 0xA2, 
                  [ORA] = 0x09, [AND] = 0x29, [EOR] = 0x49, [ADC] = 0x69, [LDA] = 0xA9, [CMP] = 0xC9, [SBC] = 0xE9 },
    [ABS]     = { [JSR] = 0x20, 
                  [BIT] = 0x2C, [JMP] = 0x4C, [STY] = 0x8C, [LDY] = 0xAC, [CPY] = 0xCC, [CPX] = 0xEC, 
                  [ORA] = 0x0D, [AND] = 0x2D, [EOR] = 0x4D, [ADC] = 0x6D, [STA] = 0x8D, [LDA] = 0xAD, [CMP] = 0xCD, [SBC] = 0xED, 
                  [ASL] = 0x0E, [ROL] = 0x2E, [LSR] = 0x4E, [ROR] = 0x6E, [STX] = 0x8E, [LDX] = 0xAE, [DEC] = 0xCE, [INC] = 0xEE, },
    [ZPG]     = { [BIT] = 0x24, [STY] = 0x84, [LDY] = 0xA4, [CPY] = 0xC4, [CPX] = 0xE4,
                  [ORA] = 0x05, [AND] = 0x25, [EOR] = 0x45, [ADC] = 0x65, [STA] = 0x85, [LDA] = 0xA5, [CMP] = 0xC5, [SBC] = 0xE5,
                  [ASL] = 0x06, [ROL] = 0x26, [LSR] = 0x46, [ROR] = 0x66, [STX] = 0x86, [LDX] = 0xA6, [DEC] = 0xC6, [INC] = 0xE6, },
    [ABS_X]   = { [LDY] = 0xBC, 
                  [ORA] = 0x1D, [AND] = 0x3D, [EOR] = 0x5D, [ADC] = 0x7D, [STA] = 0x9D, [LDA] = 0xBD, [CMP] = 0xDD, [SBC] = 0xFD,
                  [ASL] = 0x1E, [ROL] = 0x3E, [LSR] = 0x5E, [ROR] = 0x7E, [DEC] = 0xDE, [INC] = 0xFE, },   
    [ABS_Y]   = { [ORA] = 0x19, [AND] = 0x39, [EOR] = 0x59, [ADC] = 0x79, [STA] = 0x99, [LDA] = 0xB9, [CMP] = 0xD9, [SBC] = 0xF9,
                  [LDX] = 0xBE, },
    [ZPG_X]   = { [STY] = 0x94, [LDY] = 0xB4,
                  [ORA] = 0x15, [AND] = 0x35, [EOR] = 0x55, [ADC] = 0x75, [STA] = 0x95, [LDA] = 0xB5, [CMP] = 0xD5, [SBC] = 0xF5,
                  [ASL] = 0x16, [ROL] = 0x36, [LSR] = 0x56, [ROR] = 0x76, [DEC] = 0xD6, [INC] = 0xF6, },
    [ZPG_Y]   = { [STX] = 0x96, [LDX] = 0xB6, },
    [IMPL]    = { [BRK] = 0x00, [RTI] = 0x40, [RTS] = 0x60,
                  [PHP] = 0x08, [PLP] = 0x28, [PHA] = 0x48, [PLA] = 0x68, [DEY] = 0x88, [TAY] = 0xA8, [INY] = 0xC8, [INX] = 0xE8,
                  [TXA] = 0x8A, [TAX] = 0xAA, [DEX] = 0xCA, [NOP] = 0xEA, [CLC] = 0x18,
                  [SEC] = 0x38, [CLI] = 0x58, [SEI] = 0x78, [TYA] = 0x98, [CLV] = 0xB8, [CLD] = 0xD8, [SED] = 0xF8,
                  [TXS] = 0x9A, [TSX] = 0xBA, },
    [IND]     = { [JMP] = 0x6C },
    [IDX_IND] = { [ORA] = 0x01, [AND] = 0x21, [EOR] = 0x41, [ADC] = 0x61, [STA] = 0x81, [LDA] = 0xA1, [CMP] = 0xC1, [SBC] = 0xE1, },
    [IND_IDX] = { [ORA] = 0x11, [AND] = 0x31, [EOR] = 0x51, [ADC] = 0x71, [STA] = 0x91, [LDA] = 0xB1, [CMP] = 0xD1, [SBC] = 0xF1, },
    [ACC]     = { [ASL] = 0x0A, [ROL] = 0x2A, [LSR] = 0x4A, [ROR] = 0x6A, },
    [REL]     = { [BPL] = 0x10, [BMI] = 0x30, [BVC] = 0x50, [BVS] = 0x70, [BCC] = 0x90, [BCS] = 0xB0, [BNE] = 0xD0, [BEQ] = 0xF0, }
};

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

uint8_t encode_op(opcode_t op, addr_mode_t am) {
    return opcode_to_byte[am][op];
}
