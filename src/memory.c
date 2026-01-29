#include "memory.h"
#include <stdlib.h>
#include <string.h>

struct Memory {
    uint8_t cells[65536];
};

Memory* memory_create(void) {
    Memory* m = malloc(sizeof(Memory));
    if (!m) {
        printf("Failed to init memory\n");
        exit(1);
    }
    memory_reset(m);
    return m;
}

void memory_destroy(Memory* mem) {
    free(mem);
    return;
}

void memory_reset(Memory* mem) {
    if (!mem) return;
    memset(mem->cells, 0x00, sizeof(mem->cells));
    return;
}


uint8_t memory_read(Memory* mem, uint16_t addr) {
    return mem->cells[addr];
}

void memory_write(Memory* mem, uint16_t addr, uint8_t value) {
    mem->cells[addr] = value;
    return;
}

/**
 * @brief Loads a block of data into the emulated memory at a specified address.
 *
 * Copies 'size' bytes from the source buffer 'data' into the memory array
 * starting at 'start_addr'. This is typically used for loading ROM images,
 * programs, or initializing specific memory regions.
 *
 * @param[in,out] mem        Pointer to the Memory structure to write into.
 * @param[in]     start_addr The 16-bit starting address in memory where data will be loaded.
 * @param[in]     data       Pointer to the source buffer containing bytes to copy.
 * @param[in]     size       Number of bytes to copy from the source buffer.
 *
 * @note If start_addr + size exceeds 65536 (0x10000), the implementation should
 *       either truncate the copy to prevent overflow, or wrap around to address 0x0000,
 *       depending on the desired emulation behavior.
 *
 * @warning Passing a NULL pointer for 'mem' or 'data' results in undefined behavior.
 */
void memory_load(Memory* mem, uint16_t start_addr, 
                 const uint8_t* data, size_t size) {
    /* TODO: Handle loading data beyond 0xFFFF */
    memcpy(&mem->cells[start_addr], data, size);
    return;
}

uint8_t* memory_get_raw(Memory* mem) {
    return mem->cells;
}

void memory_dump(Memory* mem, uint16_t start, uint16_t end) {
    (void)mem; (void)start; (void)end;
    return;
}
