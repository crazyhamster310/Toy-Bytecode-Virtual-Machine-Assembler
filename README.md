# Toy Bytecode Virtual Machine & Assembler (ByteVM)

A minimal stack-based virtual machine in C++20 and a two-pass assembler in Python 3.11.

## Quick Start

Prerequisites:
- Clang >= 14 or GCC >= 11
- CMake >= 3.24
- Python >= 3.11
- GNU Make

```bash
# Setup assembler venv and install dev dependencies
make setup-asm

# Run linters, type checks, unit tests, and E2E validation
make check

# Build optimized release binary
make build-vm-release
```

### Assemble, Execute, Trace, and Disassemble

```bash
# 1. Assemble source to binary bytecode
assembler/.venv/bin/byte-asm examples/factorial.asm -o examples/factorial.bin

# 2. Execute on C++ runtime (reading input from stdin)
echo 5 | ./build/release/byte-vm examples/factorial.bin
# Output: 120

# 3. Execute with real-time instruction tracing
echo 5 | ./build/release/byte-vm --trace examples/factorial.bin

# 4. Disassemble binary bytecode back to instruction listing
./build/release/byte-disasm examples/factorial.bin
```

## Binary Format (BVM1)

All integers are stored in little-endian byte order.

```text
+--------------------+-------------------+-----------------------------------+
| Bytes 0..3         | Bytes 4..5        | Bytes 6..N                        |
| Magic: "BVM1"      | Version: uint16_t | Bytecode stream                   |
| 0x42 0x56 0x4D 0x31| 0x01 0x00         | Opcode [Operand] ...              |
+--------------------+-------------------+-----------------------------------+
```

Execution starts at byte offset 6 (offset 0 of the bytecode stream).
Jump addresses are absolute byte offsets relative to the start of the bytecode stream.

## Instruction Set Architecture

| Mnemonic | Opcode | Operand | Size | Stack Effect | Description |
|---|---|---|---|---|---|
| `HALT`  | `0xFF` | -               | 1 B | `[] -> []`          | Stop execution |
| `PUSH`  | `0x01` | `int32`         | 5 B | `[] -> [x]`         | Push signed 32-bit integer |
| `POP`   | `0x02` | -               | 1 B | `[x] -> []`         | Discard top value |
| `DUP`   | `0x03` | -               | 1 B | `[x] -> [x, x]`     | Duplicate top value |
| `SWAP`  | `0x04` | -               | 1 B | `[a, b] -> [b, a]`  | Swap top two values |
| `ADD`   | `0x10` | -               | 1 B | `[a, b] -> [a + b]` | Modular addition |
| `SUB`   | `0x11` | -               | 1 B | `[a, b] -> [a - b]` | Modular subtraction (`b` is top) |
| `MUL`   | `0x12` | -               | 1 B | `[a, b] -> [a * b]` | Modular multiplication |
| `DIV`   | `0x13` | -               | 1 B | `[a, b] -> [a / b]` | Signed division; faults on 0 or `INT_MIN / -1` |
| `MOD`   | `0x14` | -               | 1 B | `[a, b] -> [a % b]` | Remainder; faults on 0 or `INT_MIN % -1` |
| `EQ`    | `0x20` | -               | 1 B | `[a, b] -> [a == b]`| 1 if equal, else 0 |
| `LT`    | `0x21` | -               | 1 B | `[a, b] -> [a < b]` | 1 if a < b, else 0 |
| `GT`    | `0x22` | -               | 1 B | `[a, b] -> [a > b]` | 1 if a > b, else 0 |
| `JMP`   | `0x30` | `uint32`        | 5 B | `[] -> []`          | Jump to instruction offset |
| `JZ`    | `0x31` | `uint32`        | 5 B | `[cond] -> []`      | Pop condition, jump if `cond == 0` |
| `PRINT` | `0x40` | -               | 1 B | `[x] -> []`         | Pop integer and print to stdout with newline |
| `READ`  | `0x41` | -               | 1 B | `[] -> [x]`         | Read signed 32-bit integer from stdin |
| `PUTC`  | `0x42` | -               | 1 B | `[x] -> []`         | Pop integer and print lower byte as ASCII character |
| `LOAD`  | `0x50` | -               | 1 B | `[addr] -> [val]`   | Read `memory[addr]` |
| `STORE` | `0x51` | -               | 1 B | `[val, addr] -> []` | Write `memory[addr] = val` |

## Assembly Example

```text
; Memory layout:
;   memory[0] = N (counter)
;   memory[1] = accumulator (result)

        READ            ; Read input N from stdin
        PUSH 0
        STORE           ; memory[0] = N

        PUSH 1
        PUSH 1
        STORE           ; memory[1] = 1 (accumulator)

loop:
        PUSH 0
        LOAD            ; Load N onto stack
        DUP
        PUSH 1
        GT              ; Check if N > 1
        JZ end          ; If N <= 1, exit loop

        ; accumulator = accumulator * N
        DUP             ; Duplicate N for multiplication
        PUSH 1
        LOAD            ; Load accumulator
        MUL             ; Multiply accumulator * N
        PUSH 1
        STORE           ; memory[1] = new accumulator

        ; N = N - 1
        PUSH 1
        SUB             ; Decrement N
        PUSH 0
        STORE           ; memory[0] = N - 1

        JMP loop

end:
        POP             ; Discard duplicate N left by DUP
        PUSH 1
        LOAD            ; Load final result from memory[1]
        PRINT           ; Print result to stdout
        HALT
```

## Execution Tracing (`-t`, `--trace`)

Running `byte-vm` with `--trace` produces structured execution diagnostics to `stderr` alongside program execution:

```text
[IP: 0x00000000] READ             | In => 1            | Stack: [1]
[IP: 0x00000001] PUSH 0           |                    | Stack: [1, 0]
[IP: 0x00000006] STORE            | Mem[0] <= 1        | Stack: []
[IP: 0x00000007] PUSH 1           |                    | Stack: [1]
[IP: 0x0000000C] PUSH 1           |                    | Stack: [1, 1]
[IP: 0x00000011] STORE            | Mem[1] <= 1        | Stack: []
[IP: 0x00000012] PUSH 0           |                    | Stack: [0]
[IP: 0x00000017] LOAD             | Mem[0] => 1        | Stack: [1]
[IP: 0x00000018] DUP              |                    | Stack: [1, 1]
[IP: 0x00000019] PUSH 1           |                    | Stack: [1, 1, 1]
[IP: 0x0000001E] GT               |                    | Stack: [1, 0]
[IP: 0x0000001F] JZ 0x00000043    |                    | Stack: [1]
[IP: 0x00000043] POP              |                    | Stack: []
[IP: 0x00000044] PUSH 1           |                    | Stack: [1]
[IP: 0x00000049] LOAD             | Mem[1] => 1        | Stack: [1]
[IP: 0x0000004A] PRINT            | Out <= 1           | Stack: []
[IP: 0x0000004B] HALT             |                    | Stack: []
```

- Column 1: 32-bit hex instruction pointer (`[IP: 0x00000000]`)
- Column 2: Instruction mnemonic and immediate operand (if present)
- Column 3: Directional external I/O and memory access events (`In =>`, `Out <=`, `Mem[] <=`, `Mem[] =>`)
- Column 4: Snapshot of the evaluation stack after instruction execution

## Disassembly Output

`byte-disasm` outputs disassembly listings with 32-bit byte offsets:

```text
0x00000000: READ
0x00000001: PUSH 0
0x00000006: STORE
0x00000007: PUSH 1
0x0000000C: PUSH 1
0x00000011: STORE
0x00000012: PUSH 0
0x00000017: LOAD
0x00000018: DUP
0x00000019: PUSH 1
0x0000001E: GT
0x0000001F: JZ 0x00000043
0x00000024: DUP
0x00000025: PUSH 1
0x0000002A: LOAD
0x0000002B: MUL
0x0000002C: PUSH 1
0x00000031: STORE
0x00000032: PUSH 1
0x00000037: SUB
0x00000038: PUSH 0
0x0000003D: STORE
0x0000003E: JMP 0x00000012
0x00000043: POP
0x00000044: PUSH 1
0x00000049: LOAD
0x0000004A: PRINT
0x0000004B: HALT
```

## Error Diagnostics

Runtime errors report the instruction pointer (`IP`), mnemonic, and failure details:

```text
error: [IP: 0x0000000E] (DIV): division by zero
error: [IP: 0x00000005] (ADD): stack underflow: requires 2 operand(s), available: 1
error: [IP: 0x00000014] (LOAD): memory out of bounds: address 0xFFFFFFFF exceeds size 16384
error: invalid magic bytes: expected 'BVM1'
```

## Project Structure

```text
.
├── Makefile                # Build, test, lint, format targets
├── CMakeLists.txt          # C++ build definition
├── assembler/              # Python package (byte-asm)
│   ├── pyproject.toml      # Tooling config (ruff, mypy, pytest)
│   ├── src/byte_asm/
│   │   ├── isa.py          # Single source of truth for opcodes
│   │   ├── lexer.py        # Token scanner
│   │   ├── parser.py       # Two-pass AST and label resolution
│   │   ├── emitter.py      # Binary bytecode packing
│   │   └── cli.py          # CLI entry point
│   └── tests/
├── runtime/                # C++ implementation (byte-vm and byte-disasm)
│   ├── include/vm/
│   │   ├── disasm.hpp      # Disassembler engine interface
│   │   ├── errors.hpp      # Exception hierarchy
│   │   ├── file_io.hpp     # Binary file loading utilities
│   │   ├── memory.hpp      # EvaluationStack and LinearMemory
│   │   ├── opcodes.hpp     # Opcode enum and constants
│   │   └── vm.hpp          # VirtualMachine class definition
│   ├── src/
│   │   ├── disasm.cpp      # DisassemblyEngine implementation
│   │   ├── disasm_main.cpp # byte-disasm CLI entry point
│   │   ├── file_io.cpp     # File reading implementation
│   │   ├── main.cpp        # byte-vm CLI entry point
│   │   └── vm.cpp          # Instruction dispatch loop
│   └── tests/              # GoogleTest unit tests
└── examples/
    ├── factorial.asm
    └── fibonacci.asm
```