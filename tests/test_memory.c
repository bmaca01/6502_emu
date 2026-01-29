#include "memory.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

/* Simple test macros */
#define TEST(name) static void name(void)
#define RUN_TEST(name) do { \
    printf("  %-40s", #name); \
    name(); \
    printf(" ✓\n"); \
} while(0)

/* ================================= Tests =================================== */
TEST(test_memory_create_destroy) {
    Memory* mem = memory_create();
    assert(mem != NULL);
    memory_destroy(mem);
}

TEST(test_memory_reset) {
    Memory* mem = memory_create();

    // Write something, then reset
    memory_write(mem, 0x1234, 0xAB);
    memory_reset(mem);

    // Should be zeroed after reset
    assert(memory_read(mem, 0x1234) == 0x00);

    memory_destroy(mem);
}

TEST(test_memory_read_write) {
    Memory* mem = memory_create();

    // Basic read/write
    memory_write(mem, 0x0000, 0x42);
    assert(memory_read(mem, 0x0000) == 0x42);

    memory_write(mem, 0xFFFF, 0xFF);
    assert(memory_read(mem, 0xFFFF) == 0xFF);

    memory_destroy(mem);
}

TEST(test_memory_zero_page) {
    Memory* mem = memory_create();

    // Zero page: $00-$FF
    for (uint16_t addr = 0x00; addr <= 0xFF; addr++) {
        memory_write(mem, addr, (uint8_t)addr);
    }

    for (uint16_t addr = 0x00; addr <= 0xFF; addr++) {
        assert(memory_read(mem, addr) == (uint8_t)addr);
    }

    memory_destroy(mem);
}

TEST(test_memory_stack_region) {
    Memory* mem = memory_create();

    // Stack: $0100-$01FF
    memory_write(mem, 0x0100, 0xAA);
    memory_write(mem, 0x01FF, 0xBB);

    assert(memory_read(mem, 0x0100) == 0xAA);
    assert(memory_read(mem, 0x01FF) == 0xBB);

    memory_destroy(mem);
}

TEST(test_memory_load) {
    Memory* mem = memory_create();

    // LDA #$42; STA $0200
    uint8_t program[] = {0xA9, 0x42, 0x8D, 0x00, 0x02};
    memory_load(mem, 0x8000, program, sizeof(program));

    assert(memory_read(mem, 0x8000) == 0xA9);
    assert(memory_read(mem, 0x8001) == 0x42);
    assert(memory_read(mem, 0x8004) == 0x02);

    memory_destroy(mem);
}

/* ============================== Test Runner ================================ */

int main(void) {
    printf("\n=== Memory Module Tests ===\n\n");

    RUN_TEST(test_memory_create_destroy);
    RUN_TEST(test_memory_reset);
    RUN_TEST(test_memory_read_write);
    RUN_TEST(test_memory_zero_page);
    RUN_TEST(test_memory_stack_region);
    RUN_TEST(test_memory_load);

    printf("\nAll memory tests passed!\n\n");
    return 0;
}
