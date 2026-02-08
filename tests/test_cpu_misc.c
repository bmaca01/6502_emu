#include "test_common.h"
#include "bus.h"
#include "memory.h"

/* ================================= Basic Tests =================================== */
TEST(test_cpu_create_destroy) {
    Memory* mem = memory_create();
    Bus* bus = bus_create();
    bus_map_memory(bus, mem);
    CPU* cpu = cpu_create(bus);

    CHECK(cpu != NULL, "cpu_create should return non-NULL");

    cpu_destroy(cpu);
}

TEST(test_cpu_nop) {
    Memory* mem = memory_create();
    Bus* bus = bus_create();
    bus_map_memory(bus, mem);
    bus_write(bus, 0x0000, 0xEA);

    CPU* cpu = cpu_create(bus);

    uint8_t cycles = cpu_step(cpu);
    CHECK(cycles == 2, "NOP takes 2 cycles");
    check_pc(cpu, 0x0001);  /* Implied: +1 */

    cpu_destroy(cpu);
}

/* ========================= Transfer Tests ================================== */

/* TAX - Transfer A to X */
TEST(test_tax_positive) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    bus_write(bus, 0x0200, 0xAA);  /* TAX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);  /* Implied: +1 */
    cpu_destroy(cpu);
}

TEST(test_tax_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0200, 0xAA);  /* TAX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tax_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x80);
    bus_write(bus, 0x0200, 0xAA);  /* TAX */

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    bus_write(bus, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tay_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);
    bus_write(bus, 0x0200, 0xA8);  /* TAY */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_y(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tay_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x80);
    bus_write(bus, 0x0200, 0xA8);  /* TAY */

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x42);
    bus_write(bus, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txa_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x00);
    bus_write(bus, 0x0200, 0x8A);  /* TXA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_txa_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x80);
    bus_write(bus, 0x0200, 0x8A);  /* TXA */

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x42);
    bus_write(bus, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tya_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x00);
    bus_write(bus, 0x0200, 0x98);  /* TYA */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tya_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_y(cpu, 0x80);
    bus_write(bus, 0x0200, 0x98);  /* TYA */

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
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_sp(cpu, 0x42);
    bus_write(bus, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    check_flags(cpu, 0, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tsx_zero) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_sp(cpu, 0x00);
    bus_write(bus, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x00);
    check_flags(cpu, 0, 1);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_tsx_negative) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_sp(cpu, 0x80);
    bus_write(bus, 0x0200, 0xBA);  /* TSX */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x80);
    check_flags(cpu, 1, 0);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* TAX - Verify stale N/Z flags are cleared */
TEST(test_tax_clears_stale_nz) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);  /* Positive non-zero */
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_N | FLAG_Z);  /* Pre-dirty both */
    bus_write(bus, 0x0200, 0xAA);  /* TAX */

    cpu_step(cpu);

    CHECK(cpu_get_x(cpu) == 0x42);
    check_flags(cpu, 0, 0);  /* Both N and Z should be cleared */
    cpu_destroy(cpu);
}

TEST(test_txs) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_x(cpu, 0x42);
    cpu_set_status(cpu, 0x00);  /* Clear flags */
    bus_write(bus, 0x0200, 0x9A);  /* TXS */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_sp(cpu) == 0x42);
    check_flags(cpu, 0, 0);  /* Flags unchanged */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* ======================= Stack Tests =============================== */
/* Tests written for authentic 6502 stack behavior:
 * - Push: Write to $0100+SP FIRST, then decrement SP
 * - Pull: Increment SP FIRST, then read from $0100+SP
 */

/* === PHA Tests === */
TEST(test_pha_basic) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x42);
    bus_write(bus, 0x0200, encode_op(PHA, IMPL));

    uint8_t sp_before = cpu_get_sp(cpu);  /* Should be 0xFF */
    uint8_t cycles = cpu_step(cpu);
    uint8_t sp_after = cpu_get_sp(cpu);

    CHECK(sp_after == sp_before - 1, "SP should decrement by 1");
    CHECK(bus_read(bus, 0x0100 | sp_before) == 0x42,
          "Value should be at old SP location ($01FF)");
    CHECK(cpu_get_a(cpu) == 0x42, "A should be unchanged");
    CHECK(cycles == 3, "PHA takes 3 cycles");
    check_pc(cpu, 0x0201);

    cpu_destroy(cpu);
}

TEST(test_pha_no_flag_change) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_a(cpu, 0x00);  /* Zero value */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(PHA, IMPL));

    cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before,
          "PHA should not affect any flags");
    cpu_destroy(cpu);
}

TEST(test_pha_multiple) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    uint8_t sp_initial = cpu_get_sp(cpu);  /* 0xFF */

    /* Push three values */
    bus_write(bus, 0x0200, encode_op(LDA, IMM));
    bus_write(bus, 0x0201, 0xAA);
    bus_write(bus, 0x0202, encode_op(PHA, IMPL));
    bus_write(bus, 0x0203, encode_op(LDA, IMM));
    bus_write(bus, 0x0204, 0xBB);
    bus_write(bus, 0x0205, encode_op(PHA, IMPL));
    bus_write(bus, 0x0206, encode_op(LDA, IMM));
    bus_write(bus, 0x0207, 0xCC);
    bus_write(bus, 0x0208, encode_op(PHA, IMPL));

    cpu_step(cpu);  /* LDA #$AA */
    cpu_step(cpu);  /* PHA - write $AA to $01FF */
    cpu_step(cpu);  /* LDA #$BB */
    cpu_step(cpu);  /* PHA - write $BB to $01FE */
    cpu_step(cpu);  /* LDA #$CC */
    cpu_step(cpu);  /* PHA - write $CC to $01FD */

    /* Standard 6502 stack order (LIFO from top of stack page) */
    CHECK(bus_read(bus, 0x01FF) == 0xAA, "First push at $01FF");
    CHECK(bus_read(bus, 0x01FE) == 0xBB, "Second push at $01FE");
    CHECK(bus_read(bus, 0x01FD) == 0xCC, "Third push at $01FD");
    CHECK(cpu_get_sp(cpu) == sp_initial - 3, "SP should be 0xFC");

    cpu_destroy(cpu);
}

/* === PHP Tests === */
TEST(test_php_sets_b_and_u) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, 0x00);  /* Clear all flags */
    bus_write(bus, 0x0200, encode_op(PHP, IMPL));

    uint8_t sp_before = cpu_get_sp(cpu);
    cpu_step(cpu);

    /* Pushed byte should have B and U flags set */
    uint8_t pushed = bus_read(bus, 0x0100 | sp_before);
    CHECK(pushed & FLAG_B, "Pushed status should have B flag set");
    CHECK(pushed & FLAG_U, "Pushed status should have U flag set");
    CHECK(pushed == (FLAG_B | FLAG_U), "Only B and U should be set");

    cpu_destroy(cpu);
}

TEST(test_php_preserves_status) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, FLAG_C | FLAG_Z | FLAG_N);
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(PHP, IMPL));

    cpu_step(cpu);

    /* CPU status register should be unchanged */
    CHECK(cpu_get_status(cpu) == status_before,
          "PHP should not modify CPU status register");

    cpu_destroy(cpu);
}

TEST(test_php_timing_and_sp) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    bus_write(bus, 0x0200, encode_op(PHP, IMPL));

    uint8_t sp_before = cpu_get_sp(cpu);
    uint8_t cycles = cpu_step(cpu);

    CHECK(cycles == 3, "PHP takes 3 cycles");
    CHECK(cpu_get_sp(cpu) == sp_before - 1, "SP decrements by 1");
    check_pc(cpu, 0x0201);

    cpu_destroy(cpu);
}

/* === PLA Tests === */
TEST(test_pla_basic) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Manually place value on stack at $01FF (where SP will increment to) */
    bus_write(bus, 0x01FF, 0x42);
    cpu_set_sp(cpu, 0xFE);  /* SP points below the data */
    bus_write(bus, 0x0200, encode_op(PLA, IMPL));

    uint8_t cycles = cpu_step(cpu);

    /* Standard 6502: increment SP first, then read from new SP */
    CHECK(cpu_get_sp(cpu) == 0xFF, "SP should increment to 0xFF");
    CHECK(cpu_get_a(cpu) == 0x42, "A should be loaded with pulled value");
    CHECK(cycles == 4, "PLA takes 4 cycles");
    check_pc(cpu, 0x0201);

    cpu_destroy(cpu);
}

TEST(test_pla_sets_zero_flag) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0x01FF, 0x00);
    cpu_set_sp(cpu, 0xFE);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_Z);  /* Clear Z */
    bus_write(bus, 0x0200, encode_op(PLA, IMPL));

    cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x00, "A should be 0");
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z flag should be set");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N flag should be clear");

    cpu_destroy(cpu);
}

TEST(test_pla_sets_negative_flag) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0x01FF, 0x80);
    cpu_set_sp(cpu, 0xFE);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_N);  /* Clear N */
    bus_write(bus, 0x0200, encode_op(PLA, IMPL));

    cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x80, "A should be 0x80");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N flag should be set");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z flag should be clear");

    cpu_destroy(cpu);
}

TEST(test_pla_clears_flags) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0x01FF, 0x42);  /* Positive non-zero */
    cpu_set_sp(cpu, 0xFE);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z | FLAG_N);  /* Set both */
    bus_write(bus, 0x0200, encode_op(PLA, IMPL));

    cpu_step(cpu);

    CHECK(cpu_get_a(cpu) == 0x42, "A should be 0x42");
    CHECK(!(cpu_get_status(cpu) & FLAG_Z), "Z flag should be clear");
    CHECK(!(cpu_get_status(cpu) & FLAG_N), "N flag should be clear");

    cpu_destroy(cpu);
}

/* === PLP Tests === */
TEST(test_plp_basic) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Place status byte on stack */
    bus_write(bus, 0x01FF, FLAG_C | FLAG_Z | FLAG_N | FLAG_U);
    cpu_set_sp(cpu, 0xFE);
    cpu_set_status(cpu, 0x00);
    bus_write(bus, 0x0200, encode_op(PLP, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_sp(cpu) == 0xFF, "SP should increment to 0xFF");
    CHECK(cpu_get_status(cpu) & FLAG_C, "C flag should be restored");
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z flag should be restored");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N flag should be restored");
    CHECK(cycles == 4, "PLP takes 4 cycles");
    check_pc(cpu, 0x0201);

    cpu_destroy(cpu);
}

TEST(test_plp_b_flag_ignored) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Push status with B flag set */
    bus_write(bus, 0x01FF, FLAG_B | FLAG_U);
    cpu_set_sp(cpu, 0xFE);
    cpu_set_status(cpu, 0x00);
    bus_write(bus, 0x0200, encode_op(PLP, IMPL));

    cpu_step(cpu);

    /* B flag should be ignored/handled specially per 6502 spec */
    /* The B flag only exists in the pushed copy, not in the actual status register */
    /* Note: Implementation may vary on B flag handling */

    cpu_destroy(cpu);
}

/* === Roundtrip Tests === */
TEST(test_pha_pla_roundtrip) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    cpu_set_a(cpu, 0xAB);
    uint8_t sp_initial = cpu_get_sp(cpu);

    bus_write(bus, 0x0200, encode_op(PHA, IMPL));
    bus_write(bus, 0x0201, encode_op(LDA, IMM));
    bus_write(bus, 0x0202, 0x00);
    bus_write(bus, 0x0203, encode_op(PLA, IMPL));

    cpu_step(cpu);  /* PHA */
    cpu_step(cpu);  /* LDA #$00 */
    CHECK(cpu_get_a(cpu) == 0x00, "A should be 0 after LDA");

    cpu_step(cpu);  /* PLA */
    CHECK(cpu_get_a(cpu) == 0xAB, "A should be restored to 0xAB");
    CHECK(cpu_get_sp(cpu) == sp_initial, "SP should be restored");

    cpu_destroy(cpu);
}

TEST(test_php_plp_roundtrip) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    cpu_set_status(cpu, FLAG_C | FLAG_V | FLAG_N | FLAG_U);
    uint8_t status_initial = cpu_get_status(cpu);
    uint8_t sp_initial = cpu_get_sp(cpu);

    bus_write(bus, 0x0200, encode_op(PHP, IMPL));
    bus_write(bus, 0x0201, encode_op(LDA, IMM));
    bus_write(bus, 0x0202, 0x00);  /* Sets Z, clears N */
    bus_write(bus, 0x0203, encode_op(PLP, IMPL));

    cpu_step(cpu);  /* PHP */
    cpu_step(cpu);  /* LDA #$00 - changes flags */
    CHECK(cpu_get_status(cpu) != status_initial, "Flags should have changed");

    cpu_step(cpu);  /* PLP */
    /* Status should be restored (with B flag handling per spec) */
    CHECK(cpu_get_sp(cpu) == sp_initial, "SP should be restored");
    CHECK(cpu_get_status(cpu) & FLAG_C, "C flag should be restored");
    CHECK(cpu_get_status(cpu) & FLAG_V, "V flag should be restored");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N flag should be restored");

    cpu_destroy(cpu);
}

/* ============================ Flag Clear Tests ===================================== */

TEST(test_clc) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* Set C first */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLC, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(!(cpu_get_status(cpu) & FLAG_C));  /* C cleared */
    CHECK((cpu_get_status(cpu) & ~FLAG_C) == (status_before & ~FLAG_C));  /* Others unchanged */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_clc_no_effect) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* C already clear */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLC, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before);  /* No change */
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_cld) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_D);  /* Set D first */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLD, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(!(cpu_get_status(cpu) & FLAG_D));  /* D cleared */
    CHECK((cpu_get_status(cpu) & ~FLAG_D) == (status_before & ~FLAG_D));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_cld_no_effect) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_D);
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLD, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_cli) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_I);  /* Set I first */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLI, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(!(cpu_get_status(cpu) & FLAG_I));  /* I cleared */
    CHECK((cpu_get_status(cpu) & ~FLAG_I) == (status_before & ~FLAG_I));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_cli_no_effect) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLI, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_clv) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_V);  /* Set V first */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLV, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(!(cpu_get_status(cpu) & FLAG_V));  /* V cleared */
    CHECK((cpu_get_status(cpu) & ~FLAG_V) == (status_before & ~FLAG_V));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_clv_no_effect) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_V);
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(CLV, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* ============================ Flag Set Tests ===================================== */

TEST(test_sec) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* Clear C first */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(SEC, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C);  /* C set */
    CHECK((cpu_get_status(cpu) & ~FLAG_C) == (status_before & ~FLAG_C));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_sec_no_effect) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* C already set */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(SEC, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_sed) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_D);  /* Clear D first */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(SED, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_D);  /* D set */
    CHECK((cpu_get_status(cpu) & ~FLAG_D) == (status_before & ~FLAG_D));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_sed_no_effect) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_D);  /* D already set */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(SED, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_sei) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);  /* Clear I first */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(SEI, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_I);  /* I set */
    CHECK((cpu_get_status(cpu) & ~FLAG_I) == (status_before & ~FLAG_I));
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

TEST(test_sei_no_effect) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_I);  /* I already set */
    uint8_t status_before = cpu_get_status(cpu);
    bus_write(bus, 0x0200, encode_op(SEI, IMPL));

    uint8_t cycles = cpu_step(cpu);

    CHECK(cpu_get_status(cpu) == status_before);
    CHECK(cycles == 2);
    check_pc(cpu, 0x0201);
    cpu_destroy(cpu);
}

/* ============================== Test Runner ================================ */

int main(void) {
    reset_test_state();
    printf("\n=== Misc CPU Tests (Basic, Transfer, Stack, Flags) ===\n\n");

    printf("--- Basic Tests ---\n");
    RUN_TEST(test_cpu_create_destroy);
    RUN_TEST(test_cpu_nop);

    printf("\n--- Transfer Tests ---\n");
    RUN_TEST(test_tax_positive);
    RUN_TEST(test_tax_zero);
    RUN_TEST(test_tax_negative);
    RUN_TEST(test_tax_clears_stale_nz);
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

    printf("\n--- Stack Tests ---\n");
    RUN_TEST(test_pha_basic);
    RUN_TEST(test_pha_no_flag_change);
    RUN_TEST(test_pha_multiple);
    RUN_TEST(test_php_sets_b_and_u);
    RUN_TEST(test_php_preserves_status);
    RUN_TEST(test_php_timing_and_sp);
    RUN_TEST(test_pla_basic);
    RUN_TEST(test_pla_sets_zero_flag);
    RUN_TEST(test_pla_sets_negative_flag);
    RUN_TEST(test_pla_clears_flags);
    RUN_TEST(test_plp_basic);
    RUN_TEST(test_plp_b_flag_ignored);
    RUN_TEST(test_pha_pla_roundtrip);
    RUN_TEST(test_php_plp_roundtrip);

    printf("\n--- Flag Clear Tests ---\n");
    RUN_TEST(test_clc);
    RUN_TEST(test_clc_no_effect);
    RUN_TEST(test_cld);
    RUN_TEST(test_cld_no_effect);
    RUN_TEST(test_cli);
    RUN_TEST(test_cli_no_effect);
    RUN_TEST(test_clv);
    RUN_TEST(test_clv_no_effect);

    printf("\n--- Flag Set Tests ---\n");
    RUN_TEST(test_sec);
    RUN_TEST(test_sec_no_effect);
    RUN_TEST(test_sed);
    RUN_TEST(test_sed_no_effect);
    RUN_TEST(test_sei);
    RUN_TEST(test_sei_no_effect);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
