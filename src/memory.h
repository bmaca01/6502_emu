#ifndef MEM_H_
#define MEM_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct Memory Memory;

/* Lifecycle */
Memory*     memory_create(void);
void        memory_destroy(Memory* mem);
void        memory_reset(Memory* mem);

/* read / write */
uint8_t     memory_read(Memory* mem, uint16_t addr);
void        memory_write(Memory* mem, uint16_t addr, uint8_t value);

void        memory_load(Memory* mem, uint16_t start_addr,
                        const uint8_t* data, size_t size);
uint8_t*    memory_get_raw(Memory* mem);
void        memory_dump(Memory* mem, uint16_t start, uint16_t end);

#endif
