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
