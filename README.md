# 6502 Emulator

A verbose 6502 emulator for the love of the game (retro computing and computer architecture)

## Structure 

```
6502_emu/
├── src/
│   ├── main.c           # Entry point, system initialization
│   ├── cpu.c/.h         # CPU state, fetch-decode-execute loop
│   ├── opcodes.c/.h     # Opcode decoding and categorization
│   ├── addressing.c/.h  # Addressing mode decoding
│   ├── memory.c/.h      # Memory bus, read/write operations
│   └── util.c/.h        # Helpers (logging, bit manipulation)
├── tests/
│   ├── test_cpu.c       # CPU state and flag tests
│   └── test_memory.c    # Memory read/write tests
├── Makefile
└── README.md
```

## Module Breakdown

|Module|Responsibility|
|--|--|
|memory|Read/write bytes, memory mapping|
|addressing|Decode addressing mode from opcode byte|
|opcodes|Decode opcode byte into instruction enum, categorize instruction type|
|cpu|Orchestrate fetch-decode-execute, resolve effective addresses, execute instructions, hold processor/register state|


## Memory interface

### Behavioral Specifications

| Function | Behavior |
|----------|----------|
|`memory_read`|Returns byte at address|
|`memory_write`|Stores byte at address|
|`memory_reset`|Fills memory with 0x00|
|`memory_load`|Copies data into memory starting at address|

---

## CPU Module

The CPU module manages processor state and implements the fetch-decode-execute cycle. It depends on the memory module for all bus operations, enabling testability via dependency injection.

### Registers

| Register | Size | Description |
|----------|------|-------------|
| `A` | 8-bit | Accumulator |
| `X` | 8-bit | Index X - Loop counter, array indexing |
| `Y` | 8-bit | Index Y - Loop counter, array indexing |
| `SP` | 8-bit | Stack Pointer - Points within $0100-$01FF |
| `PC` | 16-bit | Program Counter |
| `P` | 8-bit | Status Register |

### Status Flags

```
  7   6   5   4   3   2   1   0
+---+---+---+---+---+---+---+---+
| N | V | - | B | D | I | Z | C |
+---+---+---+---+---+---+---+---+
```

| Bit | Flag | Name | Set When |
|-----|------|------|----------|
| 7 | N | Negative | Result bit 7 is set |
| 6 | V | Overflow | Signed arithmetic overflow |
| 5 | - | Unused | Always 1 |
| 4 | B | Break | BRK instruction executed |
| 3 | D | Decimal | BCD arithmetic mode |
| 2 | I | Interrupt Disable | IRQ disabled |
| 1 | Z | Zero | Result is zero |
| 0 | C | Carry | Unsigned overflow/borrow |

### Behavioral Specifications

| Function | Behavior |
|----------|----------|
|`cpu_create(Memory* mem)`|Allocates CPU struct, stores reference to memory|
|`cpu_destroy(CPU* cpu)`|Mimic hardware reset|
|`cpu_reset(CPU* cpu)`|Fills memory with 0x00|
|`cpu_step(CPU* cpu)`|Execute one instruction and return cycle count for that instruction|
|`cpu_nmi(CPU* cpu)`|Trigger non-maskable interrupt|
|`cpu_irq(CPU* cpu)`|Trigger interrupt request|
