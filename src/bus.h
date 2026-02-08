#ifndef BUS_H_
#define BUS_H_

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef struct Bus Bus;
typedef struct Memory Memory;

/* Device callback types */
typedef uint8_t (*bus_read_fn)(void* ctx, uint16_t addr);
typedef void    (*bus_write_fn)(void* ctx, uint16_t addr, uint8_t val);
typedef void    (*bus_destroy_fn)(void* ctx);

/* Lifecycle */
Bus*    bus_create(void);
void    bus_destroy(Bus* bus);

/* Region mapping */
bool    bus_map(Bus* bus, uint16_t start, uint16_t end,
                bus_read_fn read_fn, bus_write_fn write_fn,
                void* ctx, bus_destroy_fn destroy_fn);

/* Read / Write */
uint8_t bus_read(Bus* bus, uint16_t addr);
void    bus_write(Bus* bus, uint16_t addr, uint8_t val);

/* Convenience */
void    bus_load(Bus* bus, uint16_t addr, const uint8_t* data, size_t size);
void    bus_map_memory(Bus* bus, Memory* mem);

#endif
