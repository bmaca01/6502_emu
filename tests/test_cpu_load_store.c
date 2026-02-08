#include "test_common.h"
#include "bus.h"

/* ============================ LDA Tests ==================================== */
TEST(test_lda_imm_positive) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA9);
    bus_write(bus, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42, "A should contain loaded value");
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2, "LDA IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_imm_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA9);  /* LDA #$00 */
    bus_write(bus, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00, "A should be zero");
    check_flags(cpu, 0, 1);  /* N=0, Z=1 */
    CHECK(cycles == 2, "LDA IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_imm_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA9);  /* LDA #$80 */
    bus_write(bus, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80, "A should contain loaded value");
    check_flags(cpu, 1, 0);  /* N=1, Z=0 */
    CHECK(cycles == 2, "LDA IMM takes 2 cycles");
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lda_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA5);  /* LDA $10 */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0010, 0x42);  /* Value at ZP $10 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42, "A should contain value from zero page");
    CHECK(cycles == 3, "LDA ZPG takes 3 cycles");
    check_pc(cpu, 0x0202);  /* ZPG: +2 */
    cpu_destroy(cpu);
}

TEST(test_lda_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x05);
    bus_write(bus, 0x0200, 0xB5);  /* LDA $10,X */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0015, 0x42);  /* Value at ZP $10+$05 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42, "A should contain value from ZPG+X");
    CHECK(cycles == 4, "LDA ZPG,X takes 4 cycles");
    check_pc(cpu, 0x0202);  /* ZPG_X: +2 */
    cpu_destroy(cpu);
}

TEST(test_lda_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xAD);  /* LDA $1234 */
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1234, 0x42);  /* Value at $1234 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42, "A should contain value from absolute address");
    CHECK(cycles == 4, "LDA ABS takes 4 cycles");
    check_pc(cpu, 0x0203);  /* ABS: +3 */
    cpu_destroy(cpu);
}

TEST(test_lda_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0200, 0xBD);  /* LDA $1230,X */
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1234, 0x42);  /* Value at $1230+$04 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);  /* ABS_X: +3 */
    cpu_destroy(cpu);
}

TEST(test_lda_abs_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0200, 0xB9);  /* LDA $1230,Y */
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1234, 0x42);  /* Value at $1230+$04 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);  /* ABS_Y: +3 */
    cpu_destroy(cpu);
}

TEST(test_lda_ind_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0200, 0xA1);  /* LDA ($10,X) */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0014, 0x34);  /* Low byte of target addr at $10+$04 */
    bus_write(bus, 0x0015, 0x12);  /* High byte */
    bus_write(bus, 0x1234, 0x42);  /* Value at $1234 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);  /* IDX_IND: +2 */
    cpu_destroy(cpu);
}

TEST(test_lda_ind_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0200, 0xB1);  /* LDA ($10),Y */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0010, 0x30);  /* Low byte of base addr */
    bus_write(bus, 0x0011, 0x12);  /* High byte */
    bus_write(bus, 0x1234, 0x42);  /* Value at $1230+$04 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 5);  /* No page cross */
    check_pc(cpu, 0x0202);  /* IND_IDX: +2 */
    cpu_destroy(cpu);
}

/* ============================ LDX Tests ==================================== */

TEST(test_ldx_imm_positive) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA2);  /* LDX #$42 */
    bus_write(bus, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_imm_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA2);  /* LDX #$00 */
    bus_write(bus, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_imm_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA2);  /* LDX #$80 */
    bus_write(bus, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA6);  /* LDX $10 */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0010, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_zpg_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x05);
    bus_write(bus, 0x0200, 0xB6);  /* LDX $10,Y */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0015, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldx_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xAE);  /* LDX $1234 */
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ldx_abs_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0200, 0xBE);  /* LDX $1230,Y */
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ LDY Tests ==================================== */

TEST(test_ldy_imm_positive) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA0);  /* LDY #$42 */
    bus_write(bus, 0x0201, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_imm_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA0);  /* LDY #$00 */
    bus_write(bus, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_imm_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA0);  /* LDY #$80 */
    bus_write(bus, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xA4);  /* LDY $10 */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0010, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x05);
    bus_write(bus, 0x0200, 0xB4);  /* LDY $10,X */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0015, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ldy_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, 0xAC);  /* LDY $1234 */
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ldy_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0200, 0xBC);  /* LDY $1230,X */
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1234, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ STA Tests ==================================== */

TEST(test_sta_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    bus_write(bus, 0x0200, 0x85);  /* STA $10 */
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0010) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x05);
    bus_write(bus, 0x0200, 0x95);  /* STA $10,X */
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0015) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    bus_write(bus, 0x0200, 0x8D);  /* STA $1234 */
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0200, 0x9D);  /* STA $1230,X */
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x42);
    CHECK(cycles == 5);  /* Stores always take 5 cycles for ABS,X */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_abs_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0200, 0x99);  /* STA $1230,Y */
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x42);
    CHECK(cycles == 5);  /* Stores always take 5 cycles for ABS,Y */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_ind_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0200, 0x81);  /* STA ($10,X) */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0014, 0x34);  /* Target addr low */
    bus_write(bus, 0x0015, 0x12);  /* Target addr high */

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x42);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_ind_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0200, 0x91);  /* STA ($10),Y */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0010, 0x30);  /* Base addr low */
    bus_write(bus, 0x0011, 0x12);  /* Base addr high */

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x42);
    CHECK(cycles == 6);  /* Stores always take 6 cycles for (IND),Y */
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sta_no_flag_change) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);  /* Zero value */
    cpu_set_status(cpu, 0x00);  /* Clear all flags */
    bus_write(bus, 0x0200, 0x85);  /* STA $10 */
    bus_write(bus, 0x0201, 0x10);

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x42);
    bus_write(bus, 0x0200, 0x86);  /* STX $10 */
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0010) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_stx_zpg_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x42);
    cpu_set_y(cpu, 0x05);
    bus_write(bus, 0x0200, 0x96);  /* STX $10,Y */
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0015) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_stx_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x42);
    bus_write(bus, 0x0200, 0x8E);  /* STX $1234 */
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ STY Tests ==================================== */

TEST(test_sty_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x42);
    bus_write(bus, 0x0200, 0x84);  /* STY $10 */
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0010) == 0x42);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sty_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x42);
    cpu_set_x(cpu, 0x05);
    bus_write(bus, 0x0200, 0x94);  /* STY $10,X */
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0015) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_sty_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x42);
    bus_write(bus, 0x0200, 0x8C);  /* STY $1234 */
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x42);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ======================= Page-Crossing Tests =============================== */

/* LDA Absolute,X with page cross: base=$12FF + X=$01 = $1300 (crosses page) */
TEST(test_lda_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x01);
    bus_write(bus, 0x0200, 0xBD);  /* LDA $12FF,X */
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1300, 0x42);  /* Value at $12FF+$01 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* LDA Absolute,Y with page cross: base=$12FF + Y=$01 = $1300 (crosses page) */
TEST(test_lda_abs_y_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x01);
    bus_write(bus, 0x0200, 0xB9);  /* LDA $12FF,Y */
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1300, 0x42);  /* Value at $12FF+$01 */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* LDA (Indirect),Y with page cross: base=$1320 + Y=$FF = $141F (crosses page) */
TEST(test_lda_ind_y_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0xFF);
    bus_write(bus, 0x0200, 0xB1);  /* LDA ($10),Y */
    bus_write(bus, 0x0201, 0x10);
    bus_write(bus, 0x0010, 0x20);  /* Base addr low: $1320 */
    bus_write(bus, 0x0011, 0x13);  /* Base addr high */
    bus_write(bus, 0x141F, 0x42);  /* Value at $1320+$FF */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    CHECK(cycles == 6);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* LDX Absolute,Y with page cross: base=$12FF + Y=$01 = $1300 */
TEST(test_ldx_abs_y_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x01);
    bus_write(bus, 0x0200, 0xBE);  /* LDX $12FF,Y */
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1300, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* LDY Absolute,X with page cross: base=$12FF + X=$01 = $1300 */
TEST(test_ldy_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x01);
    bus_write(bus, 0x0200, 0xBC);  /* LDY $12FF,X */
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);
    bus_write(bus, 0x1300, 0x42);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    CHECK(cycles == 5);  /* +1 cycle for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_sta_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x01);
    bus_write(bus, 0x0200, 0x9D);  /* STA $12FF,X */
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1300) == 0x42);
    CHECK(cycles == 5);  /* Stores always 5 cycles - no optimization */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================== Test Runner ================================ */

int main(void) {
    reset_test_state();
    printf("\n=== Load/Store Instruction Tests ===\n\n");

    printf("--- LDA Tests ---\n");
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

    printf("\n--- Page-Crossing Tests ---\n");
    RUN_TEST(test_lda_abs_x_page_cross);
    RUN_TEST(test_lda_abs_y_page_cross);
    RUN_TEST(test_lda_ind_y_page_cross);
    RUN_TEST(test_ldx_abs_y_page_cross);
    RUN_TEST(test_ldy_abs_x_page_cross);
    RUN_TEST(test_sta_abs_x_page_cross);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
