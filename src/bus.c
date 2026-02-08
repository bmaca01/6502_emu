#include "bus.h"
#include "memory.h"
#include <stdlib.h>
#include <stdio.h>

#define MAX_REGIONS 16

typedef struct {
    uint16_t        start;
    uint16_t        end;
    bus_read_fn     read;
    bus_write_fn    write;
    void*           ctx;
    bus_destroy_fn  destroy;
} BusRegion;

struct Bus {
    BusRegion regions[MAX_REGIONS];
    int       region_count;
};

Bus* bus_create(void) {
    Bus* b = malloc(sizeof(Bus));
    if (!b) {
        printf("Failed to init bus\n");
        exit(1);
    }
    b->region_count = 0;
    return b;
}

void bus_destroy(Bus* bus) {
    if (!bus) return;

    /* Destroy owned devices (deduplicate ctx pointers) */
    void* destroyed[MAX_REGIONS];
    int destroyed_count = 0;

    for (int i = 0; i < bus->region_count; i++) {
        if (!bus->regions[i].destroy || !bus->regions[i].ctx)
            continue;

        /* Check if already destroyed */
        bool already = false;
        for (int j = 0; j < destroyed_count; j++) {
            if (destroyed[j] == bus->regions[i].ctx) {
                already = true;
                break;
            }
        }
        if (!already) {
            bus->regions[i].destroy(bus->regions[i].ctx);
            destroyed[destroyed_count++] = bus->regions[i].ctx;
        }
    }

    free(bus);
}

bool bus_map(Bus* bus, uint16_t start, uint16_t end,
             bus_read_fn read_fn, bus_write_fn write_fn,
             void* ctx, bus_destroy_fn destroy_fn) {
    if (bus->region_count >= MAX_REGIONS) return false;

    BusRegion* r = &bus->regions[bus->region_count++];
    r->start   = start;
    r->end     = end;
    r->read    = read_fn;
    r->write   = write_fn;
    r->ctx     = ctx;
    r->destroy = destroy_fn;
    return true;
}

uint8_t bus_read(Bus* bus, uint16_t addr) {
    /* Reverse scan: last-mapped region wins */
    for (int i = bus->region_count - 1; i >= 0; i--) {
        if (addr >= bus->regions[i].start && addr <= bus->regions[i].end) {
            return bus->regions[i].read(bus->regions[i].ctx, addr);
        }
    }
    return 0xFF; /* Open bus */
}

void bus_write(Bus* bus, uint16_t addr, uint8_t val) {
    /* Reverse scan: last-mapped region wins */
    for (int i = bus->region_count - 1; i >= 0; i--) {
        if (addr >= bus->regions[i].start && addr <= bus->regions[i].end) {
            bus->regions[i].write(bus->regions[i].ctx, addr, val);
            return;
        }
    }
    /* Unmapped write: silently ignored */
}

void bus_load(Bus* bus, uint16_t addr, const uint8_t* data, size_t size) {
    for (size_t i = 0; i < size; i++) {
        bus_write(bus, addr + (uint16_t)i, data[i]);
    }
}

/* Adapter functions for Memory* */
static uint8_t mem_adapter_read(void* ctx, uint16_t addr) {
    return memory_read((Memory*)ctx, addr);
}

static void mem_adapter_write(void* ctx, uint16_t addr, uint8_t val) {
    memory_write((Memory*)ctx, addr, val);
}

static void mem_adapter_destroy(void* ctx) {
    memory_destroy((Memory*)ctx);
}

void bus_map_memory(Bus* bus, Memory* mem) {
    bus_map(bus, 0x0000, 0xFFFF,
            mem_adapter_read, mem_adapter_write,
            mem, mem_adapter_destroy);
}
