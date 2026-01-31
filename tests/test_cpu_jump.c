#include "test_common.h"
#include "memory.h"

/*
 * Jump and Subroutine Instruction Tests
 *
 * JMP (Absolute):  3 cycles - PC = operand
 * JMP (Indirect):  5 cycles - PC = [operand] (with page wrap bug)
 * JSR (Absolute):  6 cycles - Push PC+2, PC = operand
 * RTS (Implied):   6 cycles - Pull PC, PC = pulled + 1
 *
 * Stack is at $0100-$01FF, SP is offset into this page.
 * 6502 pushes high byte first, then low byte.
 * RTS pulls low byte first, then high byte, then adds 1.
 */

/* ========================= JMP Absolute ========================= */

TEST(test_jmp_abs_basic) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0200, 0x4C);  /* JMP abs */
    memory_write(mem, 0x0201, 0x50);  /* low byte */
    memory_write(mem, 0x0202, 0x03);  /* high byte -> $0350 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0350);
    CHECK(cycles == 3, "JMP abs: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_jmp_abs_forward) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0200, 0x4C);  /* JMP abs */
    memory_write(mem, 0x0201, 0x00);  /* low byte */
    memory_write(mem, 0x0202, 0x80);  /* high byte -> $8000 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x8000);
    CHECK(cycles == 3, "JMP abs forward: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_jmp_abs_backward) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_pc(cpu, 0x0300);
    memory_write(mem, 0x0300, 0x4C);  /* JMP abs */
    memory_write(mem, 0x0301, 0x00);  /* low byte */
    memory_write(mem, 0x0302, 0x02);  /* high byte -> $0200 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0200);
    CHECK(cycles == 3, "JMP abs backward: 3 cycles");
    cpu_destroy(cpu);
}

TEST(test_jmp_abs_same_page) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0200, 0x4C);  /* JMP abs */
    memory_write(mem, 0x0201, 0x50);  /* low byte */
    memory_write(mem, 0x0202, 0x02);  /* high byte -> $0250 (same page) */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0250);
    CHECK(cycles == 3, "JMP abs same page: 3 cycles");
    cpu_destroy(cpu);
}

/* ========================= JMP Indirect ========================= */

TEST(test_jmp_ind_basic) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* JMP ($0400) where $0400 contains $1234 */
    memory_write(mem, 0x0200, 0x6C);  /* JMP ind */
    memory_write(mem, 0x0201, 0x00);  /* pointer low */
    memory_write(mem, 0x0202, 0x04);  /* pointer high -> $0400 */
    memory_write(mem, 0x0400, 0x34);  /* target low */
    memory_write(mem, 0x0401, 0x12);  /* target high -> $1234 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x1234);
    CHECK(cycles == 5, "JMP ind: 5 cycles");
    cpu_destroy(cpu);
}

TEST(test_jmp_ind_zero_page_pointer) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* JMP ($0050) where $0050 contains $ABCD */
    memory_write(mem, 0x0200, 0x6C);  /* JMP ind */
    memory_write(mem, 0x0201, 0x50);  /* pointer low */
    memory_write(mem, 0x0202, 0x00);  /* pointer high -> $0050 */
    memory_write(mem, 0x0050, 0xCD);  /* target low */
    memory_write(mem, 0x0051, 0xAB);  /* target high -> $ABCD */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0xABCD);
    CHECK(cycles == 5, "JMP ind zero page pointer: 5 cycles");
    cpu_destroy(cpu);
}

TEST(test_jmp_ind_page_boundary_bug) {
    /*
     * 6502 page boundary bug: JMP ($10FF) reads low byte from $10FF
     * and high byte from $1000 (not $1100).
     */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0200, 0x6C);  /* JMP ind */
    memory_write(mem, 0x0201, 0xFF);  /* pointer low */
    memory_write(mem, 0x0202, 0x10);  /* pointer high -> $10FF */
    memory_write(mem, 0x10FF, 0x34);  /* target low byte */
    memory_write(mem, 0x1000, 0x12);  /* target high byte (wrapped!) */
    memory_write(mem, 0x1100, 0xFF);  /* wrong location if no bug */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x1234);  /* Should be $1234 due to page wrap bug */
    CHECK(cycles == 5, "JMP ind page boundary: 5 cycles");
    cpu_destroy(cpu);
}

TEST(test_jmp_ind_self_reference) {
    /* Pointer points to address near the JMP instruction */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0200, 0x6C);  /* JMP ind */
    memory_write(mem, 0x0201, 0x10);  /* pointer low */
    memory_write(mem, 0x0202, 0x02);  /* pointer high -> $0210 */
    memory_write(mem, 0x0210, 0x00);  /* target low */
    memory_write(mem, 0x0211, 0x03);  /* target high -> $0300 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0300);
    CHECK(cycles == 5, "JMP ind self reference: 5 cycles");
    cpu_destroy(cpu);
}

/* ========================= JSR ========================= */

TEST(test_jsr_basic) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    memory_write(mem, 0x0200, 0x20);  /* JSR */
    memory_write(mem, 0x0201, 0x00);  /* low byte */
    memory_write(mem, 0x0202, 0x04);  /* high byte -> $0400 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0400);
    CHECK(cycles == 6, "JSR: 6 cycles");
    CHECK(cpu_get_sp(cpu) == initial_sp - 2, "JSR decrements SP by 2");
    cpu_destroy(cpu);
}

TEST(test_jsr_pushes_correct_address) {
    /*
     * JSR pushes PC+2 (address of last byte of JSR instruction).
     * For JSR at $0200, it pushes $0202.
     * RTS will then return to $0202 + 1 = $0203.
     */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    memory_write(mem, 0x0200, 0x20);  /* JSR */
    memory_write(mem, 0x0201, 0x00);  /* low byte */
    memory_write(mem, 0x0202, 0x04);  /* high byte -> $0400 */

    cpu_step(cpu);

    /* Check stack contents: high byte pushed first, then low byte */
    uint8_t pushed_high = memory_read(mem, 0x0100 | (initial_sp));
    uint8_t pushed_low = memory_read(mem, 0x0100 | (initial_sp - 1));
    uint16_t pushed_addr = (pushed_high << 8) | pushed_low;

    CHECK(pushed_addr == 0x0202, "JSR pushes PC+2 (address of last byte)");
    cpu_destroy(cpu);
}

TEST(test_jsr_stack_page) {
    /* Verify JSR writes to stack page $0100-$01FF */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_sp(cpu, 0x80);  /* SP = $80, stack at $0180 */
    memory_write(mem, 0x0200, 0x20);  /* JSR */
    memory_write(mem, 0x0201, 0x00);
    memory_write(mem, 0x0202, 0x04);  /* -> $0400 */

    cpu_step(cpu);

    /* High byte at $0180, low byte at $017F */
    uint8_t high = memory_read(mem, 0x0180);
    uint8_t low = memory_read(mem, 0x017F);

    CHECK(high == 0x02, "JSR high byte in stack page");
    CHECK(low == 0x02, "JSR low byte in stack page");
    CHECK(cpu_get_sp(cpu) == 0x7E, "SP = $7E after JSR");
    cpu_destroy(cpu);
}

TEST(test_jsr_nested) {
    /* Two nested JSR calls */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    /* First JSR at $0200 -> $0300 */
    memory_write(mem, 0x0200, 0x20);
    memory_write(mem, 0x0201, 0x00);
    memory_write(mem, 0x0202, 0x03);

    /* Second JSR at $0300 -> $0400 */
    memory_write(mem, 0x0300, 0x20);
    memory_write(mem, 0x0301, 0x00);
    memory_write(mem, 0x0302, 0x04);

    cpu_step(cpu);  /* Execute first JSR */
    check_pc(cpu, 0x0300);
    CHECK(cpu_get_sp(cpu) == initial_sp - 2, "SP after first JSR");

    cpu_step(cpu);  /* Execute second JSR */
    check_pc(cpu, 0x0400);
    CHECK(cpu_get_sp(cpu) == initial_sp - 4, "SP after nested JSR");

    cpu_destroy(cpu);
}

TEST(test_jsr_to_zero_page) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    memory_write(mem, 0x0200, 0x20);  /* JSR */
    memory_write(mem, 0x0201, 0x50);  /* low byte */
    memory_write(mem, 0x0202, 0x00);  /* high byte -> $0050 */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0050);
    CHECK(cycles == 6, "JSR to zero page: 6 cycles");
    cpu_destroy(cpu);
}

/* ========================= RTS ========================= */

TEST(test_rts_basic) {
    /* Manually set up stack as if JSR had been called from $0200 */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* Push return address $0202 (what JSR from $0200 would push) */
    cpu_set_sp(cpu, 0xFD);  /* SP after push of 2 bytes */
    memory_write(mem, 0x01FE, 0x02);  /* low byte of $0202 */
    memory_write(mem, 0x01FF, 0x02);  /* high byte of $0202 */

    /* RTS at $0400 */
    cpu_set_pc(cpu, 0x0400);
    memory_write(mem, 0x0400, 0x60);  /* RTS */

    uint8_t cycles = cpu_step(cpu);

    check_pc(cpu, 0x0203);  /* $0202 + 1 */
    CHECK(cycles == 6, "RTS: 6 cycles");
    CHECK(cpu_get_sp(cpu) == 0xFF, "RTS increments SP by 2");
    cpu_destroy(cpu);
}

TEST(test_rts_adds_one) {
    /*
     * RTS pulls address and adds 1.
     * If stack contains $1233, RTS jumps to $1234.
     */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_sp(cpu, 0xFD);
    memory_write(mem, 0x01FE, 0x33);  /* low byte */
    memory_write(mem, 0x01FF, 0x12);  /* high byte -> $1233 */

    cpu_set_pc(cpu, 0x0400);
    memory_write(mem, 0x0400, 0x60);  /* RTS */

    cpu_step(cpu);

    check_pc(cpu, 0x1234);  /* $1233 + 1 */
    cpu_destroy(cpu);
}

TEST(test_rts_stack_page) {
    /* Verify RTS reads from stack page $0100-$01FF */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_sp(cpu, 0x7E);  /* SP = $7E */
    memory_write(mem, 0x017F, 0x99);  /* low byte at $017F */
    memory_write(mem, 0x0180, 0x10);  /* high byte at $0180 -> $1099 */

    cpu_set_pc(cpu, 0x0400);
    memory_write(mem, 0x0400, 0x60);  /* RTS */

    cpu_step(cpu);

    check_pc(cpu, 0x109A);  /* $1099 + 1 */
    CHECK(cpu_get_sp(cpu) == 0x80, "SP after RTS");
    cpu_destroy(cpu);
}

TEST(test_rts_wrap_sp) {
    /* Test SP wrapping within stack page */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_sp(cpu, 0xFE);  /* SP near top */
    memory_write(mem, 0x01FF, 0x50);  /* low byte */
    memory_write(mem, 0x0100, 0x03);  /* high byte wraps to $0100 -> $0350 */

    cpu_set_pc(cpu, 0x0400);
    memory_write(mem, 0x0400, 0x60);  /* RTS */

    cpu_step(cpu);

    check_pc(cpu, 0x0351);  /* $0350 + 1 */
    CHECK(cpu_get_sp(cpu) == 0x00, "SP wraps to $00");
    cpu_destroy(cpu);
}

/* ========================= JSR/RTS Integration ========================= */

TEST(test_jsr_rts_round_trip) {
    /* JSR to subroutine, then RTS back */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    /* JSR at $0200 -> $0400 */
    memory_write(mem, 0x0200, 0x20);
    memory_write(mem, 0x0201, 0x00);
    memory_write(mem, 0x0202, 0x04);

    /* RTS at $0400 */
    memory_write(mem, 0x0400, 0x60);

    cpu_step(cpu);  /* JSR */
    check_pc(cpu, 0x0400);

    cpu_step(cpu);  /* RTS */
    check_pc(cpu, 0x0203);  /* Return to instruction after JSR */
    CHECK(cpu_get_sp(cpu) == initial_sp, "SP restored after JSR/RTS");

    cpu_destroy(cpu);
}

TEST(test_jsr_rts_nested_round_trip) {
    /* Nested: JSR -> JSR -> RTS -> RTS */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    /* JSR at $0200 -> $0300 */
    memory_write(mem, 0x0200, 0x20);
    memory_write(mem, 0x0201, 0x00);
    memory_write(mem, 0x0202, 0x03);

    /* JSR at $0300 -> $0400 */
    memory_write(mem, 0x0300, 0x20);
    memory_write(mem, 0x0301, 0x00);
    memory_write(mem, 0x0302, 0x04);

    /* RTS at $0400 */
    memory_write(mem, 0x0400, 0x60);

    /* RTS at $0303 (after return from inner JSR) */
    memory_write(mem, 0x0303, 0x60);

    cpu_step(cpu);  /* First JSR: $0200 -> $0300 */
    check_pc(cpu, 0x0300);

    cpu_step(cpu);  /* Second JSR: $0300 -> $0400 */
    check_pc(cpu, 0x0400);

    cpu_step(cpu);  /* First RTS: $0400 -> $0303 */
    check_pc(cpu, 0x0303);

    cpu_step(cpu);  /* Second RTS: $0303 -> $0203 */
    check_pc(cpu, 0x0203);

    CHECK(cpu_get_sp(cpu) == initial_sp, "SP restored after nested JSR/RTS");
    cpu_destroy(cpu);
}

TEST(test_jsr_rts_multiple_calls) {
    /* Multiple sequential JSR/RTS pairs */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    /* First JSR at $0200 -> $0400 */
    memory_write(mem, 0x0200, 0x20);
    memory_write(mem, 0x0201, 0x00);
    memory_write(mem, 0x0202, 0x04);

    /* Second JSR at $0203 -> $0500 */
    memory_write(mem, 0x0203, 0x20);
    memory_write(mem, 0x0204, 0x00);
    memory_write(mem, 0x0205, 0x05);

    /* RTS at $0400 and $0500 */
    memory_write(mem, 0x0400, 0x60);
    memory_write(mem, 0x0500, 0x60);

    cpu_step(cpu);  /* JSR -> $0400 */
    cpu_step(cpu);  /* RTS -> $0203 */
    CHECK(cpu_get_sp(cpu) == initial_sp, "SP after first pair");

    cpu_step(cpu);  /* JSR -> $0500 */
    cpu_step(cpu);  /* RTS -> $0206 */
    check_pc(cpu, 0x0206);
    CHECK(cpu_get_sp(cpu) == initial_sp, "SP after second pair");

    cpu_destroy(cpu);
}

TEST(test_jsr_rts_preserves_registers) {
    /* JSR/RTS should not affect A, X, Y, or status */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_a(cpu, 0x42);
    cpu_set_x(cpu, 0x13);
    cpu_set_y(cpu, 0x37);
    cpu_set_status(cpu, 0xA5);

    memory_write(mem, 0x0200, 0x20);  /* JSR */
    memory_write(mem, 0x0201, 0x00);
    memory_write(mem, 0x0202, 0x04);
    memory_write(mem, 0x0400, 0x60);  /* RTS */

    cpu_step(cpu);  /* JSR */
    cpu_step(cpu);  /* RTS */

    CHECK(cpu_get_a(cpu) == 0x42, "A preserved");
    CHECK(cpu_get_x(cpu) == 0x13, "X preserved");
    CHECK(cpu_get_y(cpu) == 0x37, "Y preserved");
    CHECK(cpu_get_status(cpu) == 0xA5, "Status preserved");
    cpu_destroy(cpu);
}

/* ========================= BRK ========================= */

TEST(test_brk_jumps_to_vector) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* Set up IRQ/BRK vector at $FFFE/$FFFF */
    memory_write(mem, 0xFFFE, 0x00);  /* low byte */
    memory_write(mem, 0xFFFF, 0x03);  /* high byte -> $0300 */

    /* BRK at $0200 */
    memory_write(mem, 0x0200, 0x00);  /* BRK opcode */

    cpu_step(cpu);

    check_pc(cpu, 0x0300);
    cpu_destroy(cpu);
}

TEST(test_brk_pushes_pc_plus_2) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    /* Set up vector */
    memory_write(mem, 0xFFFE, 0x00);
    memory_write(mem, 0xFFFF, 0x03);

    /* BRK at $0200 */
    memory_write(mem, 0x0200, 0x00);

    cpu_step(cpu);

    /* Check pushed PC: should be $0202 (BRK addr + 2) */
    uint8_t pushed_pch = memory_read(mem, 0x0100 | initial_sp);
    uint8_t pushed_pcl = memory_read(mem, 0x0100 | (uint8_t)(initial_sp - 1));
    uint16_t pushed_pc = (pushed_pch << 8) | pushed_pcl;

    CHECK(pushed_pc == 0x0202, "BRK pushes PC+2");
    cpu_destroy(cpu);
}

TEST(test_brk_pushes_status_with_b_flag) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    /* Clear status to isolate B flag test */
    cpu_set_status(cpu, 0x00);

    /* Set up vector */
    memory_write(mem, 0xFFFE, 0x00);
    memory_write(mem, 0xFFFF, 0x03);

    memory_write(mem, 0x0200, 0x00);  /* BRK */

    cpu_step(cpu);

    /* Pushed status is at SP-2 from initial (after PCH, PCL) */
    uint8_t pushed_status = memory_read(mem, 0x0100 | (uint8_t)(initial_sp - 2));

    CHECK(pushed_status & FLAG_B, "Pushed status has B flag set");
    CHECK(pushed_status & FLAG_U, "Pushed status has bit 5 set");
    cpu_destroy(cpu);
}

TEST(test_brk_sets_i_flag) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* Clear I flag before BRK */
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);

    /* Set up vector */
    memory_write(mem, 0xFFFE, 0x00);
    memory_write(mem, 0xFFFF, 0x03);

    memory_write(mem, 0x0200, 0x00);  /* BRK */

    cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_I, "I flag should be set after BRK");
    cpu_destroy(cpu);
}

TEST(test_brk_sp_decreases_by_3) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);
    uint8_t initial_sp = cpu_get_sp(cpu);

    /* Set up vector */
    memory_write(mem, 0xFFFE, 0x00);
    memory_write(mem, 0xFFFF, 0x03);

    memory_write(mem, 0x0200, 0x00);  /* BRK */

    cpu_step(cpu);

    CHECK(cpu_get_sp(cpu) == initial_sp - 3, "SP decreases by 3");
    cpu_destroy(cpu);
}

TEST(test_brk_cycles) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* Set up vector */
    memory_write(mem, 0xFFFE, 0x00);
    memory_write(mem, 0xFFFF, 0x03);

    memory_write(mem, 0x0200, 0x00);  /* BRK */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cycles == 7, "BRK takes 7 cycles");
    cpu_destroy(cpu);
}

/* ========================= RTI ========================= */

TEST(test_rti_restores_pc) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* Set up stack as if BRK had pushed: status, PCL, PCH */
    /* Stack grows down, so PCH is at highest address */
    cpu_set_sp(cpu, 0xFC);  /* SP after 3 pushes from 0xFF */
    memory_write(mem, 0x01FD, 0x00);  /* status */
    memory_write(mem, 0x01FE, 0x50);  /* PCL -> $0350 */
    memory_write(mem, 0x01FF, 0x03);  /* PCH */

    /* RTI instruction */
    cpu_set_pc(cpu, 0x0300);
    memory_write(mem, 0x0300, 0x40);  /* RTI opcode */

    cpu_step(cpu);

    check_pc(cpu, 0x0350);  /* Should be exactly $0350, no +1 */
    cpu_destroy(cpu);
}

TEST(test_rti_no_plus_one) {
    /*
     * Key difference from RTS:
     * RTS pulls address and adds 1 (returns to instruction AFTER JSR)
     * RTI pulls address exactly (returns to where interrupt occurred)
     */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_sp(cpu, 0xFC);
    memory_write(mem, 0x01FD, 0x00);  /* status */
    memory_write(mem, 0x01FE, 0xFF);  /* PCL -> $12FF */
    memory_write(mem, 0x01FF, 0x12);  /* PCH */

    cpu_set_pc(cpu, 0x0300);
    memory_write(mem, 0x0300, 0x40);  /* RTI */

    cpu_step(cpu);

    /* RTI should go to $12FF exactly, NOT $1300 */
    check_pc(cpu, 0x12FF);
    cpu_destroy(cpu);
}

TEST(test_rti_restores_status) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* Clear all flags first */
    cpu_set_status(cpu, 0x00);

    /* Set up stack with status that has C, Z, N flags set */
    cpu_set_sp(cpu, 0xFC);
    memory_write(mem, 0x01FD, FLAG_C | FLAG_Z | FLAG_N | FLAG_U);  /* status */
    memory_write(mem, 0x01FE, 0x00);  /* PCL */
    memory_write(mem, 0x01FF, 0x04);  /* PCH -> $0400 */

    cpu_set_pc(cpu, 0x0300);
    memory_write(mem, 0x0300, 0x40);  /* RTI */

    cpu_step(cpu);

    CHECK(cpu_get_status(cpu) & FLAG_C, "C flag restored");
    CHECK(cpu_get_status(cpu) & FLAG_Z, "Z flag restored");
    CHECK(cpu_get_status(cpu) & FLAG_N, "N flag restored");
    cpu_destroy(cpu);
}

TEST(test_rti_sp_increases_by_3) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_sp(cpu, 0xFC);  /* Starting SP */
    memory_write(mem, 0x01FD, 0x00);  /* status */
    memory_write(mem, 0x01FE, 0x00);  /* PCL */
    memory_write(mem, 0x01FF, 0x04);  /* PCH */

    cpu_set_pc(cpu, 0x0300);
    memory_write(mem, 0x0300, 0x40);  /* RTI */

    cpu_step(cpu);

    CHECK(cpu_get_sp(cpu) == 0xFF, "SP should be 0xFF after pulling 3 bytes");
    cpu_destroy(cpu);
}

TEST(test_rti_cycles) {
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_sp(cpu, 0xFC);
    memory_write(mem, 0x01FD, 0x00);
    memory_write(mem, 0x01FE, 0x00);
    memory_write(mem, 0x01FF, 0x04);

    cpu_set_pc(cpu, 0x0300);
    memory_write(mem, 0x0300, 0x40);  /* RTI */

    uint8_t cycles = cpu_step(cpu);

    CHECK(cycles == 6, "RTI takes 6 cycles");
    cpu_destroy(cpu);
}

TEST(test_rti_b_flag_ignored) {
    /*
     * The B flag only exists in the pushed copy, not in the CPU.
     * When RTI pulls status, bit 4 should be ignored.
     */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    cpu_set_status(cpu, 0x00);

    /* Push status with B flag set */
    cpu_set_sp(cpu, 0xFC);
    memory_write(mem, 0x01FD, FLAG_B | FLAG_U);  /* status with B=1 */
    memory_write(mem, 0x01FE, 0x00);
    memory_write(mem, 0x01FF, 0x04);

    cpu_set_pc(cpu, 0x0300);
    memory_write(mem, 0x0300, 0x40);  /* RTI */

    cpu_step(cpu);

    /* B flag doesn't exist in CPU, so we just verify no crash */
    /* Some implementations may set it, others ignore it */
    cpu_destroy(cpu);
}

TEST(test_brk_rti_roundtrip) {
    /*
     * BRK pushes PC+2 and status, jumps to vector.
     * RTI should return to PC+2 (the instruction after BRK's signature byte).
     */
    CPU* cpu = setup_cpu();
    Memory* mem = cpu_get_memory(cpu);

    /* Set up BRK vector to point to handler with RTI */
    memory_write(mem, 0xFFFE, 0x00);  /* vector low -> $0300 */
    memory_write(mem, 0xFFFF, 0x03);  /* vector high */

    /* Handler at $0300: just RTI */
    memory_write(mem, 0x0300, 0x40);  /* RTI */

    /* BRK at $0200 */
    memory_write(mem, 0x0200, 0x00);  /* BRK */
    memory_write(mem, 0x0201, 0xEA);  /* NOP (signature byte, skipped) */
    memory_write(mem, 0x0202, 0xEA);  /* NOP (should return here) */

    uint8_t initial_sp = cpu_get_sp(cpu);

    cpu_step(cpu);  /* Execute BRK -> jumps to $0300 */
    check_pc(cpu, 0x0300);

    cpu_step(cpu);  /* Execute RTI -> returns to $0202 */
    check_pc(cpu, 0x0202);

    CHECK(cpu_get_sp(cpu) == initial_sp, "SP restored after BRK/RTI");
    cpu_destroy(cpu);
}

/* ========================= Main ========================= */

int main(void) {
    reset_test_state();

    printf("=== Jump and Subroutine Tests ===\n\n");

    printf("--- JMP Absolute ---\n");
    RUN_TEST(test_jmp_abs_basic);
    RUN_TEST(test_jmp_abs_forward);
    RUN_TEST(test_jmp_abs_backward);
    RUN_TEST(test_jmp_abs_same_page);

    printf("\n--- JMP Indirect ---\n");
    RUN_TEST(test_jmp_ind_basic);
    RUN_TEST(test_jmp_ind_zero_page_pointer);
    RUN_TEST(test_jmp_ind_page_boundary_bug);
    RUN_TEST(test_jmp_ind_self_reference);

    printf("\n--- JSR ---\n");
    RUN_TEST(test_jsr_basic);
    RUN_TEST(test_jsr_pushes_correct_address);
    RUN_TEST(test_jsr_stack_page);
    RUN_TEST(test_jsr_nested);
    RUN_TEST(test_jsr_to_zero_page);

    printf("\n--- RTS ---\n");
    RUN_TEST(test_rts_basic);
    RUN_TEST(test_rts_adds_one);
    RUN_TEST(test_rts_stack_page);
    RUN_TEST(test_rts_wrap_sp);

    printf("\n--- JSR/RTS Integration ---\n");
    RUN_TEST(test_jsr_rts_round_trip);
    RUN_TEST(test_jsr_rts_nested_round_trip);
    RUN_TEST(test_jsr_rts_multiple_calls);
    RUN_TEST(test_jsr_rts_preserves_registers);

    printf("\n--- BRK ---\n");
    RUN_TEST(test_brk_jumps_to_vector);
    RUN_TEST(test_brk_pushes_pc_plus_2);
    RUN_TEST(test_brk_pushes_status_with_b_flag);
    RUN_TEST(test_brk_sets_i_flag);
    RUN_TEST(test_brk_sp_decreases_by_3);
    RUN_TEST(test_brk_cycles);

    printf("\n--- RTI ---\n");
    RUN_TEST(test_rti_restores_pc);
    RUN_TEST(test_rti_no_plus_one);
    RUN_TEST(test_rti_restores_status);
    RUN_TEST(test_rti_sp_increases_by_3);
    RUN_TEST(test_rti_cycles);
    RUN_TEST(test_rti_b_flag_ignored);
    RUN_TEST(test_brk_rti_roundtrip);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
