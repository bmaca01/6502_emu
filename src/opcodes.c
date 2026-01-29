#include "opcodes.h"
/**
 * See https://www.masswerk.at/6502/6502_instruction_set.html#layout
 * Could this have been just a simple table? maybe.
 */
opcode_t fetch_opcode(uint8_t b) {
    decoded_t decoded = decode_byte(b);
    switch (decoded.cc) {
        case 0: 
            switch (decoded.aaa) {
                case 0:
                    switch (decoded.bbb) {
                        case 0: return BRK; break;
                        case 2: return PHP; break;
                        case 4: return BPL; break;
                        case 6: return CLC; break;
                        default: return NOP; break;
                    } break;
                case 1:
                    switch (decoded.bbb) {
                        case 0: return JSR; break;
                        case 1:
                        case 3: return BIT; break;
                        case 2: return PLP; break;
                        case 4: return BMI; break;
                        case 6: return SEC; break;
                        default: return NOP; break;
                    } break;
                case 2:
                    switch (decoded.bbb) {
                        case 0: return RTI; break;
                        case 2: return PHA; break;
                        case 3: return JMP; break;
                        case 4: return BVC; break;
                        case 6: return CLI; break;
                        case 1: case 5: case 7:
                        default: return NOP; break;
                    } break;
                case 3:
                    switch (decoded.bbb) {
                        case 0: return RTS; break;
                        case 2: return PLA; break;
                        case 3: return JMP; break;
                        case 4: return BVS; break;
                        case 6: return SEI; break;
                        case 1: case 5: case 7:
                        default: return NOP; break;
                    } break;
                case 4:
                    switch (decoded.bbb) {
                        case 2: return DEY; break;
                        case 1: case 3:
                        case 5: return STY; break;
                        case 4: return BCC; break;
                        case 6: return TYA; break;
                        case 7: return SHY; break;
                        case 0: 
                        default: return NOP; break;
                    } break;
                case 5:
                    switch (decoded.bbb) {
                        case 0: case 1: case 3: case 5:
                        case 7: return LDY; break;
                        case 2: return TAY; break;
                        case 4: return BCS; break;
                        case 6: return CLV; break;
                        default: return NOP; break;
                    } break;
                case 6:
                    switch (decoded.bbb) {
                        case 0: case 1:
                        case 3: return CPY; break;
                        case 2: return INY; break;
                        case 4: return BNE; break;
                        case 6: return CLD; break;
                        case 5: case 7:
                        default: return NOP; break;
                    } break;
                case 7:
                    switch (decoded.bbb) {
                        case 0: case 1:
                        case 3: return CPX; break;
                        case 2: return INX; break;
                        case 4: return BEQ; break;
                        case 6: return SED; break;
                        case 5: case 7:
                        default: return NOP; break;
                    } break;
                default: return NOP; break;
            } break;
        case 1:
            switch (decoded.aaa) {
                case 0: return ORA; break;
                case 1: return AND; break;
                case 2: return EOR; break;
                case 3: return ADC; break;
                case 4:
                    if (decoded.bbb == 2) return NOP;
                    else return STA;
                    break;
                case 5: return LDA; break;
                case 6: return CMP; break;
                case 7: return SBC; break;
                default: return NOP; break;
            } break;
        case 2:
            switch (decoded.bbb) {
                case 0: case 4:
                    switch (decoded.hi) {
                        case 0x8: case 0xc:
                        case 0xe: return NOP; break;
                        case 0xa: return LDX; break;
                        default: return JAM; break;
                    } break;
                case 6:
                    switch (decoded.hi) {
                        case 0x1: case 0x3: case 0x5: 
                        case 0x7: case 0xd: 
                        case 0xf: return NOP; break;
                        case 0x9: return TXS; break;
                        case 0xb: return TSX; break;
                    } break;
                case 1: case 2: case 3: case 5:
                case 7:
                    switch (decoded.aaa) {
                        case 0: return ASL; break;
                        case 1: return ROL; break;
                        case 2: return LSR; break;
                        case 3: return ROR; break;
                        case 4: 
                            switch (b) {
                                case 0x86: case 0x8e:
                                case 0x96: return STX; break;
                                case 0x8a: return TXA; break;
                                case 0x9E: return SHX; break;
                                default: return NOP; break;
                            } break;
                        case 5:
                            if (decoded.bbb == 2) return TAX;
                            else return LDX;
                            break;
                        case 6:
                            if (decoded.bbb == 2) return DEX;
                            else return DEC;
                            break;
                        case 7:
                            if (decoded.bbb == 2) return NOP;
                            else return INC;
                            break;
                    } break;
                default: return NOP; break;
            } break;
        case 3:
            /* NMOS Illegal instructions */
            switch (decoded.aaa) {
                case 0:
                    if (decoded.bbb == 2) return ANC;
                    else return SLO;
                    break;
                case 1:
                    if (decoded.bbb == 2) return ANC;
                    else return RLA;
                    break;
                case 2:
                    if (decoded.bbb == 2) return ALR;
                    else return SRE;
                    break;
                case 3:
                    if (decoded.bbb == 2) return ARR;
                    else return RRA;
                    break;
                case 4:
                    switch (decoded.bbb) {
                        case 0: case 1: case 3: 
                        case 5: return SAX; break;
                        case 2: return ANE; break;
                        case 4: 
                        case 7: return SHA; break;
                        case 6: return TAS; break;
                        default: return NOP; break;
                    } break;
                case 5:
                    switch (decoded.bbb) {
                        case 0: case 1: case 3: 
                        case 4: case 5: 
                        case 7: return LAX; break;
                        case 2: return LXA; break;
                        case 6: return LAS; break;
                        default: return NOP; break;
                    } break;
                case 6:
                    if (decoded.bbb == 2) return SBX;
                    else return DCP;
                    break;
                case 7:
                    if (decoded.bbb == 2) return USBC;
                    else return ISC;
                    break;
            } break;
        default:
            return NOP;
            break;
    }
}

ins_type_t cat_opcode(opcode_t op) {
    switch (op) {
        case LDA: case LDX: case LDY: case STA: case STX:
        case STY: case TAX: case TAY: case TSX: case TXA:
        case TXS: case TYA: 
            return TRANS; break;
        case PHA: case PHP: case PLA: case PLP: 
            return STACK; break;
        case DEC: case DEX: case DEY: case INC:
        case INX: case INY:
            return INCDEC; break;
        case ADC: case SBC:
            return ARITH; break;
        case AND: case EOR: case ORA:
            return LOGIC; break;
        case ASL: case LSR: case ROL: case ROR:
            return SHIFT; break;
        case CLC: case CLD: case CLI: case CLV: case SEC:
        case SED: case SEI:
            return FLAG; break;
        case CMP: case CPX: case CPY:
            return COMP; break;
        case BIT:
            return BIT_T; break;
        case JMP: case JSR: case RTS:
            return JUMP; break;
        case BRK: case RTI:
            return IRPT; break;
        case NOP:
            return NOP_T; break;
        default:
            return ILLEGAL; break;
    }
}
