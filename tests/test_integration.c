#include "test_common.h"
#include "memory.h"

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

/* Run until PC reaches target or max instructions */
static uint16_t run_until_pc(CPU* cpu, uint16_t target_pc, int max) {
    uint16_t cycles = 0;
    for (int i = 0; i < max && cpu_get_pc(cpu) != target_pc; i++) {
        cycles += cpu_step(cpu);
    }
    return cycles;
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
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0xA2, 0x05,     /* LDX #$05 */
        0xCA,           /* DEX */
        0xD0, 0xFD      /* BNE -3 (back to DEX) */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

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
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0xA2, 0x04,         /* LDX #$04 */
        0xA9, 0xAA,         /* LDA #$AA */
        0xCA,               /* DEX */
        0x9D, 0x00, 0x03,   /* STA $0300,X */
        0xD0, 0xFA          /* BNE -6 (back to DEX) */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

    /* Run: LDX + LDA + (DEX + STA + BNE)*4 = 2 + 12 = 14 instructions */
    run_program(cpu, 14);

    CHECK_EQ(cpu_get_x(cpu), 0x00);
    CHECK_EQ(cpu_get_a(cpu), 0xAA);
    CHECK_EQ(memory_read(mem, 0x0300), 0xAA);
    CHECK_EQ(memory_read(mem, 0x0301), 0xAA);
    CHECK_EQ(memory_read(mem, 0x0302), 0xAA);
    CHECK_EQ(memory_read(mem, 0x0303), 0xAA);

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
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0xA9, 0x15,     /* LDA #$15 */
        0xA2, 0x02,     /* LDX #$02 */
        0x0A,           /* ASL A */
        0xCA,           /* DEX */
        0xD0, 0xFC      /* BNE -4 (back to ASL) */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

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
    Memory* mem = cpu_get_memory(cpu);

    /* Set up operands in zero page */
    memory_write(mem, 0x0010, 0x30);
    memory_write(mem, 0x0011, 0x25);

    uint8_t prog[] = {
        0x18,           /* CLC */
        0xA5, 0x10,     /* LDA $10 */
        0x65, 0x11,     /* ADC $11 */
        0x85, 0x12      /* STA $12 */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

    run_program(cpu, 4);

    CHECK_EQ(cpu_get_a(cpu), 0x55);
    CHECK_EQ(memory_read(mem, 0x0012), 0x55);
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
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0010, 0xFF);
    memory_write(mem, 0x0011, 0x02);

    uint8_t prog[] = {
        0x18,           /* CLC */
        0xA5, 0x10,     /* LDA $10 */
        0x65, 0x11,     /* ADC $11 */
        0x85, 0x12      /* STA $12 */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

    run_program(cpu, 4);

    CHECK_EQ(cpu_get_a(cpu), 0x01);
    CHECK_EQ(memory_read(mem, 0x0012), 0x01);
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
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0x18,           /* CLC */
        0xA9, 0x7F,     /* LDA #$7F */
        0x69, 0x01      /* ADC #$01 */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

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
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0xA9, 0x50,         /* LDA #$50 */
        0xC9, 0x50,         /* CMP #$50 */
        0xF0, 0x05,         /* BEQ +5 (to equal) */
        0xA9, 0x00,         /* LDA #$00 (skipped) */
        0x4C, 0x0E, 0x02,   /* JMP $020E (skipped) */
        0xA9, 0xFF,         /* LDA #$FF (equal label) */
        0x85, 0x30          /* STA $30 (done label) */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

    run_program(cpu, 5);  /* LDA, CMP, BEQ, LDA, STA */

    CHECK_EQ(cpu_get_a(cpu), 0xFF);
    CHECK_EQ(memory_read(mem, 0x0030), 0xFF);

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
    Memory* mem = cpu_get_memory(cpu);
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
    memory_load(mem, 0x0200, prog, sizeof(prog));

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
    Memory* mem = cpu_get_memory(cpu);
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
    memory_load(mem, 0x0200, prog, sizeof(prog));

    run_program(cpu, 6);  /* LDA, JSR, LDA, RTS, STA, JMP */

    CHECK_EQ(cpu_get_a(cpu), 0x42);
    CHECK_EQ(memory_read(mem, 0x0020), 0x42);
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
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0x38,           /* SEC (C = 1) */
        0x08,           /* PHP */
        0x18,           /* CLC (C = 0) */
        0x28            /* PLP */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

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
    Memory* mem = cpu_get_memory(cpu);

    /* Set up pointers in zero page (little-endian) */
    memory_write(mem, 0x0010, 0x00);  /* src ptr low */
    memory_write(mem, 0x0011, 0x03);  /* src ptr high -> $0300 */
    memory_write(mem, 0x0012, 0x00);  /* dst ptr low */
    memory_write(mem, 0x0013, 0x04);  /* dst ptr high -> $0400 */

    /* Set up source data */
    memory_write(mem, 0x0300, 0xDE);
    memory_write(mem, 0x0301, 0xAD);
    memory_write(mem, 0x0302, 0xBE);
    memory_write(mem, 0x0303, 0xEF);

    uint8_t prog[] = {
        0xA0, 0x03,     /* LDY #$03 */
        0xB1, 0x10,     /* LDA ($10),Y */
        0x91, 0x12,     /* STA ($12),Y */
        0x88,           /* DEY */
        0x10, 0xF9      /* BPL -7 (back to LDA) */
    };
    memory_load(mem, 0x0200, prog, sizeof(prog));

    /* Run: LDY + (LDA + STA + DEY + BPL)*4 = 1 + 16 = 17 instructions */
    run_program(cpu, 17);

    CHECK_EQ(memory_read(mem, 0x0400), 0xDE);
    CHECK_EQ(memory_read(mem, 0x0401), 0xAD);
    CHECK_EQ(memory_read(mem, 0x0402), 0xBE);
    CHECK_EQ(memory_read(mem, 0x0403), 0xEF);
    CHECK_EQ(cpu_get_y(cpu), 0xFF);  /* Wrapped to $FF after DEY from 0 */
    CHECK(cpu_get_status(cpu) & FLAG_N, "N set (Y = $FF)");

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

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
