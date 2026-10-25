from dataclasses import dataclass, replace

import byte_asm.isa as isa
from byte_asm.lexer import Token, TokenType


@dataclass(frozen=True, slots=True)
class Instruction:
    opcode: isa.Opcode
    operand: int | None | str
    line: int
    column: int


@dataclass(frozen=True, slots=True)
class Program:
    instructions: list[Instruction]
    symbol_table: dict[str, int]


class ParserError(Exception):
    def __init__(self, line: int, column: int, message: str) -> None:
        self.line = line
        self.column = column
        self.message = message
        super().__init__(f"[{line}:{column}] {message}")


class InvalidOperandError(ParserError): ...


class DuplicateLabelError(ParserError):
    def __init__(self, line: int, column: int, label: str) -> None:
        super().__init__(line, column, f"Duplicate Label Name: '{label}'")


class UndefinedLabelError(ParserError):
    def __init__(self, line: int, column: int, label: str) -> None:
        super().__init__(line, column, f"Undefined Label: '{label}'")


class IntegerOverflowError(ParserError):
    def __init__(self, line: int, column: int, number: int) -> None:
        super().__init__(line, column, f"Integer Overflow: {number}")


class _TokenStream:
    def __init__(self, tokens: list[Token]) -> None:
        self.tokens = tokens
        self.cursor = 0
        self.offset = 0
        self.instructions: list[Instruction] = []
        self.symbol_table: dict[str, int] = {}

    def is_at_end(self) -> bool:
        return self.cursor >= len(self.tokens) or self.tokens[self.cursor].type == TokenType.EOF

    def peek(self) -> Token:
        return self.tokens[-1] if self.is_at_end() else self.tokens[self.cursor]

    def advance(self) -> Token:
        token = self.peek()
        self.cursor += 1
        return token

    def match(self, token_type: TokenType) -> bool:
        if self.peek().type == token_type:
            self.cursor += 1
            return True
        return False

    def do_pass(self) -> tuple[list[Instruction], dict[str, int]]:
        while not self.is_at_end():
            if self.match(TokenType.NEWLINE):
                continue

            if self.peek().type == TokenType.LABEL_DEF:
                label = self.advance()
                if label.value in self.symbol_table:
                    raise DuplicateLabelError(label.line, label.column, label.value)
                assert isinstance(label.value, str)
                self.symbol_table[label.value] = self.offset
                continue

            if self.peek().type == TokenType.INSTRUCTION:
                token = self.advance()
                assert isinstance(token.value, str)
                mnemonic = token.value
                operand: str | int | None

                if mnemonic in isa.NULLARY_INSTRUCTIONS:
                    operand = None
                elif mnemonic in isa.VALUE_INSTRUCTIONS:
                    operand_token = self.advance()
                    if operand_token.type != TokenType.INTEGER:
                        raise InvalidOperandError(
                            operand_token.line,
                            operand_token.column,
                            f"Unexpected operand type: {operand_token.type}",
                        )

                    assert isinstance(operand_token.value, int)
                    operand = operand_token.value
                    if not (-(1 << 31) <= operand < (1 << 31)):
                        raise IntegerOverflowError(
                            operand_token.line, operand_token.column, operand
                        )
                elif mnemonic in isa.JUMP_INSTRUCTIONS:
                    operand_token = self.advance()

                    if operand_token.type == TokenType.INTEGER:
                        assert isinstance(operand_token.value, int)
                        operand = operand_token.value
                        if not (0 <= operand < (1 << 32)):
                            raise IntegerOverflowError(
                                operand_token.line, operand_token.column, operand
                            )
                    elif operand_token.type == TokenType.IDENTIFIER:
                        assert isinstance(operand_token.value, str)
                        operand = operand_token.value
                    else:
                        raise InvalidOperandError(
                            operand_token.line,
                            operand_token.column,
                            f"Unexpected operand type: {operand_token.type}",
                        )
                else:
                    raise ParserError(
                        token.line, token.column, f"Unknown instruction mnemonic: '{token.value}'"
                    )

                if not (self.match(TokenType.NEWLINE) or self.match(TokenType.EOF)):
                    raise InvalidOperandError(
                        self.peek().line,
                        self.peek().column,
                        f"Unexpected operand: {self.peek().value}",
                    )

                self.instructions.append(
                    Instruction(isa.Opcode[mnemonic], operand, token.line, token.column)
                )
                self.offset += isa.INSTRUCTION_SIZES.get(mnemonic, isa.DEFAULT_INSTRUCTION_SIZE)

                continue

            raise ParserError(
                self.peek().line, self.peek().column, f"Unexpected token type: '{self.peek().type}'"
            )

        return self.instructions, self.symbol_table


def _first_pass(tokens: list[Token]) -> tuple[list[Instruction], dict[str, int]]:
    return _TokenStream(tokens).do_pass()


def _second_pass(
    instructions: list[Instruction], symbol_table: dict[str, int]
) -> list[Instruction]:
    result: list[Instruction] = []
    for inst in instructions:
        operand = inst.operand
        if inst.opcode.name in isa.JUMP_INSTRUCTIONS and isinstance(inst.operand, str):
            if inst.operand not in symbol_table:
                raise UndefinedLabelError(inst.line, inst.column, inst.operand)
            operand = symbol_table[inst.operand]
        result.append(replace(inst, operand=operand))
    return result


def parse(tokens: list[Token]) -> Program:
    raw_instructions, symbol_table = _first_pass(tokens)
    resolved_instructions = _second_pass(raw_instructions, symbol_table)
    return Program(instructions=resolved_instructions, symbol_table=symbol_table)
