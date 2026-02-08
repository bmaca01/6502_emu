/*
 * Logic/Bitwise CPU tests for 6502 emulator
 * Tests: AND, ORA, EOR, ASL, LSR, ROL, ROR, BIT
 */

#include "test_common.h"
#include "bus.h"
#include "opcodes.h"
#include "addressing.h"

/* ============================ AND Tests ==================================== */

TEST(test_and_imm_positive) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0200, encode_op(AND, IMM));
    bus_write(bus, 0x0201, 0x0F);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x0F);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_imm_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xF0);
    bus_write(bus, 0x0200, encode_op(AND, IMM));
    bus_write(bus, 0x0201, 0x0F);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_imm_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0200, encode_op(AND, IMM));
    bus_write(bus, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0010, 0x55);
    bus_write(bus, 0x0200, encode_op(AND, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x55);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x05);
    bus_write(bus, 0x0015, 0xAA);
    bus_write(bus, 0x0200, encode_op(AND, ZPG_X));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xAA);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x1234, 0x33);
    bus_write(bus, 0x0200, encode_op(AND, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x33);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x1234, 0x77);
    bus_write(bus, 0x0200, encode_op(AND, ABS_X));
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x77);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x01);
    bus_write(bus, 0x1300, 0x0F);
    bus_write(bus, 0x0200, encode_op(AND, ABS_X));
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x0F);
    CHECK(cycles == 5);  /* +1 for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_abs_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x1234, 0x3C);
    bus_write(bus, 0x0200, encode_op(AND, ABS_Y));
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x3C);
    CHECK(cycles == 4);  /* No page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_and_ind_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0014, 0x34);  /* Low byte of target */
    bus_write(bus, 0x0015, 0x12);  /* High byte of target */
    bus_write(bus, 0x1234, 0x5A);
    bus_write(bus, 0x0200, encode_op(AND, IDX_IND));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x5A);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_and_ind_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0010, 0x30);  /* Low byte of base */
    bus_write(bus, 0x0011, 0x12);  /* High byte of base */
    bus_write(bus, 0x1234, 0xA5);
    bus_write(bus, 0x0200, encode_op(AND, IND_IDX));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xA5);
    CHECK(cycles == 5);  /* No page cross */
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* ============================ ORA Tests ==================================== */

TEST(test_ora_imm_positive) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0200, encode_op(ORA, IMM));
    bus_write(bus, 0x0201, 0x01);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x01);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_imm_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0200, encode_op(ORA, IMM));
    bus_write(bus, 0x0201, 0x00);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_imm_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0200, encode_op(ORA, IMM));
    bus_write(bus, 0x0201, 0x80);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x0F);
    bus_write(bus, 0x0010, 0xF0);
    bus_write(bus, 0x0200, encode_op(ORA, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x55);
    bus_write(bus, 0x1234, 0xAA);
    bus_write(bus, 0x0200, encode_op(ORA, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ora_abs_x_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x0F);
    cpu_set_x(cpu, 0x01);
    bus_write(bus, 0x1300, 0x70);
    bus_write(bus, 0x0200, encode_op(ORA, ABS_X));
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x7F);
    CHECK(cycles == 5);  /* +1 for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ora_ind_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x11);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0014, 0x34);
    bus_write(bus, 0x0015, 0x12);
    bus_write(bus, 0x1234, 0x22);
    bus_write(bus, 0x0200, encode_op(ORA, IDX_IND));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x33);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ora_ind_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x44);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0010, 0x30);
    bus_write(bus, 0x0011, 0x12);
    bus_write(bus, 0x1234, 0x88);
    bus_write(bus, 0x0200, encode_op(ORA, IND_IDX));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xCC);
    CHECK(cycles == 5);  /* No page cross */
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

/* ============================ EOR Tests ==================================== */

TEST(test_eor_imm_positive) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xF0);
    bus_write(bus, 0x0200, encode_op(EOR, IMM));
    bus_write(bus, 0x0201, 0xFF);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x0F);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_imm_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0200, encode_op(EOR, IMM));
    bus_write(bus, 0x0201, 0xFF);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_imm_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x8F);
    bus_write(bus, 0x0200, encode_op(EOR, IMM));
    bus_write(bus, 0x0201, 0x0F);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_zpg) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xAA);
    bus_write(bus, 0x0010, 0x55);
    bus_write(bus, 0x0200, encode_op(EOR, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x12);
    bus_write(bus, 0x1234, 0x34);
    bus_write(bus, 0x0200, encode_op(EOR, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x26);  /* 0x12 ^ 0x34 = 0x26 */
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_eor_abs_y_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    cpu_set_y(cpu, 0x01);
    bus_write(bus, 0x1300, 0x0F);
    bus_write(bus, 0x0200, encode_op(EOR, ABS_Y));
    bus_write(bus, 0x0201, 0xFF);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xF0);
    CHECK(cycles == 5);  /* +1 for page cross */
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_eor_ind_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x55);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x0014, 0x34);
    bus_write(bus, 0x0015, 0x12);
    bus_write(bus, 0x1234, 0xAA);
    bus_write(bus, 0x0200, encode_op(EOR, IDX_IND));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0xFF);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_eor_ind_y) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x3C);
    cpu_set_y(cpu, 0x04);
    bus_write(bus, 0x0010, 0x30);
    bus_write(bus, 0x0011, 0x12);
    bus_write(bus, 0x1234, 0x3C);
    bus_write(bus, 0x0200, encode_op(EOR, IND_IDX));
    bus_write(bus, 0x0201, 0x10);

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x01);
    bus_write(bus, 0x0200, encode_op(ASL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x81);
    bus_write(bus, 0x0200, encode_op(ASL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x80);
    bus_write(bus, 0x0200, encode_op(ASL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x40);
    bus_write(bus, 0x0200, encode_op(ASL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0010, 0x55);
    bus_write(bus, 0x0200, encode_op(ASL, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0010) == 0xAA);
    check_flags(cpu, 1, 0);  /* N=1 */
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_asl_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x05);
    bus_write(bus, 0x0015, 0x01);
    bus_write(bus, 0x0200, encode_op(ASL, ZPG_X));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0015) == 0x02);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_asl_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x1234, 0x40);
    bus_write(bus, 0x0200, encode_op(ASL, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x80);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_asl_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x1234, 0x01);
    bus_write(bus, 0x0200, encode_op(ASL, ABS_X));
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x02);
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ ROL Tests ==================================== */

TEST(test_rol_acc_no_carry) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x0200, encode_op(ROL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x0200, encode_op(ROL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x80);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x0200, encode_op(ROL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x80);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x0200, encode_op(ROL, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x0010, 0xAA);
    bus_write(bus, 0x0200, encode_op(ROL, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0010) == 0x55);  /* 0xAA << 1 | 1 = 0x55 */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_rol_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x05);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x0015, 0x55);
    bus_write(bus, 0x0200, encode_op(ROL, ZPG_X));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0015) == 0xAA);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_rol_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x1234, 0x01);
    bus_write(bus, 0x0200, encode_op(ROL, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x03);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_rol_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x04);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x1234, 0x80);
    bus_write(bus, 0x0200, encode_op(ROL, ABS_X));
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ LSR Tests ==================================== */

TEST(test_lsr_acc) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x02);
    bus_write(bus, 0x0200, encode_op(LSR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x01);
    bus_write(bus, 0x0200, encode_op(LSR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x80);
    bus_write(bus, 0x0200, encode_op(LSR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x81);
    bus_write(bus, 0x0200, encode_op(LSR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0010, 0xAA);
    bus_write(bus, 0x0200, encode_op(LSR, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0010) == 0x55);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lsr_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x05);
    bus_write(bus, 0x0015, 0x55);
    bus_write(bus, 0x0200, encode_op(LSR, ZPG_X));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0015) == 0x2A);
    CHECK(cpu_get_status(cpu) & FLAG_C);  /* 0x55 has bit 0 set */
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_lsr_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x1234, 0x02);
    bus_write(bus, 0x0200, encode_op(LSR, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x01);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_lsr_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x04);
    bus_write(bus, 0x1234, 0xFF);
    bus_write(bus, 0x0200, encode_op(LSR, ABS_X));
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x7F);
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* ============================ BIT Tests ==================================== */

/* Z Flag Tests */
TEST(test_bit_zpg_z_set) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x0F);
    bus_write(bus, 0x0010, 0xF0);        /* Memory value */
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cpu_get_a(cpu), 0x0F);         /* A unchanged */
    CHECK(!!(cpu_get_status(cpu) & FLAG_Z), "Z should be set");
    CHECK(!!(cpu_get_status(cpu) & FLAG_N), "N should be set (bit 7 of 0xF0)");
    CHECK(!!(cpu_get_status(cpu) & FLAG_V), "V should be set (bit 6 of 0xF0)");
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_bit_zpg_z_clear) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0010, 0x01);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cpu_get_a(cpu), 0xFF);         /* A unchanged */
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_V), "V should be clear");
    CHECK(cycles == 3);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_bit_a_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0010, 0xFF);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cpu_get_a(cpu), 0x00);
    CHECK(!!(cpu_get_status(cpu) & FLAG_Z), "Z should be set");
    CHECK(!!(cpu_get_status(cpu) & FLAG_N), "N from memory bit 7");
    CHECK(!!(cpu_get_status(cpu) & FLAG_V), "V from memory bit 6");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

TEST(test_bit_mem_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0010, 0x00);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cpu_get_a(cpu), 0xFF);
    CHECK(!!(cpu_get_status(cpu) & FLAG_Z), "Z should be set");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_V), "V should be clear");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

/* N Flag Tests (from memory bit 7) */
TEST(test_bit_n_set) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0010, 0x80);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(!!(cpu_get_status(cpu) & FLAG_N), "N should be set");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

TEST(test_bit_n_clear) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0010, 0x7F);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear");
    CHECK(!!(cpu_get_status(cpu) & FLAG_V), "V should be set (bit 6)");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

TEST(test_bit_n_independent) {
    /* N set even when Z=1 (A=0x00) */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0010, 0x80);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(!!(cpu_get_status(cpu) & FLAG_N), "N should be set");
    CHECK(!!(cpu_get_status(cpu) & FLAG_Z), "Z should be set");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

/* V Flag Tests (from memory bit 6) */
TEST(test_bit_v_set) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0010, 0x40);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(!!(cpu_get_status(cpu) & FLAG_V), "V should be set");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

TEST(test_bit_v_clear) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0xFF);
    bus_write(bus, 0x0010, 0xBF);  /* 0b10111111 - bit 6 clear */
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(!(cpu_get_status(cpu) & FLAG_V), "V should be clear");
    CHECK(!!(cpu_get_status(cpu) & FLAG_N), "N should be set (bit 7)");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

TEST(test_bit_v_independent) {
    /* V set even when Z=1 (A=0x00) */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0010, 0x40);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(!!(cpu_get_status(cpu) & FLAG_V), "V should be set");
    CHECK(!!(cpu_get_status(cpu) & FLAG_Z), "Z should be set");
    CHECK(cycles == 3);
    cpu_destroy(cpu);
}

/* A Register Preservation */
TEST(test_bit_a_unchanged) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x5A);
    bus_write(bus, 0x0010, 0xA5);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    cpu_step(cpu);

    CHECK_EQ(cpu_get_a(cpu), 0x5A);  /* A must not change */
    cpu_destroy(cpu);
}

/* Addressing Mode Tests */
TEST(test_bit_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x0F);
    bus_write(bus, 0x1234, 0xF0);
    bus_write(bus, 0x0200, encode_op(BIT, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cpu_get_a(cpu), 0x0F);
    CHECK(!!(cpu_get_status(cpu) & FLAG_Z), "Z should be set");
    CHECK(!!(cpu_get_status(cpu) & FLAG_N), "N from memory bit 7");
    CHECK(!!(cpu_get_status(cpu) & FLAG_V), "V from memory bit 6");
    CHECK(cycles == 4);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

/* Combined Flag Tests */
TEST(test_bit_all_flags_set) {
    /* Memory = 0xC0 (bits 7,6 set), A = 0x00 -> N=1, V=1, Z=1 */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0010, 0xC0);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    cpu_step(cpu);

    CHECK(!!(cpu_get_status(cpu) & FLAG_N), "N should be set");
    CHECK(!!(cpu_get_status(cpu) & FLAG_V), "V should be set");
    CHECK(!!(cpu_get_status(cpu) & FLAG_Z), "Z should be set");
    cpu_destroy(cpu);
}

TEST(test_bit_no_flags_set) {
    /* Memory = 0x3F (bits 7,6 clear), A = 0x3F -> N=0, V=0, Z=0 */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x3F);
    bus_write(bus, 0x0010, 0x3F);
    bus_write(bus, 0x0200, encode_op(BIT, ZPG));
    bus_write(bus, 0x0201, 0x10);

    cpu_step(cpu);

    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_V), "V should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z should be clear");
    cpu_destroy(cpu);
}

/* ============================ ROR Tests ==================================== */

TEST(test_ror_acc_no_carry) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x02);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x0200, encode_op(ROR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x02);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x0200, encode_op(ROR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x0200, encode_op(ROR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x01);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x0200, encode_op(ROR, ACC));

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x0010, 0x55);
    bus_write(bus, 0x0200, encode_op(ROR, ZPG));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0010) == 0xAA);  /* C->bit7, bit0->C */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 5);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ror_zpg_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x05);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x0015, 0xAA);
    bus_write(bus, 0x0200, encode_op(ROR, ZPG_X));
    bus_write(bus, 0x0201, 0x10);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x0015) == 0x55);
    CHECK(!(cpu_get_status(cpu) & FLAG_C));
    CHECK(cycles == 6);
    check_pc(cpu, 0x0202);
    cpu_destroy(cpu);
}

TEST(test_ror_abs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C=1 */
    bus_write(bus, 0x1234, 0x02);
    bus_write(bus, 0x0200, encode_op(ROR, ABS));
    bus_write(bus, 0x0201, 0x34);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x81);
    CHECK(cycles == 6);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

TEST(test_ror_abs_x) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x04);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C=0 */
    bus_write(bus, 0x1234, 0x01);
    bus_write(bus, 0x0200, encode_op(ROR, ABS_X));
    bus_write(bus, 0x0201, 0x30);
    bus_write(bus, 0x0202, 0x12);

    uint8_t cycles = cpu_step(cpu);

    CHECK(bus_read(bus, 0x1234) == 0x00);
    check_flags(cpu, 0, 1);  /* Z=1 */
    CHECK(cpu_get_status(cpu) & FLAG_C);
    CHECK(cycles == 7);
    check_pc(cpu, 0x0203);
    cpu_destroy(cpu);
}

int main(void) {
    reset_test_state();
    printf("\n=== Logic/Bitwise CPU Tests (AND, ORA, EOR, ASL, LSR, ROL, ROR, BIT) ===\n\n");

    printf("--- AND Tests ---\n");
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

    printf("\n--- ORA Tests ---\n");
    RUN_TEST(test_ora_imm_positive);
    RUN_TEST(test_ora_imm_zero);
    RUN_TEST(test_ora_imm_negative);
    RUN_TEST(test_ora_zpg);
    RUN_TEST(test_ora_abs);
    RUN_TEST(test_ora_abs_x_page_cross);
    RUN_TEST(test_ora_ind_x);
    RUN_TEST(test_ora_ind_y);

    printf("\n--- EOR Tests ---\n");
    RUN_TEST(test_eor_imm_positive);
    RUN_TEST(test_eor_imm_zero);
    RUN_TEST(test_eor_imm_negative);
    RUN_TEST(test_eor_zpg);
    RUN_TEST(test_eor_abs);
    RUN_TEST(test_eor_abs_y_page_cross);
    RUN_TEST(test_eor_ind_x);
    RUN_TEST(test_eor_ind_y);

    printf("\n--- ASL Tests ---\n");
    RUN_TEST(test_asl_acc);
    RUN_TEST(test_asl_acc_carry);
    RUN_TEST(test_asl_acc_zero);
    RUN_TEST(test_asl_acc_negative);
    RUN_TEST(test_asl_zpg);
    RUN_TEST(test_asl_zpg_x);
    RUN_TEST(test_asl_abs);
    RUN_TEST(test_asl_abs_x);

    printf("\n--- ROL Tests ---\n");
    RUN_TEST(test_rol_acc_no_carry);
    RUN_TEST(test_rol_acc_with_carry);
    RUN_TEST(test_rol_acc_to_carry);
    RUN_TEST(test_rol_acc_carry_through);
    RUN_TEST(test_rol_zpg);
    RUN_TEST(test_rol_zpg_x);
    RUN_TEST(test_rol_abs);
    RUN_TEST(test_rol_abs_x);

    printf("\n--- LSR Tests ---\n");
    RUN_TEST(test_lsr_acc);
    RUN_TEST(test_lsr_acc_carry);
    RUN_TEST(test_lsr_acc_high_bit);
    RUN_TEST(test_lsr_acc_both);
    RUN_TEST(test_lsr_zpg);
    RUN_TEST(test_lsr_zpg_x);
    RUN_TEST(test_lsr_abs);
    RUN_TEST(test_lsr_abs_x);

    printf("\n--- ROR Tests ---\n");
    RUN_TEST(test_ror_acc_no_carry);
    RUN_TEST(test_ror_acc_with_carry);
    RUN_TEST(test_ror_acc_to_carry);
    RUN_TEST(test_ror_acc_carry_through);
    RUN_TEST(test_ror_zpg);
    RUN_TEST(test_ror_zpg_x);
    RUN_TEST(test_ror_abs);
    RUN_TEST(test_ror_abs_x);

    printf("\n--- BIT Tests ---\n");
    RUN_TEST(test_bit_zpg_z_set);
    RUN_TEST(test_bit_zpg_z_clear);
    RUN_TEST(test_bit_a_zero);
    RUN_TEST(test_bit_mem_zero);
    RUN_TEST(test_bit_n_set);
    RUN_TEST(test_bit_n_clear);
    RUN_TEST(test_bit_n_independent);
    RUN_TEST(test_bit_v_set);
    RUN_TEST(test_bit_v_clear);
    RUN_TEST(test_bit_v_independent);
    RUN_TEST(test_bit_a_unchanged);
    RUN_TEST(test_bit_abs);
    RUN_TEST(test_bit_all_flags_set);
    RUN_TEST(test_bit_no_flags_set);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
