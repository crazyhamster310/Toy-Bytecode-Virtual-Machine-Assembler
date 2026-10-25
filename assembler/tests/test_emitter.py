import struct

from byte_asm.emitter import _HEADER_FORMAT, _JUMP_FORMAT, _VALUE_FORMAT, emit
from byte_asm.isa import MAGIC_BYTES, VERSION, Opcode
from byte_asm.lexer import tokenize
from byte_asm.parser import parse


def test_empty() -> None:
    binary_code = emit(parse(tokenize("")))
    assert len(binary_code) == 6
    assert struct.unpack(_HEADER_FORMAT, binary_code) == (MAGIC_BYTES, VERSION)


def test_nullary_instructions() -> None:
    binary_code = emit(parse(tokenize("ADD")))
    assert len(binary_code) == 7
    assert binary_code[6] == Opcode.ADD


def test_value_instructions() -> None:
    binary_code = emit(parse(tokenize("PUSH 42")))
    assert len(binary_code) == 11
    assert struct.unpack_from(_VALUE_FORMAT, binary_code, 6) == (Opcode.PUSH, 42)


def test_negative_values() -> None:
    binary_code = emit(parse(tokenize("PUSH -42")))
    assert len(binary_code) == 11
    assert struct.unpack_from(_VALUE_FORMAT, binary_code, 6) == (Opcode.PUSH, -42)


def test_jump_instructions() -> None:
    binary_code = emit(parse(tokenize("JMP 0")))
    assert len(binary_code) == 11
    assert struct.unpack_from(_JUMP_FORMAT, binary_code, 6) == (Opcode.JMP, 0)


def test_far_jump() -> None:
    binary_code = emit(parse(tokenize("JMP " + str(1 << 31))))
    assert len(binary_code) == 11
    assert struct.unpack_from(_JUMP_FORMAT, binary_code, 6) == (Opcode.JMP, 1 << 31)


def test_sequence() -> None:
    asm = """
    PUSH 10
    DUP
    ADD
    JMP end
        end:
    HALT
    """

    binary_code = emit(parse(tokenize(asm)))
    assert len(binary_code) == 19
    assert binary_code[:6] == struct.pack(_HEADER_FORMAT, MAGIC_BYTES, VERSION)
    assert binary_code[6] == Opcode.PUSH
    assert binary_code[11] == Opcode.DUP
    assert binary_code[12] == Opcode.ADD
    assert binary_code[13] == Opcode.JMP
    assert binary_code[18] == Opcode.HALT
