#include "cpu.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("  %-40s", #name); \
    name(); \
    printf(" ✓\n"); \
} while(0)

/* ============================= Test Helpers ================================ */

static CPU* setup_cpu(void) {
    Memory* mem = memory_create();
    /* Set reset vector to 0x0200 */
    memory_write(mem, 0xFFFC, 0x00);
    memory_write(mem, 0xFFFD, 0x02);
    return cpu_create(mem);
}

static void assert_flags(CPU* cpu, int n_expected, int z_expected) {
    uint8_t status = cpu_get_status(cpu);
    assert(n_expected == !!(status & FLAG_N));
    assert(z_expected == !!(status & FLAG_Z));
}

#define assert_pc(cpu, expected) \
    assert(cpu_get_pc(cpu) == (expected))

/* ================================= Tests =================================== */
TEST(test_cpu_create_destroy) {
    Memory* mem = memory_create();
    CPU* cpu = cpu_create(mem);

    assert(cpu != NULL);

    cpu_destroy(cpu);
}

TEST(test_cpu_nop) {
    Memory* mem = memory_create();
    memory_write(mem, 0x0000, 0xEA);

    CPU* cpu = cpu_create(mem);

    uint8_t cycles = cpu_step(cpu);
    assert(cycles == 2);
    assert_pc(cpu, 0x0001);  /* Implied: +1 */

    cpu_destroy(cpu);
}

/* ============================ LDA Tests ==================================== */
TEST(test_lda_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA9);
    memory_write(mem, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA9);  /* LDA #$00 */
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x00);
    assert_flags(cpu, 0, 1);  /* N=0, Z=1 */
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA9);  /* LDA #$80 */
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x80);
    assert_flags(cpu, 1, 0);  /* N=1, Z=0 */
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA5);  /* LDA $10 */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x42);  /* Value at ZP $10 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 3);
    assert_pc(cpu, 0x0202);  /* ZPG: +2 */
    cpu_destroy(cpu);
}

TEST(test_lda_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x05);
    memory_write(mem, 0x0200, 0xB5);  /* LDA $10,X */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0015, 0x42);  /* Value at ZP $10+$05 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0202);  /* ZPG_X: +2 */
    cpu_destroy(cpu);
}

TEST(test_lda_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xAD);  /* LDA $1234 */
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x42);  /* Value at $1234 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0203);  /* ABS: +3 */
    cpu_destroy(cpu);
}

TEST(test_lda_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0200, 0xBD);  /* LDA $1230,X */
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x42);  /* Value at $1230+$04 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 4);  /* No page cross */
    assert_pc(cpu, 0x0203);  /* ABS_X: +3 */
    cpu_destroy(cpu);
}

TEST(test_lda_abs_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0200, 0xB9);  /* LDA $1230,Y */
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x42);  /* Value at $1230+$04 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 4);  /* No page cross */
    assert_pc(cpu, 0x0203);  /* ABS_Y: +3 */
    cpu_destroy(cpu);
}

TEST(test_lda_ind_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0200, 0xA1);  /* LDA ($10,X) */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0014, 0x34);  /* Low byte of target addr at $10+$04 */
    memory_write(mem, 0x0015, 0x12);  /* High byte */
    memory_write(mem, 0x1234, 0x42);  /* Value at $1234 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 6);
    assert_pc(cpu, 0x0202);  /* IDX_IND: +2 */
    cpu_destroy(cpu);
}

TEST(test_lda_ind_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0200, 0xB1);  /* LDA ($10),Y */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x30);  /* Low byte of base addr */
    memory_write(mem, 0x0011, 0x12);  /* High byte */
    memory_write(mem, 0x1234, 0x42);  /* Value at $1230+$04 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 5);  /* No page cross */
    assert_pc(cpu, 0x0202);  /* IND_IDX: +2 */
    cpu_destroy(cpu);
}

/* ============================ LDX Tests ==================================== */

TEST(test_ldx_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA2);  /* LDX #$42 */
    memory_write(mem, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA2);  /* LDX #$00 */
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x00);
    assert_flags(cpu, 0, 1);
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA2);  /* LDX #$80 */
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x80);
    assert_flags(cpu, 1, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA6);  /* LDX $10 */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert(cycles == 3);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_zpg_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x05);
    memory_write(mem, 0x0200, 0xB6);  /* LDX $10,Y */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0015, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xAE);  /* LDX $1234 */
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ldx_abs_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0200, 0xBE);  /* LDX $1230,Y */
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert(cycles == 4);  /* No page cross */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ LDY Tests ==================================== */

TEST(test_ldy_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA0);  /* LDY #$42 */
    memory_write(mem, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA0);  /* LDY #$00 */
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x00);
    assert_flags(cpu, 0, 1);
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA0);  /* LDY #$80 */
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x80);
    assert_flags(cpu, 1, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA4);  /* LDY $10 */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x42);
    assert(cycles == 3);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x05);
    memory_write(mem, 0x0200, 0xB4);  /* LDY $10,X */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0015, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xAC);  /* LDY $1234 */
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ldy_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0200, 0xBC);  /* LDY $1230,X */
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x42);
    assert(cycles == 4);  /* No page cross */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ STA Tests ==================================== */

TEST(test_sta_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    memory_write(mem, 0x0200, 0x85);  /* STA $10 */
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x0010) == 0x42);
    assert(cycles == 3);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x05);
    memory_write(mem, 0x0200, 0x95);  /* STA $10,X */
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x0015) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    memory_write(mem, 0x0200, 0x8D);  /* STA $1234 */
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1234) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0200, 0x9D);  /* STA $1230,X */
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1234) == 0x42);
    assert(cycles == 5);  /* Stores always take 5 cycles for ABS,X */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_abs_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0200, 0x99);  /* STA $1230,Y */
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1234) == 0x42);
    assert(cycles == 5);  /* Stores always take 5 cycles for ABS,Y */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_ind_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0200, 0x81);  /* STA ($10,X) */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0014, 0x34);  /* Target addr low */
    memory_write(mem, 0x0015, 0x12);  /* Target addr high */

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1234) == 0x42);
    assert(cycles == 6);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_ind_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0200, 0x91);  /* STA ($10),Y */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x30);  /* Base addr low */
    memory_write(mem, 0x0011, 0x12);  /* Base addr high */

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1234) == 0x42);
    assert(cycles == 6);  /* Stores always take 6 cycles for (IND),Y */
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_no_flag_change) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);  /* Zero value */
    cpu_set_status(cpu, 0x00);  /* Clear all flags */
    memory_write(mem, 0x0200, 0x85);  /* STA $10 */
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    /* Store should NOT modify flags, even when storing zero */
    assert_flags(cpu, 0, 0);
    assert(cycles == 3);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* ============================ STX Tests ==================================== */

TEST(test_stx_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x42);
    memory_write(mem, 0x0200, 0x86);  /* STX $10 */
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x0010) == 0x42);
    assert(cycles == 3);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_stx_zpg_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x42);
    cpu_set_y(cpu, 0x05);
    memory_write(mem, 0x0200, 0x96);  /* STX $10,Y */
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x0015) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_stx_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x42);
    memory_write(mem, 0x0200, 0x8E);  /* STX $1234 */
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1234) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ STY Tests ==================================== */

TEST(test_sty_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x42);
    memory_write(mem, 0x0200, 0x84);  /* STY $10 */
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x0010) == 0x42);
    assert(cycles == 3);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sty_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x42);
    cpu_set_x(cpu, 0x05);
    memory_write(mem, 0x0200, 0x94);  /* STY $10,X */
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x0015) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sty_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x42);
    memory_write(mem, 0x0200, 0x8C);  /* STY $1234 */
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1234) == 0x42);
    assert(cycles == 4);
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ========================= Transfer Tests ================================== */

/* TAX - Transfer A to X */
TEST(test_tax_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    memory_write(mem, 0x0200, 0xAA);  /* TAX */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);  /* Implied: +1 */
    cpu_destroy(cpu);
}

TEST(test_tax_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);
    memory_write(mem, 0x0200, 0xAA);  /* TAX */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x00);
    assert_flags(cpu, 0, 1);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tax_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    memory_write(mem, 0x0200, 0xAA);  /* TAX */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x80);
    assert_flags(cpu, 1, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TAY - Transfer A to Y */
TEST(test_tay_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    memory_write(mem, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tay_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);
    memory_write(mem, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x00);
    assert_flags(cpu, 0, 1);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tay_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    memory_write(mem, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x80);
    assert_flags(cpu, 1, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TXA - Transfer X to A */
TEST(test_txa_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x42);
    memory_write(mem, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txa_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x00);
    memory_write(mem, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x00);
    assert_flags(cpu, 0, 1);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txa_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x80);
    memory_write(mem, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x80);
    assert_flags(cpu, 1, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TYA - Transfer Y to A */
TEST(test_tya_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x42);
    memory_write(mem, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tya_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x00);
    memory_write(mem, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x00);
    assert_flags(cpu, 0, 1);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tya_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x80);
    memory_write(mem, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x80);
    assert_flags(cpu, 1, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TSX - Transfer SP to X */
TEST(test_tsx_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_sp(cpu, 0x42);
    memory_write(mem, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert_flags(cpu, 0, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tsx_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_sp(cpu, 0x00);
    memory_write(mem, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x00);
    assert_flags(cpu, 0, 1);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tsx_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_sp(cpu, 0x80);
    memory_write(mem, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x80);
    assert_flags(cpu, 1, 0);
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x42);
    cpu_set_status(cpu, 0x00);  /* Clear flags */
    memory_write(mem, 0x0200, 0x9A);  /* TXS */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_sp(cpu) == 0x42);
    assert_flags(cpu, 0, 0);  /* Flags unchanged */
    assert(cycles == 2);
    assert_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* ======================= Page-Crossing Tests =============================== */

/* LDA Absolute,X with page cross: base=$12FF + X=$01 = $1300 (crosses page) */
TEST(test_lda_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x01);
    memory_write(mem, 0x0200, 0xBD);  /* LDA $12FF,X */
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1300, 0x42);  /* Value at $12FF+$01 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 5);  /* +1 cycle for page cross */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* LDA Absolute,Y with page cross: base=$12FF + Y=$01 = $1300 (crosses page) */
TEST(test_lda_abs_y_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x01);
    memory_write(mem, 0x0200, 0xB9);  /* LDA $12FF,Y */
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1300, 0x42);  /* Value at $12FF+$01 */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 5);  /* +1 cycle for page cross */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* LDA (Indirect),Y with page cross: base=$1320 + Y=$FF = $141F (crosses page) */
TEST(test_lda_ind_y_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0xFF);
    memory_write(mem, 0x0200, 0xB1);  /* LDA ($10),Y */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x20);  /* Base addr low: $1320 */
    memory_write(mem, 0x0011, 0x13);  /* Base addr high */
    memory_write(mem, 0x141F, 0x42);  /* Value at $1320+$FF */

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_a(cpu) == 0x42);
    assert(cycles == 6);  /* +1 cycle for page cross */
    assert_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* LDX Absolute,Y with page cross: base=$12FF + Y=$01 = $1300 */
TEST(test_ldx_abs_y_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x01);
    memory_write(mem, 0x0200, 0xBE);  /* LDX $12FF,Y */
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1300, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_x(cpu) == 0x42);
    assert(cycles == 5);  /* +1 cycle for page cross */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* LDY Absolute,X with page cross: base=$12FF + X=$01 = $1300 */
TEST(test_ldy_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x01);
    memory_write(mem, 0x0200, 0xBC);  /* LDY $12FF,X */
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);
    memory_write(mem, 0x1300, 0x42);

    uint8_t cycles = cpu_step(cpu);

    assert(cpu_get_y(cpu) == 0x42);
    assert(cycles == 5);  /* +1 cycle for page cross */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x01);
    memory_write(mem, 0x0200, 0x9D);  /* STA $12FF,X */
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    assert(memory_read(mem, 0x1300) == 0x42);
    assert(cycles == 5);  /* Stores always 5 cycles - no optimization */
    assert_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

int main(void) {
    printf("\n=== CPU Module Tests ===\n\n");

    RUN_TEST(test_cpu_create_destroy);
    RUN_TEST(test_cpu_nop);

    printf("\n--- LDA Tests ---\n");
    RUN_TEST(test_lda_imm_positive);
    RUN_TEST(test_lda_imm_zero);
    RUN_TEST(test_lda_imm_negative);
    RUN_TEST(test_lda_zpg);
    RUN_TEST(test_lda_zpg_x);
    RUN_TEST(test_lda_abs);
    RUN_TEST(test_lda_abs_x);
    RUN_TEST(test_lda_abs_y);
    RUN_TEST(test_lda_ind_x);
    RUN_TEST(test_lda_ind_y);

    printf("\n--- LDX Tests ---\n");
    RUN_TEST(test_ldx_imm_positive);
    RUN_TEST(test_ldx_imm_zero);
    RUN_TEST(test_ldx_imm_negative);
    RUN_TEST(test_ldx_zpg);
    RUN_TEST(test_ldx_zpg_y);
    RUN_TEST(test_ldx_abs);
    RUN_TEST(test_ldx_abs_y);

    printf("\n--- LDY Tests ---\n");
    RUN_TEST(test_ldy_imm_positive);
    RUN_TEST(test_ldy_imm_zero);
    RUN_TEST(test_ldy_imm_negative);
    RUN_TEST(test_ldy_zpg);
    RUN_TEST(test_ldy_zpg_x);
    RUN_TEST(test_ldy_abs);
    RUN_TEST(test_ldy_abs_x);

    printf("\n--- STA Tests ---\n");
    RUN_TEST(test_sta_zpg);
    RUN_TEST(test_sta_zpg_x);
    RUN_TEST(test_sta_abs);
    RUN_TEST(test_sta_abs_x);
    RUN_TEST(test_sta_abs_y);
    RUN_TEST(test_sta_ind_x);
    RUN_TEST(test_sta_ind_y);
    RUN_TEST(test_sta_no_flag_change);

    printf("\n--- STX Tests ---\n");
    RUN_TEST(test_stx_zpg);
    RUN_TEST(test_stx_zpg_y);
    RUN_TEST(test_stx_abs);

    printf("\n--- STY Tests ---\n");
    RUN_TEST(test_sty_zpg);
    RUN_TEST(test_sty_zpg_x);
    RUN_TEST(test_sty_abs);

    printf("\n--- Transfer Tests ---\n");
    RUN_TEST(test_tax_positive);
    RUN_TEST(test_tax_zero);
    RUN_TEST(test_tax_negative);
    RUN_TEST(test_tay_positive);
    RUN_TEST(test_tay_zero);
    RUN_TEST(test_tay_negative);
    RUN_TEST(test_txa_positive);
    RUN_TEST(test_txa_zero);
    RUN_TEST(test_txa_negative);
    RUN_TEST(test_tya_positive);
    RUN_TEST(test_tya_zero);
    RUN_TEST(test_tya_negative);
    RUN_TEST(test_tsx_positive);
    RUN_TEST(test_tsx_zero);
    RUN_TEST(test_tsx_negative);
    RUN_TEST(test_txs);

    printf("\n--- Page-Crossing Tests ---\n");
    RUN_TEST(test_lda_abs_x_page_cross);
    RUN_TEST(test_lda_abs_y_page_cross);
    RUN_TEST(test_lda_ind_y_page_cross);
    RUN_TEST(test_ldx_abs_y_page_cross);
    RUN_TEST(test_ldy_abs_x_page_cross);
    RUN_TEST(test_sta_abs_x_page_cross);

    printf("\nAll CPU tests passed!\n\n");
    return 0;
}
