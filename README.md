# 6502 Emulator

A verbose 6502 emulator for the love of the game (retro computing and computer architecture)

## Structure 

```
6502_emu/
├── src/
│   ├── main.c           # Entry point, system initialization
│   ├── cpu.c/.h         # CPU state, fetch-decode-execute loop
│   ├── bus.c/.h         # Bus abstraction, region-mapped device routing
│   ├── opcodes.c/.h     # Opcode decoding and categorization
│   ├── addressing.c/.h  # Addressing mode decoding
│   ├── memory.c/.h      # Memory bus, read/write operations
│   └── util.c/.h        # Helpers (logging, bit manipulation)
├── tests/
│   ├── test_common.c/.h    # Shared test framework and helpers
│   ├── test_bus.c          # Bus module tests
│   ├── test_cpu_load_store.c # Load/store instruction tests
│   ├── test_cpu_arith.c    # Arithmetic instruction tests
│   ├── test_cpu_logic.c    # Logic instruction tests
│   ├── test_cpu_branch.c   # Branch instruction tests
│   ├── test_cpu_jump.c     # Jump/subroutine instruction tests
│   ├── test_cpu_misc.c     # Transfer, stack, flag tests
│   ├── test_cpu_interrupt.c # Interrupt tests
│   ├── test_integration.c  # Integration tests
│   ├── test_memory.c       # Memory module tests
│   └── test_util.c         # Utility function tests
├── Makefile
└── README.md
```

## Module Breakdown

|Module|Responsibility|
|--|--|
|memory|Read/write bytes, memory mapping|
|bus|Route reads/writes to mapped devices by address region|
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

## Bus Module

The bus sits between the CPU and devices, routing reads and writes by address region. Devices are mapped to non-overlapping address ranges; if regions overlap, the last-mapped region wins. Unmapped reads return `$FF`; unmapped writes are silently ignored.

### Behavioral Specifications

| Function | Behavior |
|----------|----------|
|`bus_create()`|Allocates bus with empty region table|
|`bus_destroy(Bus* bus)`|Destroys all mapped devices (calls each region's destroy callback), then frees the bus|
|`bus_map(Bus* bus, start, end, read_fn, write_fn, ctx, destroy_fn)`|Registers a device for the address range `[start, end]`|
|`bus_read(Bus* bus, uint16_t addr)`|Returns byte from the device mapped at `addr`, or `$FF` if unmapped|
|`bus_write(Bus* bus, uint16_t addr, uint8_t val)`|Writes byte to the device mapped at `addr`; no-op if unmapped|
|`bus_load(Bus* bus, uint16_t addr, data, size)`|Bulk-writes `size` bytes into bus starting at `addr`|
|`bus_map_memory(Bus* bus, Memory* mem)`|Convenience: maps a Memory device across the full `$0000–$FFFF` range|

---

## CPU Module

The CPU module manages processor state and implements the fetch-decode-execute cycle. It depends on the bus for all read/write operations, enabling testability via dependency injection.

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

### Interrupts

#### Interrupt Vectors

| Vector | Address | Trigger |
|--------|---------|---------|
| NMI | $FFFA–$FFFB | Falling edge on NMI line |
| RESET | $FFFC–$FFFD | Power-on / `cpu_reset` |
| IRQ/BRK | $FFFE–$FFFF | IRQ line low (maskable) or BRK instruction |

When an interrupt is serviced the CPU pushes PC (high then low) and the status register onto the stack, sets the I flag, then loads PC from the appropriate vector. The B flag is set in the pushed status for BRK, clear for hardware interrupts. The whole sequence takes 7 cycles.

NMI is edge-triggered and has the highest priority — it cannot be masked. IRQ is level-triggered and is ignored while the I flag is set.

### Behavioral Specifications

| Function | Behavior |
|----------|----------|
|`cpu_create(Bus* bus)`|Allocates CPU struct, stores reference to bus|
|`cpu_destroy(CPU* cpu)`|Frees CPU struct and destroys the bus (and all mapped devices)|
|`cpu_reset(CPU* cpu)`|Resets registers to power-on state, loads PC from RESET vector ($FFFC)|
|`cpu_step(CPU* cpu)`|Execute one instruction and return cycle count for that instruction|
|`cpu_nmi(CPU* cpu)`|Assert NMI line (edge-triggered, serviced on next step)|
|`cpu_nmi_release(CPU* cpu)`|Release NMI line|
|`cpu_irq(CPU* cpu)`|Assert IRQ line (level-triggered, masked by I flag)|
|`cpu_irq_release(CPU* cpu)`|Release IRQ line|
