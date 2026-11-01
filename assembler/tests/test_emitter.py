import struct

import pytest

from byte_asm.emitter import _HEADER_FORMAT, _JUMP_FORMAT, _VALUE_FORMAT, emit
from byte_asm.isa import (
    JUMP_INSTRUCTIONS,
    MAGIC_BYTES,
    NULLARY_INSTRUCTIONS,
    VALUE_INSTRUCTIONS,
    VERSION,
    Opcode,
)
from byte_asm.lexer import tokenize
from byte_asm.parser import parse


def test_empty() -> None:
    binary_code = emit(parse(tokenize("")))
    assert len(binary_code) == 6
    assert struct.unpack(_HEADER_FORMAT, binary_code) == (MAGIC_BYTES, VERSION)


@pytest.mark.parametrize("mnemonic", NULLARY_INSTRUCTIONS)
def test_nullary_instructions(mnemonic: str) -> None:
    binary_code = emit(parse(tokenize(mnemonic)))
    assert len(binary_code) == 7
    assert binary_code[6] == Opcode[mnemonic]


@pytest.mark.parametrize("mnemonic", VALUE_INSTRUCTIONS)
def test_value_instructions(mnemonic: str) -> None:
    binary_code = emit(parse(tokenize(mnemonic + " 42")))
    assert len(binary_code) == 11
    assert struct.unpack_from(_VALUE_FORMAT, binary_code, 6) == (Opcode[mnemonic], 42)


@pytest.mark.parametrize("mnemonic", JUMP_INSTRUCTIONS)
def test_jump_instructions(mnemonic: str) -> None:
    binary_code = emit(parse(tokenize(mnemonic + " 0")))
    assert len(binary_code) == 11
    assert struct.unpack_from(_JUMP_FORMAT, binary_code, 6) == (Opcode[mnemonic], 0)


@pytest.mark.parametrize(
    "val",
    [-(1 << 31), (1 << 31) - 1, 0],
    ids=["int32_min", "int32_max", "zero"],
)
def test_signed_int32_boundaries(val: int) -> None:
    binary = emit(parse(tokenize(f"PUSH {val}")))
    assert struct.unpack_from(_VALUE_FORMAT, binary, 6) == (Opcode.PUSH, val)


@pytest.mark.parametrize(
    "addr",
    [0, (1 << 32) - 1],
    ids=["uint32_min", "uint32_max"],
)
def test_unsigned_uint32_boundaries(addr: int) -> None:
    binary = emit(parse(tokenize(f"JMP {addr}")))
    assert struct.unpack_from(_JUMP_FORMAT, binary, 6) == (Opcode.JMP, addr)


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
