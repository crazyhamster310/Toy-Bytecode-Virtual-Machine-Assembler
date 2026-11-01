import pytest

from byte_asm.isa import NULLARY_INSTRUCTIONS
from byte_asm.lexer import (
    InvalidIdentifierError,
    InvalidNumberError,
    ReservedWordError,
    TokenType,
    UnexpectedSymbolError,
    tokenize,
)


def test_eof() -> None:
    tokens = tokenize("")
    assert len(tokens) == 1
    assert tokens[0].type == TokenType.EOF and tokens[0].value is None


@pytest.mark.parametrize("mnemonic", NULLARY_INSTRUCTIONS)
def test_instructions(mnemonic: str) -> None:
    tokens = tokenize(mnemonic)
    assert len(tokens) == 2
    assert tokens[0].type == TokenType.INSTRUCTION and tokens[0].value == mnemonic


def test_newline() -> None:
    tokens = tokenize("ADD\nHALT")
    assert len(tokens) == 4
    assert tokens[1].type == TokenType.NEWLINE and tokens[1].value is None


def test_windows_newline() -> None:
    tokens = tokenize("ADD\r\nHALT")
    assert len(tokens) == 4
    assert tokens[1].type == TokenType.NEWLINE


def test_bare_carriage_return_error() -> None:
    with pytest.raises(UnexpectedSymbolError) as excinfo:
        tokenize("PUSH 42\r")

    assert excinfo.value.line == 1 and excinfo.value.column == 8


def test_multiple_newlines() -> None:
    tokens = tokenize("\n\nADD\n\nADD\n\n")
    assert len(tokens) == 4
    assert [t.type for t in tokens] == [
        TokenType.INSTRUCTION,
        TokenType.NEWLINE,
        TokenType.INSTRUCTION,
        TokenType.EOF,
    ]


def test_instructions_with_arguments() -> None:
    tokens = tokenize("PUSH 42")
    assert len(tokens) == 3
    assert tokens[0].type == TokenType.INSTRUCTION and tokens[0].value == "PUSH"
    assert tokens[1].type == TokenType.INTEGER and tokens[1].value == 42


def test_negative_integer() -> None:
    tokens = tokenize("PUSH -42")
    assert len(tokens) == 3
    assert tokens[1].type == TokenType.INTEGER and tokens[1].value == -42


def test_unary_plus() -> None:
    tokens = tokenize("PUSH +42")
    assert len(tokens) == 3
    assert tokens[1].type == TokenType.INTEGER and tokens[1].value == 42


def test_hex_integer() -> None:
    tokens = tokenize("PUSH 0x2A")
    assert len(tokens) == 3
    assert tokens[1].type == TokenType.INTEGER and tokens[1].value == 42


def test_case_insensitive() -> None:
    assert tokenize("ADD") == tokenize("aDd")


def test_spaces() -> None:
    tokens = tokenize("\tPUSH    42\nPUSH\t42")
    assert len(tokens) == 6
    assert (
        tokens[0].type == tokens[3].type == TokenType.INSTRUCTION
        and tokens[0].value == tokens[3].value == "PUSH"
    )
    assert (
        tokens[1].type == tokens[4].type == TokenType.INTEGER
        and tokens[1].value == tokens[4].value == 42
    )
    assert tokens[2].type == TokenType.NEWLINE


def test_comment() -> None:
    tokens = tokenize("PUSH 10 ; comment")
    assert len(tokens) == 3
    assert tokens[1].type == TokenType.INTEGER and tokens[1].value == 10


def test_comment_new_line() -> None:
    tokens = tokenize("; new line comment")
    assert len(tokens) == 1


def test_labels_definition() -> None:
    tokens = tokenize("_loop_1:")
    assert len(tokens) == 2
    assert tokens[0].type == TokenType.LABEL_DEF and tokens[0].value == "_loop_1"


def test_label_usage() -> None:
    tokens = tokenize("JMP loop")
    assert len(tokens) == 3
    assert tokens[1].type == TokenType.IDENTIFIER and tokens[1].value == "loop"


def test_labels_case_sensitive() -> None:
    tokens = tokenize("Loop:\nloop:")
    assert tokens[0].value != tokens[2].value


def test_labels_no_space() -> None:
    tokens = tokenize("loop:PUSH 42")
    assert len(tokens) == 4
    assert tokens[0].value == "loop"


def test_label_with_instruction_prefix() -> None:
    tokens = tokenize("additional: PUSH 1\nJMP additional")
    assert len(tokens) == 7
    assert tokens[0].type == TokenType.LABEL_DEF and tokens[0].value == "additional"


def test_digit_identifier() -> None:
    with pytest.raises(InvalidIdentifierError) as excinfo:
        tokenize("1loop:")

    assert excinfo.value.line == 1 and excinfo.value.column == 1


def test_identifier_space() -> None:
    with pytest.raises(UnexpectedSymbolError) as excinfo:
        tokenize("loop :")

    assert excinfo.value.line == 1 and excinfo.value.column == 6


def test_reserved_word() -> None:
    with pytest.raises(ReservedWordError) as excinfo:
        tokenize("add:")

    assert excinfo.value.line == 1 and excinfo.value.column == 1


def test_source_mapping() -> None:
    tokens = tokenize("PUSH 42\n POP")
    assert tokens[0].line == 1 and tokens[0].column == 1
    assert tokens[1].line == 1 and tokens[1].column == 6
    assert tokens[3].line == 2 and tokens[3].column == 2


def test_unknown_symbols() -> None:
    with pytest.raises(UnexpectedSymbolError) as excinfo:
        tokenize("@#")

    assert excinfo.value.line == 1 and excinfo.value.column == 1


@pytest.mark.parametrize(
    "number",
    ["12a", "+", "-", "0x"],
    ids=["unexpected_letter", "unary_plus", "unary_minus", "incomplete_hex_number"],
)
def test_invalid_number(number: str) -> None:
    with pytest.raises(InvalidNumberError) as excinfo:
        tokenize("PUSH " + number)

    assert excinfo.value.line == 1 and excinfo.value.column == 6
