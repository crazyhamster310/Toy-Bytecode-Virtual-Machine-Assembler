from enum import IntEnum


class Opcode(IntEnum):
    HALT = 0xFF
    PUSH = 0x01
    POP = 0x02
    DUP = 0x03
    ADD = 0x10
    SUB = 0x11
    MUL = 0x12
    DIV = 0x13
    MOD = 0x14
    EQ = 0x20
    LT = 0x21
    GT = 0x22
    JMP = 0x30
    JZ = 0x31
    PRINT = 0x40


INSTRUCTION_SET: frozenset[str] = frozenset(op.name for op in Opcode)

JUMP_INSTRUCTIONS: frozenset[str] = frozenset({"JMP", "JZ"})
VALUE_INSTRUCTIONS: frozenset[str] = frozenset({"PUSH"})
UNARY_INSTRUCTIONS: frozenset[str] = JUMP_INSTRUCTIONS | VALUE_INSTRUCTIONS
NULLARY_INSTRUCTIONS: frozenset[str] = INSTRUCTION_SET - UNARY_INSTRUCTIONS

INSTRUCTION_SIZES: dict[str, int] = {"PUSH": 5, "JMP": 5, "JZ": 5}
DEFAULT_INSTRUCTION_SIZE: int = 1

MAGIC_BYTES: bytes = b"BVM1"
VERSION: int = 1
