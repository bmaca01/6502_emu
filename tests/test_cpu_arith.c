/*
 * Arithmetic CPU tests for 6502 emulator
 * Tests: ADC, SBC, INC, DEC, INX, INY, DEX, DEY, CMP, CPX, CPY
 */

#include "test_common.h"
#include "memory.h"
#include "opcodes.h"
#include "addressing.h"

/* ============================ ADC Tests ==================================== */

TEST(test_add) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x70);
    memory_write(mem, 0x0200, encode_op(ADC, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 0);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_a(cpu) == 0x71);
    cpu_destroy(cpu);
}

TEST(test_add_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0xFF);
    memory_write(mem, 0x0200, encode_op(ADC, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 1);
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_a(cpu) == 0x00);
    cpu_destroy(cpu);
}

TEST(test_add_of) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x7F);
    memory_write(mem, 0x0200, encode_op(ADC, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 1, 0);
    CHECK(cpu_get_status(cpu) & FLAG_V);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_a(cpu) == 0x80);
    cpu_destroy(cpu);
}

TEST(test_add_with_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x70);
    cpu_set_status(cpu, (cpu_get_status(cpu) | FLAG_C));

    memory_write(mem, 0x0200, encode_op(ADC, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 0);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_a(cpu) == 0x72);
    cpu_destroy(cpu);
}

TEST(test_add_with_carry_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0xFE);
    cpu_set_status(cpu, (cpu_get_status(cpu) | FLAG_C));

    memory_write(mem, 0x0200, encode_op(ADC, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 1);
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_a(cpu) == 0x00);
    cpu_destroy(cpu);
}

TEST(test_add_with_carry_of) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0xA0);
    cpu_set_status(cpu, (cpu_get_status(cpu) | FLAG_C));

    memory_write(mem, 0x0200, encode_op(ADC, IMM));
    memory_write(mem, 0x0201, 0x9F);

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 0);
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cpu_get_status(cpu) & FLAG_V);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_a(cpu) == 0x40);
    cpu_destroy(cpu);
}

/* ============================ SBC Tests ==================================== */

TEST(test_sbc) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0xa9, 0x80, 0xe9, 0x80
    };
    memory_load(mem, 0x0200, prog, 4);

    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);

    cpu_step(cpu);
    CHECK(cpu_get_status(cpu) & FLAG_N);

    uint8_t cs = cpu_step(cpu);

    CHECK(cs == 2);
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cpu_get_status(cpu) & FLAG_Z);
    CHECK(cpu_get_a(cpu) == 0);

    cpu_destroy(cpu);
}

TEST(test_sbc_no_carry_set) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        0xa9, 0x80, 0xe9, 0x80
    };
    memory_load(mem, 0x0200, prog, 4);

    cpu_step(cpu);
    CHECK(cpu_get_status(cpu) & FLAG_N);

    uint8_t cs = cpu_step(cpu);
    CHECK(cpu_get_status(cpu) & FLAG_N);

    CHECK(cs == 2);
    CHECK(cpu_get_a(cpu) == 0xff);

    cpu_destroy(cpu);
}

/* SBC: 0x50 - 0x10 = 0x40 (with carry set, no borrow) */
TEST(test_sbc_simple) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);
    memory_write(mem, 0x0200, encode_op(SBC, IMM));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x40);
    check_flags(cpu, 0, 0);
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* No borrow */
    CHECK(!(cpu_get_status(cpu) & FLAG_V));
    CHECK(cycles == 2);
    cpu_destroy(cpu);
}

/* SBC: 0x50 - 0x60 = 0xF0 (borrow occurs, C cleared) */
TEST(test_sbc_borrow) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);
    memory_write(mem, 0x0200, encode_op(SBC, IMM));
    memory_write(mem, 0x0201, 0x60);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xF0);
    check_flags(cpu, 1, 0);  /* Negative result */
    CHECK(!(cpu_get_status(cpu) & FLAG_C));  /* Borrow occurred */
    CHECK(!(cpu_get_status(cpu) & FLAG_V));
    CHECK(cycles == 2);
    cpu_destroy(cpu);
}

/* SBC overflow: 0x50 - 0xB0 = 0xA0, but signed: 80 - (-80) = 160 (overflow!) */
TEST(test_sbc_overflow_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);  /* +80 signed */
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);
    memory_write(mem, 0x0200, encode_op(SBC, IMM));
    memory_write(mem, 0x0201, 0xB0);  /* -80 signed */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xA0);  /* Result looks negative but should be +160 */
    check_flags(cpu, 1, 0);
    CHECK(cpu_get_status(cpu) & FLAG_V);  /* Signed overflow */
    CHECK(cycles == 2);
    cpu_destroy(cpu);
}

/* SBC overflow: 0x80 - 0x01 = 0x7F, but signed: -128 - 1 = -129 (overflow!) */
TEST(test_sbc_overflow_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x80);  /* -128 signed */
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);
    memory_write(mem, 0x0200, encode_op(SBC, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x7F);  /* +127, but should be -129 */
    check_flags(cpu, 0, 0);
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cpu_get_status(cpu) & FLAG_V);  /* Signed overflow */
    CHECK(cycles == 2);
    cpu_destroy(cpu);
}

/* SBC with borrow in: 0x50 - 0x10 - 1 = 0x3F (carry clear = borrow) */
TEST(test_sbc_with_borrow_in) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* Clear carry = borrow in */
    memory_write(mem, 0x0200, encode_op(SBC, IMM));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x3F);  /* 0x50 - 0x10 - 1 */
    check_flags(cpu, 0, 0);
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* No new borrow */
    CHECK(cycles == 2);
    cpu_destroy(cpu);
}

/* SBC zero page addressing */
TEST(test_sbc_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x30);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);
    memory_write(mem, 0x0010, 0x10);  /* Value at ZP $10 */
    memory_write(mem, 0x0200, encode_op(SBC, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x20);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* SBC absolute addressing */
TEST(test_sbc_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x40);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);
    memory_write(mem, 0x1234, 0x15);  /* Value at $1234 */
    memory_write(mem, 0x0200, encode_op(SBC, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x2B);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* SBC absolute,X with page crossing */
TEST(test_sbc_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_x(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);
    memory_write(mem, 0x1300, 0x10);  /* Value at $12FF + 1 */
    memory_write(mem, 0x0200, encode_op(SBC, ABS_X));
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x40);
    CHECK(cycles == 5);  /* +1 for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ INC/DEC Tests ================================ */

TEST(test_inc_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x000A, 0x7F);
    memory_write(mem, 0x0200, encode_op(INC, ZPG));
    memory_write(mem, 0x0201,                0x0A);

    uint8_t inc_cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x000A) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(inc_cycles == 5);
    cpu_destroy(cpu);
}

TEST(test_inc_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x0A);
    memory_write(mem, 0x000A, 0x7F);
    memory_write(mem, 0x0200, encode_op(INC, ZPG_X));
    memory_write(mem, 0x0201,                0x00);

    uint8_t inc_cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x000A) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(inc_cycles == 6);
    cpu_destroy(cpu);
}

TEST(test_inc_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x100A, 0x7F);
    memory_write(mem, 0x0200, encode_op(INC, ABS));
    memory_write(mem, 0x0201, 0x0A);
    memory_write(mem, 0x0202, 0x10);

    uint8_t inc_cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x100A) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(inc_cycles == 6);
    cpu_destroy(cpu);
}

TEST(test_inc_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    uint8_t prog[] = {
        encode_op(INC, ABS_X), 0x00, 0x10
    };

    cpu_set_x(cpu, 0x0A);
    memory_write(mem, 0x100A, 0x7F);
    memory_load(mem, 0x0200, prog, 3);

    uint8_t inc_cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x100A) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(inc_cycles == 7);
    cpu_destroy(cpu);
}

TEST(test_dec_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x000A, 0x01);
    memory_write(mem, 0x0200, encode_op(DEC, ZPG));
    memory_write(mem, 0x0201,                0x0A);

    uint8_t inc_cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x000A) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(inc_cycles == 5);
    cpu_destroy(cpu);
}

/* ============================ INX/INY/DEX/DEY Tests ======================== */

TEST(test_inx) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0xFF);
    memory_write(mem, 0x0200, encode_op(INX, IMPL));

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 1);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_x(cpu) == 0x00);
    cpu_destroy(cpu);
}

TEST(test_iny) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x00);
    memory_write(mem, 0x0200, encode_op(INY, IMPL));

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 0);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_y(cpu) == 0x01);
    cpu_destroy(cpu);
}

TEST(test_dex) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x80);
    memory_write(mem, 0x0200, encode_op(DEX, IMPL));

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 0, 0);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_x(cpu) == 0x7f);
    cpu_destroy(cpu);
}

TEST(test_dey) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x00);
    memory_write(mem, 0x0200, encode_op(DEY, IMPL));

    uint8_t inc_cycles = cpu_step(cpu);

    check_flags(cpu, 1, 0);
    CHECK(inc_cycles == 2);
    CHECK(cpu_get_y(cpu) == 0xFF);
    cpu_destroy(cpu);
}

/* ========================== CMP Tests ====================================== */

TEST(test_cmp_equal) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IMM));
    memory_write(mem, 0x0201, 0x50);

    uint8_t cycles = cpu_step(cpu);

    /* A == M: C=1, Z=1, N=0 */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A == M");
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z should be set when A == M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when A == M");
    CHECK_EQ(cpu_get_a(cpu), 0x50);  /* A unchanged */
    CHECK(cycles == 2, "CMP IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_greater) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IMM));
    memory_write(mem, 0x0201, 0x30);

    uint8_t cycles = cpu_step(cpu);

    /* A > M: C=1, Z=0, N=0 (result=0x20) */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when A > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when result is positive");
    CHECK_EQ(cpu_get_a(cpu), 0x50);
    CHECK(cycles == 2, "CMP IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_less) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x30);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IMM));
    memory_write(mem, 0x0201, 0x50);

    uint8_t cycles = cpu_step(cpu);

    /* A < M: C=0, Z=0, N=1 (result=0xE0) */
    CHECK(!(cpu_get_status(cpu) & FLAG_C), "C should be clear when A < M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when A < M");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N should be set when result bit 7 is set");
    CHECK_EQ(cpu_get_a(cpu), 0x30);
    CHECK(cycles == 2, "CMP IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_greater_negative_result) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x80);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t cycles = cpu_step(cpu);

    /* A > M: C=1, Z=0, N=0 (result=0x7F, bit 7 clear) */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when A != M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when result=0x7F");
    CHECK_EQ(cpu_get_a(cpu), 0x80);
    CHECK(cycles == 2, "CMP IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_less_positive_result) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IMM));
    memory_write(mem, 0x0201, 0x02);

    uint8_t cycles = cpu_step(cpu);

    /* A < M: C=0, Z=0, N=1 (result=0xFF, bit 7 set) */
    CHECK(!(cpu_get_status(cpu) & FLAG_C), "C should be clear when A < M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when A != M");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N should be set when result=0xFF");
    CHECK_EQ(cpu_get_a(cpu), 0x01);
    CHECK(cycles == 2, "CMP IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_zero_vs_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x00);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IMM));
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    /* A == M: C=1, Z=1, N=0 */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A == M");
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z should be set when A == M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when result is 0");
    CHECK_EQ(cpu_get_a(cpu), 0x00);
    CHECK(cycles == 2, "CMP IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_imm) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IMM));
    memory_write(mem, 0x0201, 0x30);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 2, "CMP IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, ZPG));
    memory_write(mem, 0x0201, 0x42);
    memory_write(mem, 0x0042, 0x30);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when A != M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when result is positive");
    CHECK(cycles == 3, "CMP ZPG takes 3 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_x(cpu, 0x05);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, ZPG_X));
    memory_write(mem, 0x0201, 0x40);
    memory_write(mem, 0x0045, 0x30);  /* $40 + $05 = $45 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 4, "CMP ZPG,X takes 4 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x30);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 4, "CMP ABS takes 4 cycles");
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_cmp_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_x(cpu, 0x04);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, ABS_X));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x30);  /* $1230 + $04 = $1234 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 4, "CMP ABS,X takes 4 cycles without page cross");
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_cmp_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_x(cpu, 0xFF);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, ABS_X));
    memory_write(mem, 0x0201, 0x20);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x131F, 0x30);  /* $1220 + $FF = $131F, page cross */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 5, "CMP ABS,X takes 5 cycles with page cross");
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_cmp_abs_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_y(cpu, 0x04);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, ABS_Y));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x30);  /* $1230 + $04 = $1234 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 4, "CMP ABS,Y takes 4 cycles without page cross");
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_cmp_ind_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_x(cpu, 0x04);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IDX_IND));
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0014, 0x34);  /* Low byte at $10+$04 */
    memory_write(mem, 0x0015, 0x12);  /* High byte */
    memory_write(mem, 0x1234, 0x30);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 6, "CMP (ZPG,X) takes 6 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_ind_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_y(cpu, 0x04);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IND_IDX));
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x30);  /* Low byte of base */
    memory_write(mem, 0x0011, 0x12);  /* High byte */
    memory_write(mem, 0x1234, 0x30);  /* $1230 + $04 = $1234 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 5, "CMP (ZPG),Y takes 5 cycles without page cross");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cmp_ind_y_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x50);
    cpu_set_y(cpu, 0xFF);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CMP, IND_IDX));
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x20);  /* Base addr $1320 */
    memory_write(mem, 0x0011, 0x13);
    memory_write(mem, 0x141F, 0x30);  /* $1320 + $FF = $141F, page cross */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when A > M");
    CHECK(cycles == 6, "CMP (ZPG),Y takes 6 cycles with page cross");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* ========================== CPX Tests ====================================== */

TEST(test_cpx_equal) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x40);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPX, IMM));
    memory_write(mem, 0x0201, 0x40);

    uint8_t cycles = cpu_step(cpu);

    /* X == M: C=1, Z=1, N=0 */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when X == M");
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z should be set when X == M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when X == M");
    CHECK_EQ(cpu_get_x(cpu), 0x40);  /* X unchanged */
    CHECK(cycles == 2, "CPX IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpx_greater) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x40);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPX, IMM));
    memory_write(mem, 0x0201, 0x20);

    uint8_t cycles = cpu_step(cpu);

    /* X > M: C=1, Z=0, N=0 */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when X > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when X > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when result is positive");
    CHECK_EQ(cpu_get_x(cpu), 0x40);
    CHECK(cycles == 2, "CPX IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpx_less) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x20);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPX, IMM));
    memory_write(mem, 0x0201, 0x40);

    uint8_t cycles = cpu_step(cpu);

    /* X < M: C=0, Z=0, N=1 */
    CHECK(!(cpu_get_status(cpu) & FLAG_C), "C should be clear when X < M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when X < M");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N should be set when result bit 7 is set");
    CHECK_EQ(cpu_get_x(cpu), 0x20);
    CHECK(cycles == 2, "CPX IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpx_imm) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x40);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPX, IMM));
    memory_write(mem, 0x0201, 0x20);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when X > M");
    CHECK(cycles == 2, "CPX IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpx_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x40);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPX, ZPG));
    memory_write(mem, 0x0201, 0x42);
    memory_write(mem, 0x0042, 0x20);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when X > M");
    CHECK(cycles == 3, "CPX ZPG takes 3 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpx_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_x(cpu, 0x40);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPX, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x20);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when X > M");
    CHECK(cycles == 4, "CPX ABS takes 4 cycles");
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ========================== CPY Tests ====================================== */

TEST(test_cpy_equal) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x60);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPY, IMM));
    memory_write(mem, 0x0201, 0x60);

    uint8_t cycles = cpu_step(cpu);

    /* Y == M: C=1, Z=1, N=0 */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when Y == M");
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z should be set when Y == M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when Y == M");
    CHECK_EQ(cpu_get_y(cpu), 0x60);  /* Y unchanged */
    CHECK(cycles == 2, "CPY IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpy_greater) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x60);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPY, IMM));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    /* Y > M: C=1, Z=0, N=0 */
    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when Y > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when Y > M");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear when result is positive");
    CHECK_EQ(cpu_get_y(cpu), 0x60);
    CHECK(cycles == 2, "CPY IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpy_less) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x10);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPY, IMM));
    memory_write(mem, 0x0201, 0x60);

    uint8_t cycles = cpu_step(cpu);

    /* Y < M: C=0, Z=0, N=1 */
    CHECK(!(cpu_get_status(cpu) & FLAG_C), "C should be clear when Y < M");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear when Y < M");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N should be set when result bit 7 is set");
    CHECK_EQ(cpu_get_y(cpu), 0x10);
    CHECK(cycles == 2, "CPY IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpy_imm) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x60);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPY, IMM));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when Y > M");
    CHECK(cycles == 2, "CPY IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpy_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x60);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPY, ZPG));
    memory_write(mem, 0x0201, 0x42);
    memory_write(mem, 0x0042, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when Y > M");
    CHECK(cycles == 3, "CPY ZPG takes 3 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_cpy_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_y(cpu, 0x60);
    cpu_set_status(cpu, 0x00);

    memory_write(mem, 0x0200, encode_op(CPY, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C should be set when Y > M");
    CHECK(cycles == 4, "CPY ABS takes 4 cycles");
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

int main(void) {
    reset_test_state();
    printf("\n=== Arithmetic CPU Tests (ADC, SBC, INC/DEC, CMP/CPX/CPY) ===\n\n");

    printf("--- ADC Tests ---\n");
    RUN_TEST(test_add);
    RUN_TEST(test_add_carry);
    RUN_TEST(test_add_of);
    RUN_TEST(test_add_with_carry);
    RUN_TEST(test_add_with_carry_carry);
    RUN_TEST(test_add_with_carry_of);

    printf("\n--- SBC Tests ---\n");
    RUN_TEST(test_sbc);
    RUN_TEST(test_sbc_no_carry_set);
    RUN_TEST(test_sbc_simple);
    RUN_TEST(test_sbc_borrow);
    RUN_TEST(test_sbc_overflow_positive);
    RUN_TEST(test_sbc_overflow_negative);
    RUN_TEST(test_sbc_with_borrow_in);
    RUN_TEST(test_sbc_zpg);
    RUN_TEST(test_sbc_abs);
    RUN_TEST(test_sbc_abs_x_page_cross);

    printf("\n--- INC/DEC Tests ---\n");
    RUN_TEST(test_inc_zpg);
    RUN_TEST(test_inc_zpg_x);
    RUN_TEST(test_inc_abs);
    RUN_TEST(test_inc_abs_x);
    RUN_TEST(test_dec_zpg);

    printf("\n--- INX/INY/DEX/DEY Tests ---\n");
    RUN_TEST(test_inx);
    RUN_TEST(test_iny);
    RUN_TEST(test_dex);
    RUN_TEST(test_dey);

    printf("\n--- CMP Tests ---\n");
    RUN_TEST(test_cmp_equal);
    RUN_TEST(test_cmp_greater);
    RUN_TEST(test_cmp_less);
    RUN_TEST(test_cmp_greater_negative_result);
    RUN_TEST(test_cmp_less_positive_result);
    RUN_TEST(test_cmp_zero_vs_zero);
    RUN_TEST(test_cmp_imm);
    RUN_TEST(test_cmp_zpg);
    RUN_TEST(test_cmp_zpg_x);
    RUN_TEST(test_cmp_abs);
    RUN_TEST(test_cmp_abs_x);
    RUN_TEST(test_cmp_abs_x_page_cross);
    RUN_TEST(test_cmp_abs_y);
    RUN_TEST(test_cmp_ind_x);
    RUN_TEST(test_cmp_ind_y);
    RUN_TEST(test_cmp_ind_y_page_cross);

    printf("\n--- CPX Tests ---\n");
    RUN_TEST(test_cpx_equal);
    RUN_TEST(test_cpx_greater);
    RUN_TEST(test_cpx_less);
    RUN_TEST(test_cpx_imm);
    RUN_TEST(test_cpx_zpg);
    RUN_TEST(test_cpx_abs);

    printf("\n--- CPY Tests ---\n");
    RUN_TEST(test_cpy_equal);
    RUN_TEST(test_cpy_greater);
    RUN_TEST(test_cpy_less);
    RUN_TEST(test_cpy_imm);
    RUN_TEST(test_cpy_zpg);
    RUN_TEST(test_cpy_abs);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
