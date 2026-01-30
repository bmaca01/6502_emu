#include "cpu.h"
#include "util.h"
#include <stdio.h>
#include <string.h>

/* ============================= Test Framework ============================== */

#define MAX_FAILED_TESTS 256
static int check_failures = 0;      /* Total CHECK failures */
static int failed_test_count = 0;   /* Number of tests with failures */
static int tests_run = 0;
static const char* failed_tests[MAX_FAILED_TESTS];
static const char* current_test_name = NULL;
static int current_test_failed = 0;

#define TEST(name) static void name(void)

#define RUN_TEST(name) do { \
    current_test_name = #name; \
    current_test_failed = 0; \
    tests_run++; \
    printf("  %-40s", #name); \
    name(); \
    if (!current_test_failed) { \
        printf(" ✓\n"); \
    } else { \
        printf(" ✗\n"); \
        if (failed_test_count < MAX_FAILED_TESTS) { \
            failed_tests[failed_test_count++] = current_test_name; \
        } \
    } \
} while(0)

#define CHECK(condition) do { \
    if (!(condition)) { \
        check_failures++; \
        current_test_failed = 1; \
        printf("\n    FAIL: %s:%d: %s", __FILE__, __LINE__, #condition); \
    } \
} while(0)

#define CHECK_EQ(actual, expected) do { \
    int _a = (actual); \
    int _e = (expected); \
    if (_a != _e) { \
        check_failures++; \
        current_test_failed = 1; \
        printf("\n    FAIL: %s:%d: expected 0x%02X, got 0x%02X", \
               __FILE__, __LINE__, _e, _a); \
    } \
} while(0)

/* ============================= Test Helpers ================================ */

static CPU* setup_cpu(void) {
    Memory* mem = memory_create();
    /* Set reset vector to 0x0200 */
    memory_write(mem, 0xFFFC, 0x00);
    memory_write(mem, 0xFFFD, 0x02);
    return cpu_create(mem);
}

static void check_flags(CPU* cpu, int n_expected, int z_expected) {
    uint8_t status = cpu_get_status(cpu);
    int n_actual = !!(status & FLAG_N);
    int z_actual = !!(status & FLAG_Z);
    if (n_expected != n_actual) {
        CHECK(0 && "N flag mismatch");
    }
    if (z_expected != z_actual) {
        CHECK(0 && "Z flag mismatch");
    }
}

#define check_pc(cpu, expected) \
    CHECK_EQ(cpu_get_pc(cpu), (expected))

/* ================================= Tests =================================== */
TEST(test_cpu_create_destroy) {
    Memory* mem = memory_create();
    CPU* cpu = cpu_create(mem);

    CHECK(cpu != NULL);

    cpu_destroy(cpu);
}

TEST(test_cpu_nop) {
    Memory* mem = memory_create();
    memory_write(mem, 0x0000, 0xEA);

    CPU* cpu = cpu_create(mem);

    uint8_t cycles = cpu_step(cpu);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0001);  /* Implied: +1 */

    cpu_destroy(cpu);
}

/* ============================ LDA Tests ==================================== */
TEST(test_lda_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA9);
    memory_write(mem, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA9);  /* LDA #$00 */
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* N=0, Z=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA9);  /* LDA #$80 */
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1, Z=0 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA5);  /* LDA $10 */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x42);  /* Value at ZP $10 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);  /* ZPG: +2 */
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);  /* ZPG_X: +2 */
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);  /* ABS: +3 */
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);  /* ABS_X: +3 */
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);  /* ABS_Y: +3 */
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);  /* IDX_IND: +2 */
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 5);  /* No page cross */
    check_pc(cpu, 0x0202);  /* IND_IDX: +2 */
    cpu_destroy(cpu);
}

/* ============================ LDX Tests ==================================== */

TEST(test_ldx_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA2);  /* LDX #$42 */
    memory_write(mem, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA2);  /* LDX #$00 */
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA2);  /* LDX #$80 */
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA6);  /* LDX $10 */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
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

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
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

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
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

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ LDY Tests ==================================== */

TEST(test_ldy_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA0);  /* LDY #$42 */
    memory_write(mem, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA0);  /* LDY #$00 */
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA0);  /* LDY #$80 */
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0200, 0xA4);  /* LDY $10 */
    memory_write(mem, 0x0201, 0x10);
    memory_write(mem, 0x0010, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
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

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
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

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
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

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
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

    CHECK(memory_read(mem, 0x0010) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x0015) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x1234) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
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

    CHECK(memory_read(mem, 0x1234) == 0x42);
    CHECK(cycles == 5);  /* Stores always take 5 cycles for ABS,X */
    check_pc(cpu, 0x0203);
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

    CHECK(memory_read(mem, 0x1234) == 0x42);
    CHECK(cycles == 5);  /* Stores always take 5 cycles for ABS,Y */
    check_pc(cpu, 0x0203);
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

    CHECK(memory_read(mem, 0x1234) == 0x42);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x1234) == 0x42);
    CHECK(cycles == 6);  /* Stores always take 6 cycles for (IND),Y */
    check_pc(cpu, 0x0202);
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
    check_flags(cpu, 0, 0);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x0010) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x0015) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x1234) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
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

    CHECK(memory_read(mem, 0x0010) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x0015) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
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

    CHECK(memory_read(mem, 0x1234) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
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

    CHECK(cpu_get_x(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);  /* Implied: +1 */
    cpu_destroy(cpu);
}

TEST(test_tax_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);
    memory_write(mem, 0x0200, 0xAA);  /* TAX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tax_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    memory_write(mem, 0x0200, 0xAA);  /* TAX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TAY - Transfer A to Y */
TEST(test_tay_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    memory_write(mem, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tay_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);
    memory_write(mem, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tay_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    memory_write(mem, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TXA - Transfer X to A */
TEST(test_txa_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x42);
    memory_write(mem, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txa_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x00);
    memory_write(mem, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txa_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x80);
    memory_write(mem, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TYA - Transfer Y to A */
TEST(test_tya_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x42);
    memory_write(mem, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tya_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x00);
    memory_write(mem, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tya_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_y(cpu, 0x80);
    memory_write(mem, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TSX - Transfer SP to X */
TEST(test_tsx_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_sp(cpu, 0x42);
    memory_write(mem, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tsx_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_sp(cpu, 0x00);
    memory_write(mem, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tsx_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_sp(cpu, 0x80);
    memory_write(mem, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x42);
    cpu_set_status(cpu, 0x00);  /* Clear flags */
    memory_write(mem, 0x0200, 0x9A);  /* TXS */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_sp(cpu) == 0x42);
    check_flags(cpu, 0, 0);  /* Flags unchanged */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
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

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 6);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0202);
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

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
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

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
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

    CHECK(memory_read(mem, 0x1300) == 0x42);
    CHECK(cycles == 5);  /* Stores always 5 cycles - no optimization */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ======================= Stack push =============================== */
TEST(test_pha) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x42);
    memory_write(mem, 0x0200, encode_op(PHA, IMPL));
    uint8_t _sp = cpu_get_sp(cpu);

    uint8_t cycles = cpu_step(cpu);

    CHECK(_sp > cpu_get_sp(cpu));
    CHECK(_sp - 1 == cpu_get_sp(cpu));
    CHECK(memory_read(mem, (0x0100 | cpu_get_sp(cpu))) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0201);

    cpu_destroy(cpu);
}

TEST(test_php) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    //cpu_set_a(cpu, 0x42);
    memory_write(mem, 0x0200, encode_op(PHP, IMPL));
    uint8_t _sp = cpu_get_sp(cpu);
    uint8_t _p = cpu_get_status(cpu);

    uint8_t cycles = cpu_step(cpu);

    CHECK(_sp > cpu_get_sp(cpu));
    CHECK(_sp - 1 == cpu_get_sp(cpu));
    CHECK(memory_read(mem, (0x0100 | cpu_get_sp(cpu))) == (_p | FLAG_B | FLAG_U));
    CHECK(cycles == 3);
    check_pc(cpu, 0x0201);

    cpu_destroy(cpu);
}

TEST(test_pla) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /** simple program
     *  LDA #$AB
     *  PHA
     *  LDA #$00
     *  PLA ; A should be AB
     */
    memory_write(mem, 0x0200, encode_op(LDA, IMM));
    memory_write(mem, 0x0201,                0xAB);
    memory_write(mem, 0x0202, encode_op(PHA, IMPL));
    memory_write(mem, 0x0203, encode_op(LDA, IMM));
    memory_write(mem, 0x0204,                0x00);
    memory_write(mem, 0x0205, encode_op(PLA, IMPL));

    uint8_t _sp_old = cpu_get_sp(cpu);
    cpu_step(cpu);
    cpu_step(cpu);

    CHECK(_sp_old-1 == cpu_get_sp(cpu));

    cpu_step(cpu);
    CHECK(cpu_get_a(cpu) == 0);

    uint8_t pull_cycles = cpu_step(cpu);
    CHECK(cpu_get_a(cpu) == 0xAB);
    CHECK(_sp_old == cpu_get_sp(cpu));
    CHECK(pull_cycles == 4);

    cpu_destroy(cpu);
}

TEST(test_plp) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0200, encode_op(PHP, IMPL));
    memory_write(mem, 0x0201, encode_op(LDA, IMM));
    memory_write(mem, 0x0202,                0xF0); // Should set N
    memory_write(mem, 0x0203, encode_op(PLP, IMPL));

    uint8_t _sp_old = cpu_get_sp(cpu);
    uint8_t _p_old = cpu_get_status(cpu);

    cpu_step(cpu);
    cpu_step(cpu);
    CHECK(_sp_old - 1 == cpu_get_sp(cpu));
    CHECK(cpu_get_status(cpu) != _p_old);
    CHECK(cpu_get_status(cpu) & FLAG_N);

    uint8_t pull_cycles = cpu_step(cpu);
    CHECK(!(cpu_get_status(cpu) & FLAG_N));
    CHECK(cpu_get_status(cpu) == (_p_old | FLAG_B));   // Pushing sets bit 4
    CHECK(_sp_old == cpu_get_sp(cpu));
    CHECK(pull_cycles == 4);

    cpu_destroy(cpu);
}

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

/* ============================ AND Tests ==================================== */

TEST(test_and_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    memory_write(mem, 0x0200, encode_op(AND, IMM));
    memory_write(mem, 0x0201, 0x0F);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x0F);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xF0);
    memory_write(mem, 0x0200, encode_op(AND, IMM));
    memory_write(mem, 0x0201, 0x0F);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    memory_write(mem, 0x0200, encode_op(AND, IMM));
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    memory_write(mem, 0x0010, 0x55);
    memory_write(mem, 0x0200, encode_op(AND, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x55);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x05);
    memory_write(mem, 0x0015, 0xAA);
    memory_write(mem, 0x0200, encode_op(AND, ZPG_X));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xAA);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    memory_write(mem, 0x1234, 0x33);
    memory_write(mem, 0x0200, encode_op(AND, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x33);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x1234, 0x77);
    memory_write(mem, 0x0200, encode_op(AND, ABS_X));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x77);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x01);
    memory_write(mem, 0x1300, 0x0F);
    memory_write(mem, 0x0200, encode_op(AND, ABS_X));
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x0F);
    CHECK(cycles == 5);  /* +1 for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_abs_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x1234, 0x3C);
    memory_write(mem, 0x0200, encode_op(AND, ABS_Y));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x3C);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_ind_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0014, 0x34);  /* Low byte of target */
    memory_write(mem, 0x0015, 0x12);  /* High byte of target */
    memory_write(mem, 0x1234, 0x5A);
    memory_write(mem, 0x0200, encode_op(AND, IDX_IND));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x5A);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_ind_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0010, 0x30);  /* Low byte of base */
    memory_write(mem, 0x0011, 0x12);  /* High byte of base */
    memory_write(mem, 0x1234, 0xA5);
    memory_write(mem, 0x0200, encode_op(AND, IND_IDX));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xA5);
    CHECK(cycles == 5);  /* No page cross */
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* ============================ ORA Tests ==================================== */

TEST(test_ora_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);
    memory_write(mem, 0x0200, encode_op(ORA, IMM));
    memory_write(mem, 0x0201, 0x01);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x01);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);
    memory_write(mem, 0x0200, encode_op(ORA, IMM));
    memory_write(mem, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x00);
    memory_write(mem, 0x0200, encode_op(ORA, IMM));
    memory_write(mem, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x0F);
    memory_write(mem, 0x0010, 0xF0);
    memory_write(mem, 0x0200, encode_op(ORA, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x55);
    memory_write(mem, 0x1234, 0xAA);
    memory_write(mem, 0x0200, encode_op(ORA, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ora_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x0F);
    cpu_set_x(cpu, 0x01);
    memory_write(mem, 0x1300, 0x70);
    memory_write(mem, 0x0200, encode_op(ORA, ABS_X));
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x7F);
    CHECK(cycles == 5);  /* +1 for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ora_ind_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x11);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0014, 0x34);
    memory_write(mem, 0x0015, 0x12);
    memory_write(mem, 0x1234, 0x22);
    memory_write(mem, 0x0200, encode_op(ORA, IDX_IND));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x33);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_ind_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x44);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0010, 0x30);
    memory_write(mem, 0x0011, 0x12);
    memory_write(mem, 0x1234, 0x88);
    memory_write(mem, 0x0200, encode_op(ORA, IND_IDX));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xCC);
    CHECK(cycles == 5);  /* No page cross */
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* ============================ EOR Tests ==================================== */

TEST(test_eor_imm_positive) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xF0);
    memory_write(mem, 0x0200, encode_op(EOR, IMM));
    memory_write(mem, 0x0201, 0xFF);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x0F);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_imm_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    memory_write(mem, 0x0200, encode_op(EOR, IMM));
    memory_write(mem, 0x0201, 0xFF);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_imm_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x8F);
    memory_write(mem, 0x0200, encode_op(EOR, IMM));
    memory_write(mem, 0x0201, 0x0F);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xAA);
    memory_write(mem, 0x0010, 0x55);
    memory_write(mem, 0x0200, encode_op(EOR, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x12);
    memory_write(mem, 0x1234, 0x34);
    memory_write(mem, 0x0200, encode_op(EOR, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x26);  /* 0x12 ^ 0x34 = 0x26 */
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_eor_abs_y_page_cross) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_y(cpu, 0x01);
    memory_write(mem, 0x1300, 0x0F);
    memory_write(mem, 0x0200, encode_op(EOR, ABS_Y));
    memory_write(mem, 0x0201, 0xFF);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xF0);
    CHECK(cycles == 5);  /* +1 for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_eor_ind_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x55);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x0014, 0x34);
    memory_write(mem, 0x0015, 0x12);
    memory_write(mem, 0x1234, 0xAA);
    memory_write(mem, 0x0200, encode_op(EOR, IDX_IND));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_ind_y) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x3C);
    cpu_set_y(cpu, 0x04);
    memory_write(mem, 0x0010, 0x30);
    memory_write(mem, 0x0011, 0x12);
    memory_write(mem, 0x1234, 0x3C);
    memory_write(mem, 0x0200, encode_op(EOR, IND_IDX));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);  /* XOR with self = 0 */
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 5);  /* No page cross */
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* ============================ ASL Tests ==================================== */

TEST(test_asl_acc) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x01);
    memory_write(mem, 0x0200, encode_op(ASL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x02);
    check_flags(cpu, 0, 0);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_asl_acc_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x81);
    memory_write(mem, 0x0200, encode_op(ASL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x02);
    check_flags(cpu, 0, 0);
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* MSB shifted to carry */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_asl_acc_zero) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    memory_write(mem, 0x0200, encode_op(ASL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_asl_acc_negative) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x40);
    memory_write(mem, 0x0200, encode_op(ASL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_asl_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0010, 0x55);
    memory_write(mem, 0x0200, encode_op(ASL, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0010) == 0xAA);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_asl_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x05);
    memory_write(mem, 0x0015, 0x01);
    memory_write(mem, 0x0200, encode_op(ASL, ZPG_X));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0015) == 0x02);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_asl_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x1234, 0x40);
    memory_write(mem, 0x0200, encode_op(ASL, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x80);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_asl_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x1234, 0x01);
    memory_write(mem, 0x0200, encode_op(ASL, ABS_X));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x02);
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ ROL Tests ==================================== */

TEST(test_rol_acc_no_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x0200, encode_op(ROL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x02);
    check_flags(cpu, 0, 0);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_rol_acc_with_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x0200, encode_op(ROL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x03);  /* bit 0 gets old carry */
    check_flags(cpu, 0, 0);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_rol_acc_to_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x0200, encode_op(ROL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* MSB to carry */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_rol_acc_carry_through) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x0200, encode_op(ROL, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x01);  /* old carry becomes bit 0 */
    check_flags(cpu, 0, 0);
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* MSB to carry */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_rol_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x0010, 0xAA);
    memory_write(mem, 0x0200, encode_op(ROL, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0010) == 0x55);  /* 0xAA << 1 | 1 = 0x55 */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_rol_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x05);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x0015, 0x55);
    memory_write(mem, 0x0200, encode_op(ROL, ZPG_X));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0015) == 0xAA);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_rol_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x1234, 0x01);
    memory_write(mem, 0x0200, encode_op(ROL, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x03);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_rol_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x04);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x1234, 0x80);
    memory_write(mem, 0x0200, encode_op(ROL, ABS_X));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ LSR Tests ==================================== */

TEST(test_lsr_acc) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x02);
    memory_write(mem, 0x0200, encode_op(LSR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x01);
    check_flags(cpu, 0, 0);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_lsr_acc_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x01);
    memory_write(mem, 0x0200, encode_op(LSR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* LSB to carry */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_lsr_acc_high_bit) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x80);
    memory_write(mem, 0x0200, encode_op(LSR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x40);
    check_flags(cpu, 0, 0);  /* N always 0 for LSR */
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_lsr_acc_both) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x81);
    memory_write(mem, 0x0200, encode_op(LSR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x40);
    check_flags(cpu, 0, 0);
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* LSB to carry */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_lsr_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x0010, 0xAA);
    memory_write(mem, 0x0200, encode_op(LSR, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0010) == 0x55);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lsr_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x05);
    memory_write(mem, 0x0015, 0x55);
    memory_write(mem, 0x0200, encode_op(LSR, ZPG_X));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0015) == 0x2A);
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* 0x55 has bit 0 set */
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lsr_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    memory_write(mem, 0x1234, 0x02);
    memory_write(mem, 0x0200, encode_op(LSR, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x01);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_lsr_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x04);
    memory_write(mem, 0x1234, 0xFF);
    memory_write(mem, 0x0200, encode_op(LSR, ABS_X));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x7F);
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ ROR Tests ==================================== */

TEST(test_ror_acc_no_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x02);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x0200, encode_op(ROR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x01);
    check_flags(cpu, 0, 0);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_ror_acc_with_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x02);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x0200, encode_op(ROR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x81);  /* carry goes to bit 7 */
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_ror_acc_to_carry) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x0200, encode_op(ROR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* LSB to carry */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_ror_acc_carry_through) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x0200, encode_op(ROR, ACC));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);  /* carry to bit 7, LSB to carry */
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_ror_zpg) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x0010, 0x55);
    memory_write(mem, 0x0200, encode_op(ROR, ZPG));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0010) == 0xAA);  /* C->bit7, bit0->C */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ror_zpg_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x05);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x0015, 0xAA);
    memory_write(mem, 0x0200, encode_op(ROR, ZPG_X));
    memory_write(mem, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x0015) == 0x55);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ror_abs) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    memory_write(mem, 0x1234, 0x02);
    memory_write(mem, 0x0200, encode_op(ROR, ABS));
    memory_write(mem, 0x0201, 0x34);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x81);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ror_abs_x) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    cpu_set_x(cpu, 0x04);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    memory_write(mem, 0x1234, 0x01);
    memory_write(mem, 0x0200, encode_op(ROR, ABS_X));
    memory_write(mem, 0x0201, 0x30);
    memory_write(mem, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(memory_read(mem, 0x1234) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
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

    printf("\n--- Stack Tests ---\n");
    RUN_TEST(test_pha);
    RUN_TEST(test_php);
    RUN_TEST(test_pla);
    RUN_TEST(test_plp);

    printf("\n--- Increment / Decrement Tests ---\n");
    RUN_TEST(test_inc_zpg);
    RUN_TEST(test_inc_zpg_x);
    RUN_TEST(test_inc_abs);
    RUN_TEST(test_inc_abs_x);
    RUN_TEST(test_dec_zpg);
    RUN_TEST(test_inx);
    RUN_TEST(test_iny);
    RUN_TEST(test_dex);
    RUN_TEST(test_dey);

    printf("\n--- ADC tests ---\n");
    RUN_TEST(test_add);
    RUN_TEST(test_add_carry);
    RUN_TEST(test_add_of);
    RUN_TEST(test_add_with_carry);
    RUN_TEST(test_add_with_carry_carry);
    RUN_TEST(test_add_with_carry_of);

    printf("\n--- SBC tests ---\n");
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

    printf("\n--- AND tests ---\n");
    RUN_TEST(test_and_imm_positive);
    RUN_TEST(test_and_imm_zero);
    RUN_TEST(test_and_imm_negative);
    RUN_TEST(test_and_zpg);
    RUN_TEST(test_and_zpg_x);
    RUN_TEST(test_and_abs);
    RUN_TEST(test_and_abs_x);
    RUN_TEST(test_and_abs_x_page_cross);
    RUN_TEST(test_and_abs_y);
    RUN_TEST(test_and_ind_x);
    RUN_TEST(test_and_ind_y);

    printf("\n--- ORA tests ---\n");
    RUN_TEST(test_ora_imm_positive);
    RUN_TEST(test_ora_imm_zero);
    RUN_TEST(test_ora_imm_negative);
    RUN_TEST(test_ora_zpg);
    RUN_TEST(test_ora_abs);
    RUN_TEST(test_ora_abs_x_page_cross);
    RUN_TEST(test_ora_ind_x);
    RUN_TEST(test_ora_ind_y);

    printf("\n--- EOR tests ---\n");
    RUN_TEST(test_eor_imm_positive);
    RUN_TEST(test_eor_imm_zero);
    RUN_TEST(test_eor_imm_negative);
    RUN_TEST(test_eor_zpg);
    RUN_TEST(test_eor_abs);
    RUN_TEST(test_eor_abs_y_page_cross);
    RUN_TEST(test_eor_ind_x);
    RUN_TEST(test_eor_ind_y);

    printf("\n--- ASL tests ---\n");
    RUN_TEST(test_asl_acc);
    RUN_TEST(test_asl_acc_carry);
    RUN_TEST(test_asl_acc_zero);
    RUN_TEST(test_asl_acc_negative);
    RUN_TEST(test_asl_zpg);
    RUN_TEST(test_asl_zpg_x);
    RUN_TEST(test_asl_abs);
    RUN_TEST(test_asl_abs_x);

    printf("\n--- ROL tests ---\n");
    RUN_TEST(test_rol_acc_no_carry);
    RUN_TEST(test_rol_acc_with_carry);
    RUN_TEST(test_rol_acc_to_carry);
    RUN_TEST(test_rol_acc_carry_through);
    RUN_TEST(test_rol_zpg);
    RUN_TEST(test_rol_zpg_x);
    RUN_TEST(test_rol_abs);
    RUN_TEST(test_rol_abs_x);

    printf("\n--- LSR tests ---\n");
    RUN_TEST(test_lsr_acc);
    RUN_TEST(test_lsr_acc_carry);
    RUN_TEST(test_lsr_acc_high_bit);
    RUN_TEST(test_lsr_acc_both);
    RUN_TEST(test_lsr_zpg);
    RUN_TEST(test_lsr_zpg_x);
    RUN_TEST(test_lsr_abs);
    RUN_TEST(test_lsr_abs_x);

    printf("\n--- ROR tests ---\n");
    RUN_TEST(test_ror_acc_no_carry);
    RUN_TEST(test_ror_acc_with_carry);
    RUN_TEST(test_ror_acc_to_carry);
    RUN_TEST(test_ror_acc_carry_through);
    RUN_TEST(test_ror_zpg);
    RUN_TEST(test_ror_zpg_x);
    RUN_TEST(test_ror_abs);
    RUN_TEST(test_ror_abs_x);

    /* Summary */
    printf("\n========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Passed: %d\n", tests_run - failed_test_count);
    printf("Failed: %d\n", failed_test_count);

    if (failed_test_count > 0) {
        printf("\nFailed tests:\n");
        for (int i = 0; i < failed_test_count; i++) {
            printf("  - %s\n", failed_tests[i]);
        }
        printf("\n");
        return 1;
    }

    printf("\nAll CPU tests passed!\n\n");
    return 0;
}
