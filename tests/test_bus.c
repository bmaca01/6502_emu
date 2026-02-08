#include "test_common.h"
#include "bus.h"
#include "memory.h"

/* Simple test device: 256-byte RAM */
typedef struct {
    uint8_t data[256];
} TestDevice;

static uint8_t test_dev_read(void* ctx, uint16_t addr) {
    TestDevice* dev = (TestDevice*)ctx;
    return dev->data[addr & 0xFF];
}

static void test_dev_write(void* ctx, uint16_t addr, uint8_t val) {
    TestDevice* dev = (TestDevice*)ctx;
    dev->data[addr & 0xFF] = val;
}

static void test_dev_destroy(void* ctx) {
    free(ctx);
}

/* ============================ Bus Tests ==================================== */

TEST(test_bus_create_destroy) {
    Bus* bus = bus_create();
    CHECK(bus != NULL, "bus_create should return non-NULL");
    bus_destroy(bus);
}

TEST(test_bus_unmapped_returns_0xff) {
    Bus* bus = bus_create();

    CHECK(bus_read(bus, 0x0000) == 0xFF, "unmapped read should return 0xFF");
    CHECK(bus_read(bus, 0x1234) == 0xFF, "unmapped read should return 0xFF");
    CHECK(bus_read(bus, 0xFFFF) == 0xFF, "unmapped read should return 0xFF");

    bus_destroy(bus);
}

TEST(test_bus_map_and_read_write) {
    Bus* bus = bus_create();
    TestDevice* dev = calloc(1, sizeof(TestDevice));

    bus_map(bus, 0x2000, 0x20FF, test_dev_read, test_dev_write,
            dev, test_dev_destroy);

    bus_write(bus, 0x2000, 0x42);
    CHECK(bus_read(bus, 0x2000) == 0x42, "should read back written value");

    bus_write(bus, 0x20FF, 0xAB);
    CHECK(bus_read(bus, 0x20FF) == 0xAB, "should read back at end of range");

    /* Outside range should still be unmapped */
    CHECK(bus_read(bus, 0x2100) == 0xFF, "outside range should be unmapped");

    bus_destroy(bus);
}

TEST(test_bus_region_priority) {
    Bus* bus = bus_create();
    TestDevice* dev1 = calloc(1, sizeof(TestDevice));
    TestDevice* dev2 = calloc(1, sizeof(TestDevice));

    /* Map dev1 first, then dev2 overlapping */
    bus_map(bus, 0x0000, 0x00FF, test_dev_read, test_dev_write,
            dev1, test_dev_destroy);
    bus_map(bus, 0x0000, 0x00FF, test_dev_read, test_dev_write,
            dev2, test_dev_destroy);

    /* Write via bus goes to dev2 (later mapping wins) */
    bus_write(bus, 0x0000, 0x42);
    CHECK(dev2->data[0] == 0x42, "later mapping should win");
    CHECK(dev1->data[0] == 0x00, "earlier mapping should not be written");

    bus_destroy(bus);
}

TEST(test_bus_map_memory) {
    Bus* bus = bus_create();
    Memory* mem = memory_create();

    bus_map_memory(bus, mem);

    bus_write(bus, 0x0200, 0x42);
    CHECK(bus_read(bus, 0x0200) == 0x42, "should read back through Memory");
    CHECK(memory_read(mem, 0x0200) == 0x42, "Memory should also see the write");

    /* bus_destroy should also destroy the Memory */
    bus_destroy(bus);
}

TEST(test_bus_load) {
    Bus* bus = bus_create();
    Memory* mem = memory_create();
    bus_map_memory(bus, mem);

    uint8_t data[] = {0xA9, 0x42, 0xAA, 0xEA};
    bus_load(bus, 0x0200, data, sizeof(data));

    CHECK(bus_read(bus, 0x0200) == 0xA9, "first byte loaded");
    CHECK(bus_read(bus, 0x0201) == 0x42, "second byte loaded");
    CHECK(bus_read(bus, 0x0202) == 0xAA, "third byte loaded");
    CHECK(bus_read(bus, 0x0203) == 0xEA, "fourth byte loaded");

    bus_destroy(bus);
}

TEST(test_bus_partial_overlap) {
    Bus* bus = bus_create();
    TestDevice* ram = calloc(1, sizeof(TestDevice));
    TestDevice* io  = calloc(1, sizeof(TestDevice));

    /* RAM covers full range */
    bus_map(bus, 0x0000, 0x00FF, test_dev_read, test_dev_write,
            ram, test_dev_destroy);
    /* I/O overlays $0080-$00FF */
    bus_map(bus, 0x0080, 0x00FF, test_dev_read, test_dev_write,
            io, test_dev_destroy);

    /* Write to $0040 goes to RAM */
    bus_write(bus, 0x0040, 0xAA);
    CHECK(ram->data[0x40] == 0xAA, "low addr goes to RAM");
    CHECK(io->data[0x40] == 0x00, "IO should not see low addr write");

    /* Write to $0090 goes to IO (overlay wins) */
    bus_write(bus, 0x0090, 0xBB);
    CHECK(io->data[0x90] == 0xBB, "high addr goes to IO overlay");
    CHECK(ram->data[0x90] == 0x00, "RAM should not see overlaid write");

    bus_destroy(bus);
}

/* ============================== Test Runner ================================ */

int main(void) {
    reset_test_state();
    printf("\n=== Bus Tests ===\n\n");

    RUN_TEST(test_bus_create_destroy);
    RUN_TEST(test_bus_unmapped_returns_0xff);
    RUN_TEST(test_bus_map_and_read_write);
    RUN_TEST(test_bus_region_priority);
    RUN_TEST(test_bus_map_memory);
    RUN_TEST(test_bus_load);
    RUN_TEST(test_bus_partial_overlap);

    print_test_summary();
    return failed_test_count > 0 ? 1 : 0;
}
