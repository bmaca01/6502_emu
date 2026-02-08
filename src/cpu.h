#ifndef CPU_H_
#define CPU_H_

#include "bus.h"

#define FLAG_C (1 << 0)  // Carry
#define FLAG_Z (1 << 1)  // Zero
#define FLAG_I (1 << 2)  // Interrupt Disable
#define FLAG_D (1 << 3)  // Decimal Mode
#define FLAG_B (1 << 4)  // Break
#define FLAG_U (1 << 5)  // Unused
#define FLAG_V (1 << 6)  // Overflow
#define FLAG_N (1 << 7)  // Negative

typedef struct CPU CPU;

CPU*    cpu_create(Bus* bus);
void    cpu_destroy(CPU* cpu);
void    cpu_reset(CPU* cpu);

uint8_t cpu_step(CPU* cpu);

void    cpu_nmi(CPU* cpu);
void    cpu_irq(CPU* cpu);

/* Accessors for testing */
uint8_t  cpu_get_a(CPU* cpu);
uint8_t  cpu_get_x(CPU* cpu);
uint8_t  cpu_get_y(CPU* cpu);
uint8_t  cpu_get_sp(CPU* cpu);
uint16_t cpu_get_pc(CPU* cpu);
uint8_t  cpu_get_status(CPU* cpu);

void     cpu_set_a(CPU* cpu, uint8_t val);
void     cpu_set_x(CPU* cpu, uint8_t val);
void     cpu_set_y(CPU* cpu, uint8_t val);
void     cpu_set_sp(CPU* cpu, uint8_t val);
void     cpu_set_pc(CPU* cpu, uint16_t val);
void     cpu_set_status(CPU* cpu, uint8_t val);

Bus*     cpu_get_bus(CPU* cpu);

#endif
