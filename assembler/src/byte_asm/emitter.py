import struct

from byte_asm.isa import (
    JUMP_INSTRUCTIONS,
    MAGIC_BYTES,
    NULLARY_INSTRUCTIONS,
    VALUE_INSTRUCTIONS,
    VERSION,
)
from byte_asm.parser import Program

_HEADER_FORMAT = "<4sH"
_NULLARY_FORMAT = "<B"
_VALUE_FORMAT = "<Bi"
_JUMP_FORMAT = "<BI"


def emit(program: Program) -> bytes:
    byte_code = bytearray()
    byte_code.extend(struct.pack(_HEADER_FORMAT, MAGIC_BYTES, VERSION))

    for instruction in program.instructions:
        if instruction.opcode.name in NULLARY_INSTRUCTIONS:
            byte_code.extend(struct.pack(_NULLARY_FORMAT, instruction.opcode))
        elif instruction.opcode.name in VALUE_INSTRUCTIONS:
            assert isinstance(instruction.operand, int)
            byte_code.extend(struct.pack(_VALUE_FORMAT, instruction.opcode, instruction.operand))
        elif instruction.opcode.name in JUMP_INSTRUCTIONS:
            assert isinstance(instruction.operand, int)
            byte_code.extend(struct.pack(_JUMP_FORMAT, instruction.opcode, instruction.operand))
        else:
            raise ValueError(f"Unknown instruction category: {instruction.opcode}")

    return bytes(byte_code)
