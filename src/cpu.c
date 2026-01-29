#include "cpu.h"
#include "opcodes.h"
#include "addressing.h"
#include <stdlib.h>
#include <string.h>

/* Forward declarations */
static void cpu_instruction_exec(CPU* cpu, uint8_t* curr_cycles,
                                 opcode_t opcode, addr_mode_t a_mode,
                                 uint16_t operand, uint16_t ea);

static void cpu_resolve_ea(CPU* cpu, addr_mode_t curr_am, int16_t *operand, 
                           uint16_t *ea, bool *cross_page);

static void cpu_increment_cycles(addr_mode_t curr_am, opcode_t curr_oc, 
                                 ins_type_t curr_it, bool cp, uint8_t *c);

struct CPU {
    uint8_t a;
    uint8_t x;
    uint8_t y;
    uint8_t sp;
    uint16_t pc;
    uint8_t status;

    Memory* mem;

    uint64_t total_cycles;
    bool halted;

    // Internal registers
    uint16_t mar;   // Memory Address Register
    uint8_t mdr;    // Memory Data Register
    uint8_t cir;    // Current Instruction Register
};

CPU* cpu_create(Memory* mem) {
    CPU* c = malloc(sizeof(CPU));
    if (!c) {
        printf("Failed to init cpu\n");
        exit(1);
    }
    c->mem = mem;
    cpu_reset(c);
    return c;
}

void cpu_destroy(CPU* cpu) {
    if (cpu->mem) memory_destroy(cpu->mem);
    free(cpu);
    return;
}

void cpu_reset(CPU* cpu) {
    if (!cpu) return;
    cpu->sp = 0xFD;
    cpu->status = FLAG_U | FLAG_I;

    /* Read from reset vector */
    uint8_t lo = memory_read(cpu->mem, 0xFFFC);
    uint8_t hi = memory_read(cpu->mem, 0xFFFD);
    cpu->pc = (hi << 8) | lo;

    cpu->halted = false;

    return;
}

uint8_t cpu_step(CPU* cpu) {
    uint8_t curr_cycles = 0;
    bool cross_page = false;

    /* 1. Fetch opcode */
    cpu->mar = cpu->pc;
    cpu->mdr = memory_read(cpu->mem, cpu->mar);
    cpu->cir = cpu->mdr;

    /* 2. Decode */
    opcode_t curr_opcode = fetch_opcode(cpu->cir);
    addr_mode_t curr_addr_mode = fetch_addr_mode(cpu->cir);
    ins_type_t curr_ins_type = cat_opcode(curr_opcode);
    curr_cycles++;

    /* 3. Execute */
    /* 3. a) resolve the address */
    uint16_t operand = 0;
    uint16_t e_addr = 0;
    cpu_resolve_ea(cpu, curr_addr_mode, &operand, &e_addr, &cross_page);

    /* 3. b) calculate cycle counts after address resolution */
    cpu_increment_cycles(curr_addr_mode, curr_opcode, curr_ins_type, cross_page, &curr_cycles);

    // Handle per operation additional cycle increment
    cpu_instruction_exec(cpu, &curr_cycles, curr_opcode, curr_addr_mode, operand, e_addr);

    /* 4. Update PC */
    cpu->pc = cpu->mar + 1;

    return curr_cycles;
}

static void cpu_instruction_exec(CPU* cpu, uint8_t* curr_cycles,
                                 opcode_t opcode, addr_mode_t a_mode,
                                 uint16_t operand, uint16_t ea) {
    /**
     * At this point, (hopefully) we know:
     * - opcode
     * - addressing mode
     * - operand
     * - effective address
     * which means we can pass this info to execute an instruction..?
     */

    uint8_t *src, *dst, val;
    switch (opcode) {
        case LDA: case LDX: case LDY:
            if (a_mode == IMM)      val = operand;
            else                    val = memory_read(cpu->mem, ea);

            if (opcode == LDA)      dst = &cpu->a;  //cpu->a = val;
            if (opcode == LDX)      dst = &cpu->x;  //cpu->x = val;
            if (opcode == LDY)      dst = &cpu->y;  //cpu->y = val;

            *dst = val;

            if (*dst == 0)          cpu->status |= FLAG_Z;
            if ((*dst & 0x80)>>7)   cpu->status |= FLAG_N;
            /* Cycle already counted in addressing mode resolution */
            break;
        case STA: case STX: case STY:
            if (opcode == STA)  memory_write(cpu->mem, ea, cpu->a);
            if (opcode == STX)  memory_write(cpu->mem, ea, cpu->x);
            if (opcode == STY)  memory_write(cpu->mem, ea, cpu->y);
            /* Cycle already counted in addressing mode resolution */
            break;
        case TAX: case TAY: case TSX: case TXA: case TXS: case TYA:
            // Only TXS does not set flags
            if (opcode == TAX || opcode == TSX) dst = &cpu->x;
            if (opcode == TXA || opcode == TYA) dst = &cpu->a;
            if (opcode == TAY)                  dst = &cpu->y;
            if (opcode == TXS)                  dst = &cpu->sp;

            if (opcode == TAX || opcode == TAY) src = &cpu->a;
            if (opcode == TXA || opcode == TXS) src = &cpu->x;
            if (opcode == TSX)                  src = &cpu->sp;
            if (opcode == TYA)                  src = &cpu->y;

            *dst = *src;

            if (opcode != TXS) {
                if (*dst == 0)              cpu->status |= FLAG_Z;
                if ((*dst & 0x80)>>7)       cpu->status |= FLAG_N;
            }
            /* Transfer instructions are implied mode: +1 for internal operation */
            (*curr_cycles)++;
            break;
        case PHA: case PHP:
            src = (opcode == PHA) ? &cpu->a : &cpu->status;

            if (opcode == PHP)
                *src |= (FLAG_B | FLAG_U);

            memory_write(cpu->mem, (0x0100 | (--cpu->sp)), *src);
            (*curr_cycles)++;
            (*curr_cycles)++;
            break;
        case PLA: case PLP:
            dst = (opcode == PLA) ? &cpu->a : &cpu->status;
            val = memory_read(cpu->mem, (0x0100 | cpu->sp));
            cpu->sp++;

            *dst = val;

            if (opcode == PLA){
                if (*dst == 0)              cpu->status |= FLAG_Z;
                if ((*dst & 0x80)>>7)       cpu->status |= FLAG_N;
            }

            (*curr_cycles)++;
            (*curr_cycles)++;
            (*curr_cycles)++;
            break;
        case NOP:
            (*curr_cycles)++;
            break;
        default:
            printf("[DEBUG] cpu.c -> cpu_instruction_exec(): invalid opcode\n");
            exit(1);        // TODO: clean up function before exit?
            break;
    }
    return;
}

static void cpu_resolve_ea(CPU* cpu, addr_mode_t curr_am, int16_t *operand_ptr, 
                           uint16_t *ea_ptr, bool *cross_page_ptr) {
    bool cross_page = false;
    uint16_t operand = 0;
    uint16_t e_addr = 0;
    switch (curr_am) {
        case IMPL: break;   // Handle per instruction..?
        case ACC:
            operand = cpu->a;
            break;
        case IMM:
            operand = memory_read(cpu->mem, ++cpu->mar);
            break;
        case REL:
            operand = (int8_t)memory_read(cpu->mem, ++cpu->mar);
            break;
        case ABS: case ABS_X: case ABS_Y: case IND:
            // Get address; increment MAR +2
            operand = (memory_read(cpu->mem, ++cpu->mar)) 
                    | (memory_read(cpu->mem, ++cpu->mar)<<8);
            switch (curr_am) {
                case ABS:   e_addr = operand;           break;
                case ABS_X: e_addr = operand + cpu->x;  break; // TODO: how to handle overflow..?
                case ABS_Y: e_addr = operand + cpu->y;  break;
                case IND:   
                    e_addr = (memory_read(cpu->mem, operand))
                           | (memory_read(cpu->mem, (operand & 0xFF00) | ((operand + 1) & 0x00FF))<<8);
                    break;
                default:
                    printf("[DEBUG] cpu.c -> cpu_step(): invalid absolute addressing\n");
                    exit(1);
                    break;
            } 
            if (curr_am == ABS_X || curr_am == ABS_Y) {
                cross_page = (e_addr & 0xFF00) != (operand & 0xFF00);
            }
            break;
        case ZPG: case ZPG_X: case ZPG_Y:
        case IND_IDX: case IDX_IND:
            operand = memory_read(cpu->mem, ++cpu->mar);
            switch (curr_am) {
                case ZPG:   e_addr = operand; break;
                case ZPG_X: e_addr = (operand + cpu->x) & 0x00FF; break;
                case ZPG_Y: e_addr = (operand + cpu->y) & 0x00FF; break;
                case IDX_IND:
                    e_addr = (memory_read(cpu->mem, (operand + cpu->x) & 0x00FF))
                           | (memory_read(cpu->mem, (operand + cpu->x + 1) & 0x00FF) << 8);
                    break;
                case IND_IDX:
                    e_addr = ((memory_read(cpu->mem, operand))
                           |  (memory_read(cpu->mem, (operand + 1) & 0xFF) << 8))
                           +  cpu->y;
                    cross_page = ((e_addr & 0xFF00)>>8) != (memory_read(cpu->mem, operand + 1));
                    break;
                default:
                    printf("[DEBUG] cpu.c -> cpu_step(): invalid zero-page addressing\n");
                    exit(1);
                    break;
            } break;
        default:
            printf("[DEBUG] cpu.c -> cpu_step(): invalid addressing type\n");
            exit(1);
            break;
    }

    *operand_ptr = operand;
    *ea_ptr = e_addr;
    *cross_page_ptr = cross_page;
    return;
}

static void cpu_increment_cycles(addr_mode_t curr_am, opcode_t curr_oc, 
                                 ins_type_t curr_it, bool cp, uint8_t *c) {
    /*
     * Cycle counting based on 6502 reference:
     *   IMM:          2 (opcode + operand)
     *   ZPG:          3 (opcode + ZP addr + read)
     *   ZPG_X/Y:      4 (opcode + ZP addr + index calc + read)
     *   ABS:          4 (opcode + addr_lo + addr_hi + read)
     *   ABS_X/Y:      4 or 5 (opcode + addr_lo + addr_hi + read [+1 if page cross])
     *   (IND,X):      6 (opcode + ptr + index calc + target_lo + target_hi + read)
     *   (IND),Y:      5 or 6 (opcode + ptr + base_lo + base_hi + read [+1 if page cross])
     *   IND (JMP):    5 (opcode + addr_lo + addr_hi + target_lo + target_hi)
     *   IMPL/ACC:     2 (opcode + internal operation)
     *
     * Store operations: same as read, but ABS_X/Y and (IND),Y always take the penalty
     */
    opcode_t curr_opcode = curr_oc;
    ins_type_t curr_ins_type = curr_it;
    addr_mode_t curr_addr_mode = curr_am;
    uint8_t curr_cycles = 0;

    bool cross_page = cp;
    bool is_store = (curr_opcode == STA || curr_opcode == STX || curr_opcode == STY);
    bool is_rmw = (curr_ins_type == SHIFT || curr_ins_type == INCDEC)
                  && curr_addr_mode != ACC
                  && curr_addr_mode != IMPL;
    bool needs_penalty = is_store || is_rmw || cross_page;
    switch (curr_addr_mode) {
        case IDX_IND:   /* (IND,X): 6 cycles */
            curr_cycles += 5;
            break;
        case IND_IDX:   /* (IND),Y: 5 cycles (+1 if penalty) */
            curr_cycles += 4;
            if (needs_penalty) curr_cycles++;
            break;
        case IND:       /* IND (JMP only): 5 cycles */
            curr_cycles += 4;
            break;
        case ABS_X:
        case ABS_Y:     /* ABS,X/Y: 4 cycles (+1 if penalty) */
            curr_cycles += 3;
            if (needs_penalty) curr_cycles++;
            break;
        case ABS:       /* ABS: 4 cycles */
            curr_cycles += 3;
            break;
        case ZPG_X:
        case ZPG_Y:     /* ZPG,X/Y: 4 cycles */
            curr_cycles += 3;
            break;
        case ZPG:       /* ZPG: 3 cycles */
            curr_cycles += 2;
            break;
        case IMM:       /* IMM: 2 cycles */
            curr_cycles += 1;
            break;
        case ACC:
        case IMPL:      /* Implied/Accumulator: handled in instruction exec */
            break;
        default:
            printf("[DEBUG] cpu.c -> cpu_step(): invalid addressing type\n");
            exit(1);
            break;
    }

    (*c) += curr_cycles;
    return;
}


void cpu_nmi(CPU* cpu) {
    return;
}

void cpu_irq(CPU* cpu) {
    return;
}

uint8_t  cpu_get_a(CPU* cpu)      { return cpu->a; }
uint8_t  cpu_get_x(CPU* cpu)      { return cpu->x; }
uint8_t  cpu_get_y(CPU* cpu)      { return cpu->y; }
uint8_t  cpu_get_sp(CPU* cpu)     { return cpu->sp; }
uint16_t cpu_get_pc(CPU* cpu)     { return cpu->pc; }
uint8_t  cpu_get_status(CPU* cpu) { return cpu->status; }

void cpu_set_a(CPU* cpu, uint8_t val)       { cpu->a = val; }
void cpu_set_x(CPU* cpu, uint8_t val)       { cpu->x = val; }
void cpu_set_y(CPU* cpu, uint8_t val)       { cpu->y = val; }
void cpu_set_sp(CPU* cpu, uint8_t val)      { cpu->sp = val; }
void cpu_set_pc(CPU* cpu, uint16_t val)     { cpu->pc = val; }
void cpu_set_status(CPU* cpu, uint8_t val)  { cpu->status = val; }

Memory* cpu_get_memory(CPU* cpu) { return cpu->mem; }
