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

Assemble and execute an example:
```bash
assembler/.venv/bin/byte-asm examples/factorial.asm -o examples/factorial.bin
./build/release/byte-vm examples/factorial.bin
# Output: 120
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
| `LOAD`  | `0x50` | -               | 1 B | `[addr] -> [val]`   | Read `memory[addr]` |
| `STORE` | `0x51` | -               | 1 B | `[val, addr] -> []` | Write `memory[addr] = val` |

## Assembly Syntax

```text
; Comments start with semicolon
        PUSH 10
        PUSH 0
        STORE           ; memory[0] = 10

loop:
        PUSH 0
        LOAD
        DUP
        JZ end          ; while memory[0] != 0
        PUSH 1
        SUB
        PUSH 0
        STORE
        JMP loop

end:
        POP
        PUSH 42
        PRINT
        HALT
```

## Project Structure

```text
.
├── Makefile              # Build, test, lint, format targets
├── CMakeLists.txt        # C++ build definition
├── assembler/            # Python package (byte-asm)
│   ├── pyproject.toml    # Tooling config (ruff, mypy, pytest)
│   ├── src/byte_asm/
│   │   ├── isa.py        # Single source of truth for opcodes
│   │   ├── lexer.py      # Token scanner
│   │   ├── parser.py     # Two-pass AST and label resolution
│   │   ├── emitter.py    # Binary bytecode packing
│   │   └── cli.py        # CLI entry point
│   └── tests/
├── runtime/              # C++ VM implementation (byte-vm)
│   ├── include/vm/
│   │   ├── errors.hpp    # Exception hierarchy
│   │   ├── memory.hpp    # EvaluationStack and LinearMemory
│   │   ├── opcodes.hpp   # Opcode enum and constants
│   │   └── vm.hpp        # VirtualMachine class definition
│   ├── src/
│   │   ├── main.cpp      # CLI binary entry point
│   │   └── vm.cpp        # Instruction dispatch loop
│   └── tests/            # GoogleTest unit tests
└── examples/
    ├── factorial.asm
    └── fibonacci.asm
```