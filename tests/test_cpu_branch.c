#include "test_common.h"
#include "bus.h"

/*
 * Conditional Branch Instruction Tests
 *
 * All branches use relative addressing (REL):
 *   - Opcode + signed 8-bit offset
 *   - Offset is relative to PC after fetching the instruction (PC + 2)
 *
 * Cycle counts:
 *   - Not taken: 2 cycles
 *   - Taken, no page cross: 3 cycles
 *   - Taken, page cross: 4 cycles
 */

/* ========================= BCC - Branch if Carry Clear ========================= */

TEST(test_bcc_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* Clear carry */

    bus_write(bus, 0x0200, 0x90);  /* BCC */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BCC taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_bcc_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* Clear carry */

    bus_write(bus, 0x0200, 0x90);  /* BCC */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BCC taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_bcc_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* Set carry */

    bus_write(bus, 0x0200, 0x90);  /* BCC */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BCC not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_bcc_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* Clear carry */

    /* Start near page boundary */
    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0x90);  /* BCC */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 = 0x0312 (crosses to page 0x03) */
    CHECK(cycles == 4, "BCC taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= BCS - Branch if Carry Set ========================= */

TEST(test_bcs_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* Set carry */

    bus_write(bus, 0x0200, 0xB0);  /* BCS */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BCS taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_bcs_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* Set carry */

    bus_write(bus, 0x0200, 0xB0);  /* BCS */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BCS taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_bcs_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_C);  /* Clear carry */

    bus_write(bus, 0x0200, 0xB0);  /* BCS */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BCS not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_bcs_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* Set carry */

    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0xB0);  /* BCS */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 */
    CHECK(cycles == 4, "BCS taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= BEQ - Branch if Equal (Z=1) ========================= */

TEST(test_beq_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z);  /* Set zero flag */

    bus_write(bus, 0x0200, 0xF0);  /* BEQ */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BEQ taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_beq_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z);  /* Set zero flag */

    bus_write(bus, 0x0200, 0xF0);  /* BEQ */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BEQ taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_beq_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_Z);  /* Clear zero flag */

    bus_write(bus, 0x0200, 0xF0);  /* BEQ */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BEQ not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_beq_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z);  /* Set zero flag */

    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0xF0);  /* BEQ */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 */
    CHECK(cycles == 4, "BEQ taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= BNE - Branch if Not Equal (Z=0) ========================= */

TEST(test_bne_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_Z);  /* Clear zero flag */

    bus_write(bus, 0x0200, 0xD0);  /* BNE */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BNE taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_bne_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_Z);  /* Clear zero flag */

    bus_write(bus, 0x0200, 0xD0);  /* BNE */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BNE taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_bne_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z);  /* Set zero flag */

    bus_write(bus, 0x0200, 0xD0);  /* BNE */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BNE not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_bne_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_Z);  /* Clear zero flag */

    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0xD0);  /* BNE */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 */
    CHECK(cycles == 4, "BNE taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= BMI - Branch if Minus (N=1) ========================= */

TEST(test_bmi_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_N);  /* Set negative flag */

    bus_write(bus, 0x0200, 0x30);  /* BMI */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BMI taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_bmi_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_N);  /* Set negative flag */

    bus_write(bus, 0x0200, 0x30);  /* BMI */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BMI taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_bmi_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_N);  /* Clear negative flag */

    bus_write(bus, 0x0200, 0x30);  /* BMI */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BMI not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_bmi_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_N);  /* Set negative flag */

    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0x30);  /* BMI */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 */
    CHECK(cycles == 4, "BMI taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= BPL - Branch if Plus (N=0) ========================= */

TEST(test_bpl_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_N);  /* Clear negative flag */

    bus_write(bus, 0x0200, 0x10);  /* BPL */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BPL taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_bpl_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_N);  /* Clear negative flag */

    bus_write(bus, 0x0200, 0x10);  /* BPL */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BPL taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_bpl_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_N);  /* Set negative flag */

    bus_write(bus, 0x0200, 0x10);  /* BPL */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BPL not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_bpl_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_N);  /* Clear negative flag */

    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0x10);  /* BPL */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 */
    CHECK(cycles == 4, "BPL taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= BVC - Branch if Overflow Clear (V=0) ========================= */

TEST(test_bvc_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_V);  /* Clear overflow flag */

    bus_write(bus, 0x0200, 0x50);  /* BVC */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BVC taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_bvc_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_V);  /* Clear overflow flag */

    bus_write(bus, 0x0200, 0x50);  /* BVC */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BVC taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_bvc_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_V);  /* Set overflow flag */

    bus_write(bus, 0x0200, 0x50);  /* BVC */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BVC not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_bvc_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_V);  /* Clear overflow flag */

    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0x50);  /* BVC */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 */
    CHECK(cycles == 4, "BVC taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= BVS - Branch if Overflow Set (V=1) ========================= */

TEST(test_bvs_taken_forward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_V);  /* Set overflow flag */

    bus_write(bus, 0x0200, 0x70);  /* BVS */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0207);  /* 0x0202 + 5 */
    CHECK(cycles == 3, "BVS taken, no page cross: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_bvs_taken_backward) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_V);  /* Set overflow flag */

    bus_write(bus, 0x0200, 0x70);  /* BVS */
    bus_write(bus, 0x0201, 0xFC);  /* offset -4 (signed) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x01FE);  /* 0x0202 - 4 */
    CHECK(cycles == 4, "BVS taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_bvs_not_taken) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_V);  /* Clear overflow flag */

    bus_write(bus, 0x0200, 0x70);  /* BVS */
    bus_write(bus, 0x0201, 0x05);  /* offset +5 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* No branch */
    CHECK(cycles == 2, "BVS not taken: 2 cycles");
    cpu_destroy(cpu);
}

TEST(test_bvs_page_cross) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_V);  /* Set overflow flag */

    cpu_set_pc(cpu, 0x02F0);
    bus_write(bus, 0x02F0, 0x70);  /* BVS */
    bus_write(bus, 0x02F1, 0x20);  /* offset +32 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0312);  /* 0x02F2 + 0x20 */
    CHECK(cycles == 4, "BVS taken, page cross: 4 cycles");
    cpu_destroy(cpu);
}

/* ========================= Edge Cases ========================= */

TEST(test_branch_offset_zero) {
    /* Branch to self - offset 0 means next instruction (PC+2), then +0 */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z);  /* Set zero flag */

    bus_write(bus, 0x0200, 0xF0);  /* BEQ */
    bus_write(bus, 0x0201, 0x00);  /* offset 0 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0202);  /* 0x0202 + 0 */
    CHECK(cycles == 3, "Branch with offset 0, taken: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_branch_max_forward) {
    /* Maximum forward offset: +127 */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z);  /* Set zero flag */

    bus_write(bus, 0x0200, 0xF0);  /* BEQ */
    bus_write(bus, 0x0201, 0x7F);  /* offset +127 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0281);  /* 0x0202 + 127 */
    CHECK(cycles == 3, "Branch with max forward offset: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_branch_max_backward) {
    /* Maximum backward offset: -128 */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_Z);  /* Set zero flag */

    cpu_set_pc(cpu, 0x0300);
    bus_write(bus, 0x0300, 0xF0);  /* BEQ */
    bus_write(bus, 0x0301, 0x80);  /* offset -128 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0282);  /* 0x0302 - 128 */
    CHECK(cycles == 4, "Branch with max backward offset, page cross: 4 cycles");
    cpu_destroy(cpu);
}

TEST(test_branch_no_page_cross_boundary) {
    /* Branch that stays within same page */
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);
    cpu_set_status(cpu, cpu_get_status(cpu) | FLAG_C);  /* Set carry */

    cpu_set_pc(cpu, 0x0250);
    bus_write(bus, 0x0250, 0xB0);  /* BCS */
    bus_write(bus, 0x0251, 0x10);  /* offset +16 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0262);  /* 0x0252 + 16, still in page 0x02 */
    CHECK(cycles == 3, "Branch within same page: 3 cycles");
    cpu_destroy(cpu);
}

/* ========================= Main ========================= */

int main(void) {
    reset_test_state();

    printf("=== Branch Instruction Tests ===\n\n");

    printf("--- BCC (Branch if Carry Clear) ---\n");
    RUN_TEST(test_bcc_taken_forward);
    RUN_TEST(test_bcc_taken_backward);
    RUN_TEST(test_bcc_not_taken);
    RUN_TEST(test_bcc_page_cross);

    printf("\n--- BCS (Branch if Carry Set) ---\n");
    RUN_TEST(test_bcs_taken_forward);
    RUN_TEST(test_bcs_taken_backward);
    RUN_TEST(test_bcs_not_taken);
    RUN_TEST(test_bcs_page_cross);

    printf("\n--- BEQ (Branch if Equal / Z=1) ---\n");
    RUN_TEST(test_beq_taken_forward);
    RUN_TEST(test_beq_taken_backward);
    RUN_TEST(test_beq_not_taken);
    RUN_TEST(test_beq_page_cross);

    printf("\n--- BNE (Branch if Not Equal / Z=0) ---\n");
    RUN_TEST(test_bne_taken_forward);
    RUN_TEST(test_bne_taken_backward);
    RUN_TEST(test_bne_not_taken);
    RUN_TEST(test_bne_page_cross);

    printf("\n--- BMI (Branch if Minus / N=1) ---\n");
    RUN_TEST(test_bmi_taken_forward);
    RUN_TEST(test_bmi_taken_backward);
    RUN_TEST(test_bmi_not_taken);
    RUN_TEST(test_bmi_page_cross);

    printf("\n--- BPL (Branch if Plus / N=0) ---\n");
    RUN_TEST(test_bpl_taken_forward);
    RUN_TEST(test_bpl_taken_backward);
    RUN_TEST(test_bpl_not_taken);
    RUN_TEST(test_bpl_page_cross);

    printf("\n--- BVC (Branch if Overflow Clear / V=0) ---\n");
    RUN_TEST(test_bvc_taken_forward);
    RUN_TEST(test_bvc_taken_backward);
    RUN_TEST(test_bvc_not_taken);
    RUN_TEST(test_bvc_page_cross);

    printf("\n--- BVS (Branch if Overflow Set / V=1) ---\n");
    RUN_TEST(test_bvs_taken_forward);
    RUN_TEST(test_bvs_taken_backward);
    RUN_TEST(test_bvs_not_taken);
    RUN_TEST(test_bvs_page_cross);

    printf("\n--- Edge Cases ---\n");
    RUN_TEST(test_branch_offset_zero);
    RUN_TEST(test_branch_max_forward);
    RUN_TEST(test_branch_max_backward);
    RUN_TEST(test_branch_no_page_cross_boundary);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
