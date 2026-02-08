#include "test_common.h"
#include "bus.h"

/*
 * Integration Tests - Small 6502 Programs
 *
 * Each test runs a small program (10-20 instructions) and verifies
 * the final state of CPU registers, flags, and memory.
 */

/* ===================== Helper Functions ===================== */

/* Run N instructions, return total cycles */
static uint16_t run_program(CPU* cpu, int max_instructions) {
    uint16_t total_cycles = 0;
    for (int i = 0; i < max_instructions; i++) {
        total_cycles += cpu_step(cpu);
    }
    return total_cycles;
}

/* ===================== Loop Tests ===================== */

/*
 * Counter Loop: DEX until zero
 *
 *      LDX #$05        ; X = 5
 * loop: DEX            ; X--
 *      BNE loop        ; if X != 0, branch back
 *
 * Final state: X = 0, Z = 1, N = 0
 */
TEST(test_counter_loop) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    uint8_t prog[] = {
        0xA2, 0x05,     /* LDX #$05 */
        0xCA,           /* DEX */
        0xD0, 0xFD      /* BNE -3 (back to DEX) */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run: LDX + (DEX + BNE)*5 = 1 + 10 = 11 instructions */
    run_program(cpu, 11);

    CHECK_EQ(cpu_get_x(cpu), 0x00);
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z flag should be set");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N flag should be clear");
    check_pc(cpu, 0x0205);  /* Past BNE when not taken */

    cpu_destroy(cpu);
}

/*
 * Memory Fill: Fill 4 bytes with $AA
 *
 *      LDX #$04        ; 4 bytes
 *      LDA #$AA        ; fill value
 * loop: DEX
 *      STA $0300,X     ; store at $0300+X
 *      BNE loop
 *
 * Final state: $0300-$0303 = $AA, X = 0
 */
TEST(test_memory_fill) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    uint8_t prog[] = {
        0xA2, 0x04,         /* LDX #$04 */
        0xA9, 0xAA,         /* LDA #$AA */
        0xCA,               /* DEX */
        0x9D, 0x00, 0x03,   /* STA $0300,X */
        0xD0, 0xFA          /* BNE -6 (back to DEX) */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run: LDX + LDA + (DEX + STA + BNE)*4 = 2 + 12 = 14 instructions */
    run_program(cpu, 14);

    CHECK_EQ(cpu_get_x(cpu), 0x00);
    CHECK_EQ(cpu_get_a(cpu), 0xAA);
    CHECK_EQ(bus_read(bus, 0x0300), 0xAA);
    CHECK_EQ(bus_read(bus, 0x0301), 0xAA);
    CHECK_EQ(bus_read(bus, 0x0302), 0xAA);
    CHECK_EQ(bus_read(bus, 0x0303), 0xAA);

    cpu_destroy(cpu);
}

/*
 * Multiply by 4 using left shifts
 *
 *      LDA #$15        ; value = $15 (21)
 *      LDX #$02        ; shift count
 * loop: ASL A          ; A = A * 2
 *      DEX
 *      BNE loop
 *
 * Final state: A = $54 (84 = 21 * 4)
 */
TEST(test_multiply_by_shift) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    uint8_t prog[] = {
        0xA9, 0x15,     /* LDA #$15 */
        0xA2, 0x02,     /* LDX #$02 */
        0x0A,           /* ASL A */
        0xCA,           /* DEX */
        0xD0, 0xFC      /* BNE -4 (back to ASL) */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run: LDA + LDX + (ASL + DEX + BNE)*2 = 2 + 6 = 8 instructions */
    run_program(cpu, 8);

    CHECK_EQ(cpu_get_a(cpu), 0x54);  /* $15 * 4 = $54 */
    CHECK_EQ(cpu_get_x(cpu), 0x00);
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z flag set from DEX");

    cpu_destroy(cpu);
}

/* ===================== Arithmetic Tests ===================== */

/*
 * Add Two Numbers from Zero Page
 *
 *      CLC             ; clear carry
 *      LDA $10         ; load first number
 *      ADC $11         ; add second number
 *      STA $12         ; store result
 *
 * Test: $10 = $30, $11 = $25 -> $12 = $55
 */
TEST(test_add_two_numbers) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Set up operands in zero page */
    bus_write(bus, 0x0010, 0x30);
    bus_write(bus, 0x0011, 0x25);

    uint8_t prog[] = {
        0x18,           /* CLC */
        0xA5, 0x10,     /* LDA $10 */
        0x65, 0x11,     /* ADC $11 */
        0x85, 0x12      /* STA $12 */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    run_program(cpu, 4);

    CHECK_EQ(cpu_get_a(cpu), 0x55);
    CHECK_EQ(bus_read(bus, 0x0012), 0x55);
    CHECK(!(cpu_get_status(cpu) & FLAG_C), "No carry");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Not zero");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "Not negative");

    cpu_destroy(cpu);
}

/*
 * Add with Carry Out
 *
 *      CLC
 *      LDA $10         ; $FF
 *      ADC $11         ; + $02 = $101 -> A = $01, C = 1
 *      STA $12
 *
 * Test: $FF + $02 = $101, result $01 with carry
 */
TEST(test_add_with_carry) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0x0010, 0xFF);
    bus_write(bus, 0x0011, 0x02);

    uint8_t prog[] = {
        0x18,           /* CLC */
        0xA5, 0x10,     /* LDA $10 */
        0x65, 0x11,     /* ADC $11 */
        0x85, 0x12      /* STA $12 */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    run_program(cpu, 4);

    CHECK_EQ(cpu_get_a(cpu), 0x01);
    CHECK_EQ(bus_read(bus, 0x0012), 0x01);
    CHECK(cpu_get_status(cpu) & FLAG_C, "Carry should be set");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Not zero");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "Not negative");

    cpu_destroy(cpu);
}

/*
 * Signed Overflow Test
 *
 *      CLC
 *      LDA #$7F        ; +127
 *      ADC #$01        ; + 1 = -128 (overflow!)
 *
 * Final: A = $80, V = 1, N = 1
 */
TEST(test_signed_overflow) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    uint8_t prog[] = {
        0x18,           /* CLC */
        0xA9, 0x7F,     /* LDA #$7F */
        0x69, 0x01      /* ADC #$01 */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    run_program(cpu, 3);

    CHECK_EQ(cpu_get_a(cpu), 0x80);
    CHECK(cpu_get_status(cpu) & FLAG_V, "V flag should be set (overflow)");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N flag should be set (negative)");
    CHECK(!(cpu_get_status(cpu) & FLAG_C), "C flag should be clear");

    cpu_destroy(cpu);
}

/* ===================== Branch/Compare Tests ===================== */

/*
 * Compare and Branch
 *
 *          LDA #$50
 *          CMP #$50        ; A == $50?
 *          BEQ equal       ; yes, branch
 *          LDA #$00        ; should be skipped
 *          JMP done
 * equal:   LDA #$FF        ; should execute
 * done:    STA $30
 *
 * Final: A = $FF, $30 = $FF
 */
TEST(test_compare_and_branch) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    uint8_t prog[] = {
        0xA9, 0x50,         /* LDA #$50 */
        0xC9, 0x50,         /* CMP #$50 */
        0xF0, 0x05,         /* BEQ +5 (to equal) */
        0xA9, 0x00,         /* LDA #$00 (skipped) */
        0x4C, 0x0E, 0x02,   /* JMP $020E (skipped) */
        0xA9, 0xFF,         /* LDA #$FF (equal label) */
        0x85, 0x30          /* STA $30 (done label) */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    run_program(cpu, 5);  /* LDA, CMP, BEQ, LDA, STA */

    CHECK_EQ(cpu_get_a(cpu), 0xFF);
    CHECK_EQ(bus_read(bus, 0x0030), 0xFF);

    cpu_destroy(cpu);
}

/* ===================== Stack Tests ===================== */

/*
 * Push/Pull Sequence (LIFO order)
 *
 *      LDA #$11
 *      PHA             ; push $11
 *      LDA #$22
 *      PHA             ; push $22
 *      LDA #$33
 *      PHA             ; push $33
 *      PLA             ; A = $33
 *      PLA             ; A = $22
 *      PLA             ; A = $11
 *
 * Final: A = $11, SP restored
 */
TEST(test_push_pull_sequence) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    uint8_t prog[] = {
        0xA9, 0x11,     /* LDA #$11 */
        0x48,           /* PHA */
        0xA9, 0x22,     /* LDA #$22 */
        0x48,           /* PHA */
        0xA9, 0x33,     /* LDA #$33 */
        0x48,           /* PHA */
        0x68,           /* PLA -> $33 */
        0x68,           /* PLA -> $22 */
        0x68            /* PLA -> $11 */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run all 10 instructions:
     * LDA, PHA, LDA, PHA, LDA, PHA, PLA, PLA, PLA = 9 ops
     * After 7 ops: A = $33 (first PLA)
     * After 8 ops: A = $22 (second PLA)
     * After 9 ops: A = $11 (third PLA)
     */
    run_program(cpu, 7);  /* up to first PLA */
    CHECK_EQ(cpu_get_a(cpu), 0x33);

    run_program(cpu, 1);  /* second PLA */
    CHECK_EQ(cpu_get_a(cpu), 0x22);

    run_program(cpu, 1);  /* third PLA */
    CHECK_EQ(cpu_get_a(cpu), 0x11);
    CHECK_EQ(cpu_get_sp(cpu), initial_sp);

    cpu_destroy(cpu);
}

/*
 * Subroutine Call and Return
 *
 *          LDA #$00        ; $0200
 *          JSR sub         ; $0202 -> $020B
 *          STA $20         ; $0205 (store result)
 *          JMP done        ; $0207
 * sub:     LDA #$42        ; $020A
 *          RTS             ; $020C
 * done:    NOP             ; $020D
 *
 * Final: A = $42, $20 = $42
 */
TEST(test_subroutine_call) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    uint8_t prog[] = {
        0xA9, 0x00,         /* $0200: LDA #$00 */
        0x20, 0x0A, 0x02,   /* $0202: JSR $020A */
        0x85, 0x20,         /* $0205: STA $20 */
        0x4C, 0x0D, 0x02,   /* $0207: JMP $020D */
        0xA9, 0x42,         /* $020A: LDA #$42 (sub) */
        0x60,               /* $020C: RTS */
        0xEA                /* $020D: NOP (done) */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    run_program(cpu, 6);  /* LDA, JSR, LDA, RTS, STA, JMP */

    CHECK_EQ(cpu_get_a(cpu), 0x42);
    CHECK_EQ(bus_read(bus, 0x0020), 0x42);
    CHECK_EQ(cpu_get_sp(cpu), initial_sp);
    check_pc(cpu, 0x020D);

    cpu_destroy(cpu);
}

/*
 * Flag Preservation with PHP/PLP
 *
 *      SEC             ; C = 1
 *      PHP             ; save flags
 *      CLC             ; C = 0
 *      PLP             ; restore flags
 *      ; C should be 1 again
 *
 * Tests that PLP correctly restores flags
 */
TEST(test_flag_preservation) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    uint8_t prog[] = {
        0x38,           /* SEC (C = 1) */
        0x08,           /* PHP */
        0x18,           /* CLC (C = 0) */
        0x28            /* PLP */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    run_program(cpu, 2);  /* SEC, PHP */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set after SEC");

    run_program(cpu, 1);  /* CLC */
    CHECK(!(cpu_get_status(cpu) & FLAG_C), "C should be clear after CLC");

    run_program(cpu, 1);  /* PLP */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be restored by PLP");

    cpu_destroy(cpu);
}

/* ===================== Memory Tests ===================== */

/*
 * Indirect Indexed Copy
 *
 * Copy 4 bytes from $0300 to $0400 using (indirect),Y addressing
 *
 *          LDY #$03
 * loop:    LDA ($10),Y     ; ptr at $10 -> $0300
 *          STA ($12),Y     ; ptr at $12 -> $0400
 *          DEY
 *          BPL loop
 *
 * Pointers: $10-$11 = $0300, $12-$13 = $0400
 */
TEST(test_indirect_indexed_copy) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Set up pointers in zero page (little-endian) */
    bus_write(bus, 0x0010, 0x00);  /* src ptr low */
    bus_write(bus, 0x0011, 0x03);  /* src ptr high -> $0300 */
    bus_write(bus, 0x0012, 0x00);  /* dst ptr low */
    bus_write(bus, 0x0013, 0x04);  /* dst ptr high -> $0400 */

    /* Set up source data */
    bus_write(bus, 0x0300, 0xDE);
    bus_write(bus, 0x0301, 0xAD);
    bus_write(bus, 0x0302, 0xBE);
    bus_write(bus, 0x0303, 0xEF);

    uint8_t prog[] = {
        0xA0, 0x03,     /* LDY #$03 */
        0xB1, 0x10,     /* LDA ($10),Y */
        0x91, 0x12,     /* STA ($12),Y */
        0x88,           /* DEY */
        0x10, 0xF9      /* BPL -7 (back to LDA) */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run: LDY + (LDA + STA + DEY + BPL)*4 = 1 + 16 = 17 instructions */
    run_program(cpu, 17);

    CHECK_EQ(bus_read(bus, 0x0400), 0xDE);
    CHECK_EQ(bus_read(bus, 0x0401), 0xAD);
    CHECK_EQ(bus_read(bus, 0x0402), 0xBE);
    CHECK_EQ(bus_read(bus, 0x0403), 0xEF);
    CHECK_EQ(cpu_get_y(cpu), 0xFF);  /* Wrapped to $FF after DEY from 0 */
    CHECK(cpu_get_status(cpu) & FLAG_N, "N set (Y = $FF)");

    cpu_destroy(cpu);
}

/* ===================== Stress Tests ===================== */

/*
 * Bubble Sort - Sort 8 bytes in ascending order
 *
 * This is a ~60 instruction program that exercises:
 * - Nested loops
 * - Indexed addressing (ZPG,X)
 * - Compare and branch
 * - Memory read/write
 * - Register transfers
 *
 * Data at $50-$57 (8 bytes to sort)
 * Uses $40 as swap flag, $41 as outer loop counter
 *
 *          LDA #$07        ; outer loop: 7 passes max
 *          STA $41
 * outer:   LDA #$00        ; clear swap flag
 *          STA $40
 *          LDX #$00        ; inner loop index
 * inner:   LDA $50,X       ; load current element
 *          CMP $51,X       ; compare with next
 *          BCC noswap      ; if current < next, no swap
 *          BEQ noswap      ; if equal, no swap
 *          ; swap elements
 *          TAY             ; save current in Y
 *          LDA $51,X       ; load next
 *          STA $50,X       ; store in current position
 *          TYA             ; restore current from Y
 *          STA $51,X       ; store in next position
 *          LDA #$01        ; set swap flag
 *          STA $40
 * noswap:  INX
 *          CPX #$07        ; done with inner loop?
 *          BNE inner
 *          ; check if any swaps occurred
 *          LDA $40
 *          BEQ done        ; no swaps = sorted
 *          DEC $41
 *          BNE outer       ; continue outer loop
 * done:    BRK
 */
TEST(test_stress_bubble_sort) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Unsorted data: 64, 25, 12, 22, 11, 90, 42, 8 */
    bus_write(bus, 0x0050, 64);
    bus_write(bus, 0x0051, 25);
    bus_write(bus, 0x0052, 12);
    bus_write(bus, 0x0053, 22);
    bus_write(bus, 0x0054, 11);
    bus_write(bus, 0x0055, 90);
    bus_write(bus, 0x0056, 42);
    bus_write(bus, 0x0057, 8);

    uint8_t prog[] = {
        /* $0200 */ 0xA9, 0x07,         /* LDA #$07 */
        /* $0202 */ 0x85, 0x41,         /* STA $41 */
        /* outer: $0204 */
        /* $0204 */ 0xA9, 0x00,         /* LDA #$00 */
        /* $0206 */ 0x85, 0x40,         /* STA $40 */
        /* $0208 */ 0xA2, 0x00,         /* LDX #$00 */
        /* inner: $020A */
        /* $020A */ 0xB5, 0x50,         /* LDA $50,X */
        /* $020C */ 0xD5, 0x51,         /* CMP $51,X */
        /* $020E */ 0x90, 0x0E,         /* BCC noswap (+14 -> $021E) */
        /* $0210 */ 0xF0, 0x0C,         /* BEQ noswap (+12 -> $021E) */
        /* swap: */
        /* $0212 */ 0xA8,               /* TAY */
        /* $0213 */ 0xB5, 0x51,         /* LDA $51,X */
        /* $0215 */ 0x95, 0x50,         /* STA $50,X */
        /* $0217 */ 0x98,               /* TYA */
        /* $0218 */ 0x95, 0x51,         /* STA $51,X */
        /* $021A */ 0xA9, 0x01,         /* LDA #$01 */
        /* $021C */ 0x85, 0x40,         /* STA $40 */
        /* noswap: $021E */
        /* $021E */ 0xE8,               /* INX */
        /* $021F */ 0xE0, 0x07,         /* CPX #$07 */
        /* $0221 */ 0xD0, 0xE7,         /* BNE inner (-25 -> $020A) */
        /* check swaps */
        /* $0223 */ 0xA5, 0x40,         /* LDA $40 */
        /* $0225 */ 0xF0, 0x06,         /* BEQ done (+6 -> $022D) */
        /* $0227 */ 0xC6, 0x41,         /* DEC $41 */
        /* $0229 */ 0xD0, 0xD9,         /* BNE outer (-39 -> $0204) */
        /* done: $022B or $022D depending on path */
        /* $022B */ 0x00,               /* BRK (fallthrough) */
        /* $022C */ 0x00,               /* BRK (padding) */
        /* $022D */ 0x00                /* BRK (from BEQ done) */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run until BRK (max 2000 instructions for safety) */
    int instructions = 0;
    while (instructions < 2000) {
        uint8_t opcode = bus_read(bus, cpu_get_pc(cpu));
        if (opcode == 0x00) break;  /* BRK */
        cpu_step(cpu);
        instructions++;
    }

    /* Verify sorted: 8, 11, 12, 22, 25, 42, 64, 90 */
    CHECK_EQ(bus_read(bus, 0x0050), 8);
    CHECK_EQ(bus_read(bus, 0x0051), 11);
    CHECK_EQ(bus_read(bus, 0x0052), 12);
    CHECK_EQ(bus_read(bus, 0x0053), 22);
    CHECK_EQ(bus_read(bus, 0x0054), 25);
    CHECK_EQ(bus_read(bus, 0x0055), 42);
    CHECK_EQ(bus_read(bus, 0x0056), 64);
    CHECK_EQ(bus_read(bus, 0x0057), 90);

    /* Verify reasonable instruction count (bubble sort ~300-500 for this data) */
    CHECK(instructions > 100, "Should execute many instructions");
    CHECK(instructions < 1000, "Should not run forever");

    cpu_destroy(cpu);
}

/*
 * Fibonacci Sequence Generator
 *
 * Generates first 12 Fibonacci numbers: 1,1,2,3,5,8,13,21,34,55,89,144
 * Results stored at $60-$6B
 *
 * Tests: loops, ADC, memory read/write, indexed addressing
 *
 *          LDA #$01        ; fib[0] = 1
 *          STA $60
 *          STA $61         ; fib[1] = 1
 *          LDX #$02        ; start at index 2
 * loop:    LDA $5E,X       ; load fib[n-2]
 *          CLC
 *          ADC $5F,X       ; add fib[n-1]
 *          STA $60,X       ; store fib[n]
 *          INX
 *          CPX #$0C        ; 12 numbers total
 *          BNE loop
 *          BRK
 */
TEST(test_stress_fibonacci) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    uint8_t prog[] = {
        /* $0200 */ 0xA9, 0x01,         /* LDA #$01 */
        /* $0202 */ 0x85, 0x60,         /* STA $60 */
        /* $0204 */ 0x85, 0x61,         /* STA $61 */
        /* $0206 */ 0xA2, 0x02,         /* LDX #$02 */
        /* loop: $0208 */
        /* $0208 */ 0xB5, 0x5E,         /* LDA $5E,X (fib[n-2]) */
        /* $020A */ 0x18,               /* CLC */
        /* $020B */ 0x75, 0x5F,         /* ADC $5F,X (fib[n-1]) */
        /* $020D */ 0x95, 0x60,         /* STA $60,X (fib[n]) */
        /* $020F */ 0xE8,               /* INX */
        /* $0210 */ 0xE0, 0x0C,         /* CPX #$0C */
        /* $0212 */ 0xD0, 0xF4,         /* BNE loop (-12 -> $0208) */
        /* $0214 */ 0x00                /* BRK */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run until BRK */
    while (bus_read(bus, cpu_get_pc(cpu)) != 0x00) {
        cpu_step(cpu);
    }

    /* Verify Fibonacci sequence: 1,1,2,3,5,8,13,21,34,55,89,144 */
    CHECK_EQ(bus_read(bus, 0x0060), 1);
    CHECK_EQ(bus_read(bus, 0x0061), 1);
    CHECK_EQ(bus_read(bus, 0x0062), 2);
    CHECK_EQ(bus_read(bus, 0x0063), 3);
    CHECK_EQ(bus_read(bus, 0x0064), 5);
    CHECK_EQ(bus_read(bus, 0x0065), 8);
    CHECK_EQ(bus_read(bus, 0x0066), 13);
    CHECK_EQ(bus_read(bus, 0x0067), 21);
    CHECK_EQ(bus_read(bus, 0x0068), 34);
    CHECK_EQ(bus_read(bus, 0x0069), 55);
    CHECK_EQ(bus_read(bus, 0x006A), 89);
    CHECK_EQ(bus_read(bus, 0x006B), 144);

    cpu_destroy(cpu);
}

/*
 * Nested Subroutine Stress Test
 *
 * Tests deep stack usage with 4 levels of nested JSR/RTS.
 * Each subroutine pushes a value, calls the next, then pops and stores.
 *
 *          JSR sub1        ; call level 1
 *          BRK
 * sub1:    LDA #$11
 *          PHA
 *          JSR sub2
 *          PLA
 *          STA $70         ; store $11
 *          RTS
 * sub2:    LDA #$22
 *          PHA
 *          JSR sub3
 *          PLA
 *          STA $71         ; store $22
 *          RTS
 * sub3:    LDA #$33
 *          PHA
 *          JSR sub4
 *          PLA
 *          STA $72         ; store $33
 *          RTS
 * sub4:    LDA #$44
 *          STA $73         ; store $44 (deepest level)
 *          RTS
 */
TEST(test_stress_nested_calls) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    uint8_t prog[] = {
        /* $0200 */ 0x20, 0x05, 0x02,   /* JSR sub1 ($0205) */
        /* $0203 */ 0x00,               /* BRK */
        /* $0204 */ 0x00,               /* (padding) */
        /* sub1: $0205 */
        /* $0205 */ 0xA9, 0x11,         /* LDA #$11 */
        /* $0207 */ 0x48,               /* PHA */
        /* $0208 */ 0x20, 0x12, 0x02,   /* JSR sub2 ($0212) */
        /* $020B */ 0x68,               /* PLA */
        /* $020C */ 0x85, 0x70,         /* STA $70 */
        /* $020E */ 0x60,               /* RTS */
        /* $020F */ 0x00, 0x00, 0x00,   /* (padding) */
        /* sub2: $0212 */
        /* $0212 */ 0xA9, 0x22,         /* LDA #$22 */
        /* $0214 */ 0x48,               /* PHA */
        /* $0215 */ 0x20, 0x1F, 0x02,   /* JSR sub3 ($021F) */
        /* $0218 */ 0x68,               /* PLA */
        /* $0219 */ 0x85, 0x71,         /* STA $71 */
        /* $021B */ 0x60,               /* RTS */
        /* $021C */ 0x00, 0x00, 0x00,   /* (padding) */
        /* sub3: $021F */
        /* $021F */ 0xA9, 0x33,         /* LDA #$33 */
        /* $0221 */ 0x48,               /* PHA */
        /* $0222 */ 0x20, 0x2C, 0x02,   /* JSR sub4 ($022C) */
        /* $0225 */ 0x68,               /* PLA */
        /* $0226 */ 0x85, 0x72,         /* STA $72 */
        /* $0228 */ 0x60,               /* RTS */
        /* $0229 */ 0x00, 0x00, 0x00,   /* (padding) */
        /* sub4: $022C */
        /* $022C */ 0xA9, 0x44,         /* LDA #$44 */
        /* $022E */ 0x85, 0x73,         /* STA $73 */
        /* $0230 */ 0x60                /* RTS */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run until BRK */
    int instructions = 0;
    while (bus_read(bus, cpu_get_pc(cpu)) != 0x00 && instructions < 100) {
        cpu_step(cpu);
        instructions++;
    }

    /* Verify all subroutines executed and returned correctly */
    CHECK_EQ(bus_read(bus, 0x0070), 0x11);
    CHECK_EQ(bus_read(bus, 0x0071), 0x22);
    CHECK_EQ(bus_read(bus, 0x0072), 0x33);
    CHECK_EQ(bus_read(bus, 0x0073), 0x44);

    /* Stack should be restored */
    CHECK_EQ(cpu_get_sp(cpu), initial_sp);

    /* Should have ended at BRK at $0203 */
    CHECK_EQ(cpu_get_pc(cpu), 0x0203);

    cpu_destroy(cpu);
}

/*
 * Memory Block Compare
 *
 * Compares two 16-byte blocks and counts differences.
 * Result stored in $80.
 *
 * Block 1: $0300-$030F
 * Block 2: $0310-$031F
 *
 *          LDA #$00        ; difference count = 0
 *          STA $80
 *          LDX #$0F        ; 16 bytes (index 15 down to 0)
 * loop:    LDA $0300,X
 *          CMP $0310,X
 *          BEQ same
 *          INC $80         ; count difference
 * same:    DEX
 *          BPL loop
 *          BRK
 */
TEST(test_stress_memcmp) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Block 1: 0-15 */
    for (int i = 0; i < 16; i++) {
        bus_write(bus, 0x0300 + i, i);
    }

    /* Block 2: same as block 1, except positions 3, 7, 11 are different */
    for (int i = 0; i < 16; i++) {
        bus_write(bus, 0x0310 + i, i);
    }
    bus_write(bus, 0x0313, 0xFF);  /* difference at index 3 */
    bus_write(bus, 0x0317, 0xFF);  /* difference at index 7 */
    bus_write(bus, 0x031B, 0xFF);  /* difference at index 11 */

    uint8_t prog[] = {
        /* $0200 */ 0xA9, 0x00,             /* LDA #$00 */
        /* $0202 */ 0x85, 0x80,             /* STA $80 */
        /* $0204 */ 0xA2, 0x0F,             /* LDX #$0F */
        /* loop: $0206 */
        /* $0206 */ 0xBD, 0x00, 0x03,       /* LDA $0300,X */
        /* $0209 */ 0xDD, 0x10, 0x03,       /* CMP $0310,X */
        /* $020C */ 0xF0, 0x02,             /* BEQ same (+2 -> $0210) */
        /* $020E */ 0xE6, 0x80,             /* INC $80 */
        /* same: $0210 */
        /* $0210 */ 0xCA,                   /* DEX */
        /* $0211 */ 0x10, 0xF3,             /* BPL loop (-13 -> $0206) */
        /* $0213 */ 0x00                    /* BRK */
    };
    bus_load(bus, 0x0200, prog, sizeof(prog));

    /* Run until BRK */
    while (bus_read(bus, cpu_get_pc(cpu)) != 0x00) {
        cpu_step(cpu);
    }

    /* Should have found 3 differences */
    CHECK_EQ(bus_read(bus, 0x0080), 3);
    CHECK_EQ(cpu_get_x(cpu), 0xFF);  /* X wrapped to $FF */

    cpu_destroy(cpu);
}

/* ===================== Main ===================== */

int main(void) {
    reset_test_state();
    printf("\n=== Integration Tests (Small Programs) ===\n\n");

    printf("--- Loop Tests ---\n");
    RUN_TEST(test_counter_loop);
    RUN_TEST(test_memory_fill);
    RUN_TEST(test_multiply_by_shift);

    printf("\n--- Arithmetic Tests ---\n");
    RUN_TEST(test_add_two_numbers);
    RUN_TEST(test_add_with_carry);
    RUN_TEST(test_signed_overflow);

    printf("\n--- Branch/Compare Tests ---\n");
    RUN_TEST(test_compare_and_branch);

    printf("\n--- Stack Tests ---\n");
    RUN_TEST(test_push_pull_sequence);
    RUN_TEST(test_subroutine_call);
    RUN_TEST(test_flag_preservation);

    printf("\n--- Memory Tests ---\n");
    RUN_TEST(test_indirect_indexed_copy);

    printf("\n--- Stress Tests ---\n");
    RUN_TEST(test_stress_bubble_sort);
    RUN_TEST(test_stress_fibonacci);
    RUN_TEST(test_stress_nested_calls);
    RUN_TEST(test_stress_memcmp);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
