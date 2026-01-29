/**
 * Module responsible for 6502 ISA implementation / decoding
 * for emulator
 */
#ifndef OPCODES_H_
#define OPCODES_H_

#include <stdint.h>
#include <stddef.h>
enum opcode {
    /* Transfer */
    LDA,LDX,LDY,STA,STX,STY,TAX,TAY,TSX,TXA,TXS,TYA,

    /* Dec & Inc */
    DEC,DEX,DEY,INC,INX,INY,

    /* Flag */
    CLC,CLD,CLI,CLV,SEC,SED,SEI,

    /* Conditional Branch */
    BCC,BCS,BEQ,BMI,BNE,BPL,BVC,BVS,
    
    PHA,PHP,PLA,PLP,/* Stack */
    ADC,SBC,        /* Arith */
    AND,EOR,ORA,    /* Logical */  
    ASL,LSR,ROL,ROR,/* Shift & Rotate */
    CMP,CPX,CPY,    /* Comparisons */
    BIT,            /* Bit test */
    JMP,JSR,RTS,    /* Jump & Subroutine */
    BRK,RTI,        /* Interrupts */
    NOP,            /* NOP */

    /* Illegal NMOS instructions */
    ALR, ANC, ANC2, ANE, ARR, DCP,
    ISC, LAS, LAX, LXA, RLA, RRA,
    SAX, SBX, SHA, SHX, SHY, SLO,
    SRE, TAS, USBC, JAM
};

enum ins_type {
    TRANS,      /* Load, store, interregister transfer */
    STACK,      /* Stack instructions */
    INCDEC,     /* Increments and decrements */
    ARITH,      /* ADC and SBC */
    LOGIC,      /* AND, EOR, ORA */
    SHIFT,      /* ASL, LSR, ROL, ROR */
    FLAG,       /* Set / clear flags */
    COMP,       /* CMP, CPX, CPY */
    BIT_T,      /* BIT */
    BRANCH,     /* Conditional Branch */
    JUMP,       /* JMP, JSR, RTS */
    IRPT,       /* Interrupts: BRK / RTI */
    NOP_T,      /* NOP */
    ILLEGAL     /* Catch all illegal */
};

typedef enum opcode opcode_t;
typedef enum ins_type ins_type_t;

/**
 * 3-3-2 structure:
 * ex) $A9 - LDA #oper
 *     A9 = 1010 1001
 *          aaab bbcc
 * 
 *           cc: instruction group
 *          bbb: addressing mode
 *          aaa: operation from group
 */
opcode_t fetch_opcode(uint8_t b);
ins_type_t cat_opcode(opcode_t op);

#endif
