# 8086 Disassembler and Emulator

This program is a disassembler and emulator for a subset of Intel 8086 instructions. It reads binary machine code from an input file, disassembles it into assembly instructions, and optionally simulates the execution of those instructions while tracking register and flag state changes.

## Features

- Disassembly: Converts 8086 machine code to assembly mnemonics
Simulation: Optionally executes instructions and tracks register/flag changes
- Register Tracking: Monitors all general-purpose and segment registers
- Flag Tracking: Tracks status flags (CF, PF, AF, ZF, SF, OF)
- Memory Dump: Optionally dumps memory state to a file

## Usage
```bash
./program <infile> <outfile> [simulate [dump]]
```
- `<infile>`: Binary input file containing 8086 machine code
- `<outfile>`: Text output file for disassembled instructions
- `simulate`: Optional flag to enable instruction simulation
- `dump`: Optional flag to enable memory dump to ./bin/dump.data

## Supported Instructions

### Data Transfer Instructions

- `mov` between registers and memory
- `mov` with immediate values
- `mov` to/from accumulator
- `mov` to/from segment registers

### Arithmetic Instructions

- `add`, `sub`, `cmp` between registers and memory
- `add`, `sub`, `cmp` with immediate values
- `add`, `sub`, `cmp` with accumulator

### Control Transfer Instructions
- Conditional jumps: `jz`, `jl`, `jle`, `jb`, `jbe`, `jp`, `jo`, `js`, `jnz`, `jge`, `jg`, `jae`, `ja`, `jnp`, `jno`, `jns`
- Loop instructions: `loop`, `loopz`, `loopnz`
- `jcxz` (jump if CX is zero)

## Internal Architecture

### Data Structures

#### Register Organization:

- General-purpose registers: AX, BX, CX, DX, SP, BP, SI, DI
- Segment registers: ES, CS, SS, DS
- Flags register: 16-bit status flags

#### Instruction Format:

The program uses an Instruction structure that tracks:

- Opcode type
- Width (byte/word operation)
- Direction (register→memory or memory→register)
- ModR/M byte components (mod, reg, r/m fields)
- Immediate values
- Displacement values
- Jump targets

### Memory Management

- Uses an arena allocator for efficient memory management
- 64KB RAM space (simulating 8086 address space)
- Input file limited to 65,536 bytes maximum

## Key Components

1. **Opcode Decoding**: Identifies instruction types from byte patterns
2. **ModR/M Processing**: Handles complex 8086 addressing modes
3. **Displacement Calculation**: Computes memory address offsets
4. **Arithmetic Operations**: Simulates ADD/SUB/CMP with flag updates
5. **Condition Handling**: Implements conditional jump logic based on flags
6. **Output Generation**: Creates formatted assembly output with simulation comments

## Simulation Details

When simulation is enabled, the program:

- Executes each instruction and updates register values
- Tracks changes to status flags after arithmetic operations
- Handles conditional jumps based on current flag states
- Updates memory contents for store operations
- Provides before/after values in comments for easy tracing

## Output Format

The output file contains:

- Instruction disassembly with IP labels
- Simulation comments showing register changes (when enabled)
- Flag state changes after arithmetic operations
- Final register values section
- Final flags state

## Limitations

- Supports only a subset of 8086 instructions
- No support for interrupts or privileged instructions
- No I/O port simulation
- Limited to 16-bit real mode operation
- Memory dump format is raw binary without segmentation

## Error Handling

The program includes comprehensive error checking for:

- File I/O operations
- Memory allocation failures
- Invalid instruction decoding
- Address calculation errors
- Buffer overflow prevention

## Build Requirements

- C standard library
- POSIX system calls (open, read, write, close, fstat)
- Standard integer types (uint8_t, uint16_t, etc.)

This tool is useful for educational purposes, debugging 8086 code, and understanding how x86 assembly instructions map to machine code.
