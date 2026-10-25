import pytest

import byte_asm.parser as par
from byte_asm.isa import JUMP_INSTRUCTIONS, NULLARY_INSTRUCTIONS, UNARY_INSTRUCTIONS
from byte_asm.lexer import tokenize


def test_empty_program() -> None:
    program = par.parse(tokenize(""))
    assert program.instructions == []
    assert program.symbol_table == {}


@pytest.mark.parametrize("mnemonic", NULLARY_INSTRUCTIONS)
def test_instructions_no_operands(mnemonic: str) -> None:
    program = par.parse(tokenize(mnemonic))
    assert len(program.instructions) == 1
    assert program.instructions[0].opcode.name == mnemonic
    assert program.instructions[0].operand is None
    assert len(program.symbol_table) == 0


def test_push() -> None:
    program = par.parse(tokenize("PUSH 42"))
    assert len(program.instructions) == 1

    inst = program.instructions[0]
    assert inst.opcode.name == "PUSH"
    assert isinstance(inst.operand, int) and inst.operand == 42
    assert len(program.symbol_table) == 0


def test_label() -> None:
    program = par.parse(tokenize("PUSH 10\nADD\ntarget:\nHALT"))
    assert len(program.instructions) == 3
    assert program.symbol_table == {"target": 6}


def test_source_mapping() -> None:
    inst = par.parse(tokenize("PUSH 10\nADD\ntarget:\n    HALT")).instructions
    assert inst[0].line == 1 and inst[0].column == 1
    assert inst[1].line == 2 and inst[1].column == 1
    assert inst[2].line == 4 and inst[2].column == 5


def test_multiple_labels() -> None:
    program = par.parse(tokenize("label1:\nlabel2:\nHALT"))
    assert len(program.instructions) == 1
    assert program.symbol_table == {"label1": 0, "label2": 0}


def test_trailing_label() -> None:
    program = par.parse(tokenize("PUSH 1\nend:"))
    assert len(program.instructions) == 1
    assert program.symbol_table == {"end": 5}


@pytest.mark.parametrize("mnemonic", JUMP_INSTRUCTIONS)
def test_jump(mnemonic: str) -> None:
    program = par.parse(tokenize(mnemonic + " target\ntarget:\nHALT"))
    assert len(program.instructions) == 2

    i = program.instructions[0]
    assert i.opcode.name == mnemonic
    assert isinstance(i.operand, int) and i.operand == 5

    assert len(program.symbol_table) == 1


@pytest.mark.parametrize("mnemonic", JUMP_INSTRUCTIONS)
def test_backward_jump(mnemonic: str) -> None:
    program = par.parse(tokenize("target:\nADD\n" + mnemonic + " target"))
    assert len(program.instructions) == 2

    i = program.instructions[1]
    assert i.opcode.name == mnemonic
    assert isinstance(i.operand, int) and i.operand == 0

    assert len(program.symbol_table) == 1


@pytest.mark.parametrize("mnemonic", JUMP_INSTRUCTIONS)
def test_numeric_jump(mnemonic: str) -> None:
    program = par.parse(tokenize(mnemonic + " 5\nHALT"))
    assert len(program.instructions) == 2

    i = program.instructions[0]
    assert i.opcode.name == mnemonic
    assert isinstance(i.operand, int) and i.operand == 5

    assert len(program.symbol_table) == 0


@pytest.mark.parametrize("mnemonic", NULLARY_INSTRUCTIONS)
def test_unexpected_argument(mnemonic: str) -> None:
    with pytest.raises(par.InvalidOperandError) as excinfo:
        par.parse(tokenize(mnemonic + " 42"))

    assert excinfo.value.line == 1 and excinfo.value.column == len(mnemonic) + 2


@pytest.mark.parametrize("mnemonic", UNARY_INSTRUCTIONS)
def test_no_expected_argument(mnemonic: str) -> None:
    with pytest.raises(par.InvalidOperandError) as excinfo:
        par.parse(tokenize(mnemonic))

    assert excinfo.value.line == 1 and excinfo.value.column == len(mnemonic) + 1


@pytest.mark.parametrize("argument", ["label", "ADD"])
def test_invalid_operand_type_push(argument: str) -> None:
    with pytest.raises(par.InvalidOperandError) as excinfo:
        par.parse(tokenize("PUSH " + argument))

    assert excinfo.value.line == 1 and excinfo.value.column == 6


@pytest.mark.parametrize("text, column", [("PUSH 1 2", 8), ("ADD SUB", 5)])
def test_trailing_tokens(text: str, column: int) -> None:
    with pytest.raises(par.InvalidOperandError) as excinfo:
        par.parse(tokenize(text))

    assert excinfo.value.line == 1 and excinfo.value.column == column


@pytest.mark.parametrize(
    "text, column",
    [(f"PUSH {1 << 31}", 6), (f"PUSH {-(1 << 31) - 1}", 6), ("JMP -1", 5), (f"JZ {1 << 32}", 4)],
)
def test_int_overflow(text: str, column: int) -> None:
    with pytest.raises(par.IntegerOverflowError) as excinfo:
        par.parse(tokenize(text))

    assert excinfo.value.line == 1 and excinfo.value.column == column


def test_label_duplicate() -> None:
    with pytest.raises(par.DuplicateLabelError) as excinfo:
        par.parse(tokenize("label:\nPUSH 42\nlabel:\nHALT"))

    assert excinfo.value.line == 3 and excinfo.value.column == 1


@pytest.mark.parametrize("mnemonic", JUMP_INSTRUCTIONS)
def test_undefined_label(mnemonic: str) -> None:
    with pytest.raises(par.UndefinedLabelError) as excinfo:
        par.parse(tokenize(mnemonic + " label"))

    assert excinfo.value.line == 1 and excinfo.value.column == 1
