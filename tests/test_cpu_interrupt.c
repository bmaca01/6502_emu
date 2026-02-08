#include "test_common.h"
#include "bus.h"
#include "memory.h"

/*
 * 6502 Interrupt Vector Addresses:
 *   NMI:   $FFFA/$FFFB
 *   RESET: $FFFC/$FFFD
 *   IRQ:   $FFFE/$FFFF (shared with BRK)
 *
 * setup_cpu() sets reset vector to $0200 and IRQ vector is at $FFFE/$FFFF.
 * Tests set up NMI/IRQ vectors as needed.
 */

/* ========================= IRQ Tests ========================= */

/* IRQ basic: assert IRQ with FLAG_I clear -> vectors through $FFFE/$FFFF,
 * pushes PC + status (B=0), sets FLAG_I, 7 cycles */
TEST(test_irq_basic) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Set up IRQ vector at $FFFE/$FFFF -> handler at $0400 */
    bus_write(bus, 0xFFFE, 0x00);
    bus_write(bus, 0xFFFF, 0x04);

    /* Put a NOP at $0200 (next instruction if no IRQ) */
    bus_write(bus, 0x0200, 0xEA);  /* NOP */

    /* Put a NOP at the handler too */
    bus_write(bus, 0x0400, 0xEA);  /* NOP */

    /* Clear FLAG_I so IRQ is not masked */
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);
    uint8_t status_before = cpu_get_status(cpu);
    uint8_t sp_before = cpu_get_sp(cpu);

    /* Assert IRQ line */
    cpu_irq(cpu);

    /* Step should service the IRQ instead of executing NOP */
    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cycles, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0400);
    CHECK(cpu_get_status(cpu) & FLAG_I, "FLAG_I should be set after IRQ");
    CHECK_EQ(cpu_get_sp(cpu), sp_before - 3);

    /* Check pushed values on stack */
    /* PCH */
    CHECK_EQ(bus_read(bus, 0x0100 | sp_before), 0x02);
    /* PCL */
    CHECK_EQ(bus_read(bus, 0x0100 | (uint8_t)(sp_before - 1)), 0x00);
    /* Status: B should be clear, U should be set */
    uint8_t pushed_status = bus_read(bus, 0x0100 | (uint8_t)(sp_before - 2));
    CHECK(!(pushed_status & FLAG_B), "Pushed status should have B=0 for IRQ");
    CHECK(pushed_status & FLAG_U, "Pushed status should have U=1");
    /* Other flags should match what was set before */
    CHECK_EQ(pushed_status & ~(FLAG_B | FLAG_U), status_before & ~(FLAG_B | FLAG_U));

    cpu_irq_release(cpu);
    cpu_destroy(cpu);
}

/* IRQ masked: assert IRQ with FLAG_I set -> no interrupt, normal execution */
TEST(test_irq_masked) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0xFFFE, 0x00);
    bus_write(bus, 0xFFFF, 0x04);
    bus_write(bus, 0x0200, 0xEA);  /* NOP */

    /* FLAG_I is set by default after reset */
    CHECK(cpu_get_status(cpu) & FLAG_I, "FLAG_I should be set after reset");

    cpu_irq(cpu);
    uint8_t cycles = cpu_step(cpu);

    /* Should have executed the NOP, not the IRQ */
    CHECK_EQ(cycles, 2);
    CHECK_EQ(cpu_get_pc(cpu), 0x0201);

    cpu_irq_release(cpu);
    cpu_destroy(cpu);
}

/* IRQ level-triggered: assert IRQ, service it (RTI + CLI), IRQ still held -> fires again */
TEST(test_irq_level_triggered) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* IRQ handler at $0400: RTI */
    bus_write(bus, 0xFFFE, 0x00);
    bus_write(bus, 0xFFFF, 0x04);
    bus_write(bus, 0x0400, 0x40);  /* RTI */

    /* Main code at $0200: CLI, NOP */
    bus_write(bus, 0x0200, 0x58);  /* CLI */
    bus_write(bus, 0x0201, 0xEA);  /* NOP */

    /* Clear FLAG_I */
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);

    /* Assert IRQ and hold it */
    cpu_irq(cpu);

    /* Step 1: services IRQ (PC was $0200, pushed to stack) */
    uint8_t cycles1 = cpu_step(cpu);
    CHECK_EQ(cycles1, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0400);

    /* Step 2: execute RTI at $0400 -> returns to $0200 */
    cpu_step(cpu);
    CHECK_EQ(cpu_get_pc(cpu), 0x0200);

    /* After RTI, status is restored (FLAG_I clear), so IRQ should fire again.
     * But we need to execute CLI first since the pushed status had FLAG_I clear,
     * RTI restores it. Actually, we cleared FLAG_I before the IRQ, and the IRQ
     * pushes the status with FLAG_I clear. RTI pulls it back, so FLAG_I is clear. */

    /* Step 3: IRQ still held low, FLAG_I clear -> fires again */
    uint8_t cycles3 = cpu_step(cpu);
    CHECK_EQ(cycles3, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0400);

    cpu_irq_release(cpu);
    cpu_destroy(cpu);
}

/* IRQ release: assert then release IRQ before step -> no interrupt */
TEST(test_irq_release) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0xFFFE, 0x00);
    bus_write(bus, 0xFFFF, 0x04);
    bus_write(bus, 0x0200, 0xEA);  /* NOP */

    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);

    /* Assert then immediately release */
    cpu_irq(cpu);
    cpu_irq_release(cpu);

    uint8_t cycles = cpu_step(cpu);

    /* Should execute NOP normally since IRQ line is no longer held */
    CHECK_EQ(cycles, 2);
    CHECK_EQ(cpu_get_pc(cpu), 0x0201);

    cpu_destroy(cpu);
}

/* ========================= NMI Tests ========================= */

/* NMI basic: assert NMI -> vectors through $FFFA/$FFFB, pushes PC + status (B=0), 7 cycles */
TEST(test_nmi_basic) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* Set up NMI vector -> handler at $0500 */
    bus_write(bus, 0xFFFA, 0x00);
    bus_write(bus, 0xFFFB, 0x05);

    bus_write(bus, 0x0200, 0xEA);  /* NOP */
    bus_write(bus, 0x0500, 0xEA);  /* NOP */

    uint8_t sp_before = cpu_get_sp(cpu);

    /* Assert NMI */
    cpu_nmi(cpu);

    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cycles, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0500);
    CHECK(cpu_get_status(cpu) & FLAG_I, "FLAG_I should be set after NMI");
    CHECK_EQ(cpu_get_sp(cpu), sp_before - 3);

    /* Check pushed status has B=0 */
    uint8_t pushed_status = bus_read(bus, 0x0100 | (uint8_t)(sp_before - 2));
    CHECK(!(pushed_status & FLAG_B), "Pushed status should have B=0 for NMI");
    CHECK(pushed_status & FLAG_U, "Pushed status should have U=1");

    /* Check pushed PC = $0200 */
    CHECK_EQ(bus_read(bus, 0x0100 | sp_before), 0x02);       /* PCH */
    CHECK_EQ(bus_read(bus, 0x0100 | (uint8_t)(sp_before - 1)), 0x00);  /* PCL */

    cpu_nmi_release(cpu);
    cpu_destroy(cpu);
}

/* NMI ignores FLAG_I: assert NMI with FLAG_I set -> still fires */
TEST(test_nmi_ignores_flag_i) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0xFFFA, 0x00);
    bus_write(bus, 0xFFFB, 0x05);
    bus_write(bus, 0x0200, 0xEA);  /* NOP */

    /* FLAG_I is set by default after reset */
    CHECK(cpu_get_status(cpu) & FLAG_I, "FLAG_I should be set");

    cpu_nmi(cpu);
    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cycles, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0500);

    cpu_nmi_release(cpu);
    cpu_destroy(cpu);
}

/* NMI edge-triggered: hold NMI low across multiple steps -> fires only once */
TEST(test_nmi_edge_triggered) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0xFFFA, 0x00);
    bus_write(bus, 0xFFFB, 0x05);

    /* NOP sled at handler */
    bus_write(bus, 0x0500, 0xEA);  /* NOP */
    bus_write(bus, 0x0501, 0xEA);  /* NOP */
    bus_write(bus, 0x0502, 0xEA);  /* NOP */

    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);

    /* Assert NMI and hold it */
    cpu_nmi(cpu);

    /* Step 1: NMI fires */
    uint8_t cycles1 = cpu_step(cpu);
    CHECK_EQ(cycles1, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0500);

    /* Step 2: NMI still held, but edge already detected -> should NOT fire again */
    uint8_t cycles2 = cpu_step(cpu);
    CHECK_EQ(cycles2, 2);  /* NOP */
    CHECK_EQ(cpu_get_pc(cpu), 0x0501);

    /* Step 3: Still held, still no new edge */
    uint8_t cycles3 = cpu_step(cpu);
    CHECK_EQ(cycles3, 2);  /* NOP */
    CHECK_EQ(cpu_get_pc(cpu), 0x0502);

    cpu_nmi_release(cpu);
    cpu_destroy(cpu);
}

/* NMI re-trigger: assert, release, assert again -> fires twice */
TEST(test_nmi_retrigger) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    bus_write(bus, 0xFFFA, 0x00);
    bus_write(bus, 0xFFFB, 0x05);

    /* Handler: NOP sled */
    bus_write(bus, 0x0500, 0xEA);  /* NOP */
    bus_write(bus, 0x0501, 0xEA);  /* NOP */

    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);

    /* First NMI assertion */
    cpu_nmi(cpu);

    /* Step 1: NMI fires, jumps to $0500 */
    uint8_t cycles1 = cpu_step(cpu);
    CHECK_EQ(cycles1, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0500);

    /* Release NMI */
    cpu_nmi_release(cpu);

    /* Step 2: execute NOP at $0500, edge detection updates */
    cpu_step(cpu);
    CHECK_EQ(cpu_get_pc(cpu), 0x0501);

    /* Re-assert NMI (new falling edge) */
    cpu_nmi(cpu);

    /* Step 3: NMI fires again */
    uint8_t cycles3 = cpu_step(cpu);
    CHECK_EQ(cycles3, 7);

    cpu_nmi_release(cpu);
    cpu_destroy(cpu);
}

/* NMI priority over IRQ: both asserted -> NMI fires first */
TEST(test_nmi_priority_over_irq) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* NMI handler at $0500 */
    bus_write(bus, 0xFFFA, 0x00);
    bus_write(bus, 0xFFFB, 0x05);
    /* IRQ handler at $0400 */
    bus_write(bus, 0xFFFE, 0x00);
    bus_write(bus, 0xFFFF, 0x04);

    bus_write(bus, 0x0200, 0xEA);  /* NOP */

    /* Clear FLAG_I so IRQ would fire */
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);

    /* Assert both */
    cpu_nmi(cpu);
    cpu_irq(cpu);

    /* Step: NMI should take priority */
    uint8_t cycles = cpu_step(cpu);
    CHECK_EQ(cycles, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0500);

    cpu_nmi_release(cpu);
    cpu_irq_release(cpu);
    cpu_destroy(cpu);
}

/* BRK unchanged: BRK still works correctly with the refactored code */
TEST(test_brk_still_works) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* IRQ/BRK handler at $0400 */
    bus_write(bus, 0xFFFE, 0x00);
    bus_write(bus, 0xFFFF, 0x04);

    /* BRK at $0200 */
    bus_write(bus, 0x0200, 0x00);  /* BRK */

    uint8_t sp_before = cpu_get_sp(cpu);

    /* Clear FLAG_I so we can verify it gets set */
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);

    uint8_t cycles = cpu_step(cpu);

    CHECK_EQ(cycles, 7);
    CHECK_EQ(cpu_get_pc(cpu), 0x0400);
    CHECK(cpu_get_status(cpu) & FLAG_I, "FLAG_I should be set after BRK");
    CHECK_EQ(cpu_get_sp(cpu), sp_before - 3);

    /* BRK pushes PC+2 */
    uint8_t pch = bus_read(bus, 0x0100 | sp_before);
    uint8_t pcl = bus_read(bus, 0x0100 | (uint8_t)(sp_before - 1));
    uint16_t pushed_pc = ((uint16_t)pch << 8) | pcl;
    CHECK_EQ(pushed_pc, 0x0202);

    /* BRK pushes status with B=1 */
    uint8_t pushed_status = bus_read(bus, 0x0100 | (uint8_t)(sp_before - 2));
    CHECK(pushed_status & FLAG_B, "BRK should push status with B=1");
    CHECK(pushed_status & FLAG_U, "BRK should push status with U=1");

    cpu_destroy(cpu);
}

/* RTI from IRQ roundtrip: IRQ -> handler -> RTI -> resumes at correct address */
TEST(test_rti_from_irq_roundtrip) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* IRQ handler at $0400: RTI */
    bus_write(bus, 0xFFFE, 0x00);
    bus_write(bus, 0xFFFF, 0x04);
    bus_write(bus, 0x0400, 0x40);  /* RTI */

    /* Main code at $0200: NOP, NOP */
    bus_write(bus, 0x0200, 0xEA);  /* NOP */
    bus_write(bus, 0x0201, 0xEA);  /* NOP */

    /* Clear FLAG_I */
    cpu_set_status(cpu, cpu_get_status(cpu) & ~FLAG_I);
    uint8_t status_before = cpu_get_status(cpu);

    /* Assert IRQ */
    cpu_irq(cpu);

    /* Step 1: IRQ fires, pushes $0200, jumps to $0400 */
    cpu_step(cpu);
    CHECK_EQ(cpu_get_pc(cpu), 0x0400);

    /* Release IRQ before RTI, otherwise it would fire again */
    cpu_irq_release(cpu);

    /* Step 2: RTI at $0400 -> returns to $0200 */
    cpu_step(cpu);
    CHECK_EQ(cpu_get_pc(cpu), 0x0200);

    /* Status should be restored (FLAG_I should be clear again) */
    /* Note: RTI clears B and U from pulled status, so compare without those */
    CHECK_EQ(cpu_get_status(cpu) & ~(FLAG_B | FLAG_U),
             status_before & ~(FLAG_B | FLAG_U));

    /* Step 3: Should now execute NOP at $0200 normally */
    uint8_t cycles = cpu_step(cpu);
    CHECK_EQ(cycles, 2);
    CHECK_EQ(cpu_get_pc(cpu), 0x0201);

    cpu_destroy(cpu);
}

/* RTI from NMI roundtrip */
TEST(test_rti_from_nmi_roundtrip) {
    CPU* cpu = setup_cpu();
    Bus* bus = cpu_get_bus(cpu);

    /* NMI handler at $0500: RTI */
    bus_write(bus, 0xFFFA, 0x00);
    bus_write(bus, 0xFFFB, 0x05);
    bus_write(bus, 0x0500, 0x40);  /* RTI */

    /* Main code at $0200: NOP */
    bus_write(bus, 0x0200, 0xEA);  /* NOP */

    uint8_t status_before = cpu_get_status(cpu);

    /* Assert NMI */
    cpu_nmi(cpu);

    /* Step 1: NMI fires */
    cpu_step(cpu);
    CHECK_EQ(cpu_get_pc(cpu), 0x0500);

    /* Release NMI */
    cpu_nmi_release(cpu);

    /* Step 2: RTI -> returns to $0200 */
    cpu_step(cpu);
    CHECK_EQ(cpu_get_pc(cpu), 0x0200);

    /* Status restored */
    CHECK_EQ(cpu_get_status(cpu) & ~(FLAG_B | FLAG_U),
             status_before & ~(FLAG_B | FLAG_U));

    /* Step 3: Normal NOP execution */
    uint8_t cycles = cpu_step(cpu);
    CHECK_EQ(cycles, 2);
    CHECK_EQ(cpu_get_pc(cpu), 0x0201);

    cpu_destroy(cpu);
}

/* ============================== Test Runner ================================ */

int main(void) {
    reset_test_state();
    printf("\n=== CPU Interrupt Tests (IRQ, NMI) ===\n\n");

    printf("--- IRQ Tests ---\n");
    RUN_TEST(test_irq_basic);
    RUN_TEST(test_irq_masked);
    RUN_TEST(test_irq_level_triggered);
    RUN_TEST(test_irq_release);

    printf("\n--- NMI Tests ---\n");
    RUN_TEST(test_nmi_basic);
    RUN_TEST(test_nmi_ignores_flag_i);
    RUN_TEST(test_nmi_edge_triggered);
    RUN_TEST(test_nmi_retrigger);
    RUN_TEST(test_nmi_priority_over_irq);

    printf("\n--- Regression / Roundtrip Tests ---\n");
    RUN_TEST(test_brk_still_works);
    RUN_TEST(test_rti_from_irq_roundtrip);
    RUN_TEST(test_rti_from_nmi_roundtrip);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
