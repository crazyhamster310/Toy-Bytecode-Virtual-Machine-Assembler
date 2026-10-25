from dataclasses import dataclass
from enum import StrEnum, auto
from string import ascii_letters, digits, whitespace

from byte_asm.isa import INSTRUCTION_SET


class TokenType(StrEnum):
    INSTRUCTION = auto()
    LABEL_DEF = auto()
    IDENTIFIER = auto()
    INTEGER = auto()
    NEWLINE = auto()
    EOF = auto()


@dataclass(frozen=True, slots=True)
class Token:
    type: TokenType
    value: str | int | None
    line: int
    column: int


class LexerError(Exception):
    def __init__(self, line: int, column: int, message: str) -> None:
        self.line = line
        self.column = column
        super().__init__(f"[{line}:{column}] {message}")


class UnexpectedSymbolError(LexerError):
    def __init__(self, line: int, column: int, symbol: str) -> None:
        super().__init__(line, column, f"Unexpected symbol: {symbol}")


class InvalidNumberError(LexerError):
    def __init__(self, line: int, column: int, number: str) -> None:
        super().__init__(line, column, f"Invalid number: {number}")


class InvalidIdentifierError(LexerError):
    def __init__(self, line: int, column: int, identifier: str) -> None:
        super().__init__(line, column, f"Invalid identifier: {identifier}")


class ReservedWordError(LexerError):
    def __init__(self, line: int, column: int, word: str) -> None:
        super().__init__(line, column, f"Reserved word: {word}")


_IDENT_START: frozenset[str] = frozenset(ascii_letters + "_")
_IDENT_BODY: frozenset[str] = frozenset(ascii_letters + digits + "_")
_NUMBER_STOP_LIST: frozenset[str] = frozenset(whitespace + ":;")


class _Lexer:
    def __init__(self, source: str) -> None:
        self.source = source
        self.cursor = 0
        self.line = 1
        self.column = 1
        self.tokens: list[Token] = []

    def is_at_end(self) -> bool:
        return self.cursor >= len(self.source)

    def peek(self) -> str:
        return "" if self.is_at_end() else self.source[self.cursor]

    def advance(self) -> str:
        symbol = self.peek()
        self.cursor += 1
        self.column += 1
        return symbol

    def newline(self) -> None:
        if self.tokens and self.tokens[-1].type != TokenType.NEWLINE:
            self.tokens.append(Token(TokenType.NEWLINE, None, self.line, self.column))
        self.line += 1
        self.column = 1
        self.cursor += 1

    def tokenize(self) -> list[Token]:
        while not self.is_at_end():
            if self.peek() in " \t":
                self.advance()

            elif self.peek() == ";":
                while not (self.peek() in "\r\n" or self.is_at_end()):
                    self.advance()

            elif self.peek() == "\r":
                self.advance()
                if self.peek() == "\n":
                    self.newline()
                else:
                    raise UnexpectedSymbolError(self.line, self.column - 1, "\r")

            elif self.peek() == "\n":
                self.newline()

            elif self.peek() in _IDENT_START:
                start_line = self.line
                start_column = self.column
                start_index = self.cursor

                while not self.is_at_end() and self.peek() in _IDENT_BODY:
                    self.advance()

                text = self.source[start_index : self.cursor]

                if self.peek() == ":":
                    if text.upper() in INSTRUCTION_SET:
                        raise ReservedWordError(start_line, start_column, text)
                    self.tokens.append(Token(TokenType.LABEL_DEF, text, start_line, start_column))
                    self.advance()
                elif text.upper() in INSTRUCTION_SET:
                    self.tokens.append(
                        Token(TokenType.INSTRUCTION, text.upper(), start_line, start_column)
                    )
                else:
                    self.tokens.append(Token(TokenType.IDENTIFIER, text, start_line, start_column))

            elif self.peek() in digits + "+-":
                start_line = self.line
                start_column = self.column
                start_index = self.cursor

                while not self.is_at_end() and self.peek() not in _NUMBER_STOP_LIST:
                    self.advance()

                raw_int = self.source[start_index : self.cursor]

                if self.peek() == ":":
                    raise InvalidIdentifierError(start_line, start_column, raw_int)

                try:
                    parsed_int = int(raw_int, 0)

                    self.tokens.append(
                        Token(TokenType.INTEGER, parsed_int, start_line, start_column)
                    )
                except ValueError:
                    raise InvalidNumberError(start_line, start_column, raw_int) from ValueError

            else:
                raise UnexpectedSymbolError(self.line, self.column, self.peek())

        if self.tokens and self.tokens[-1].type == TokenType.NEWLINE:
            self.tokens.pop()

        self.tokens.append(Token(TokenType.EOF, None, self.line, self.column))

        return self.tokens


def tokenize(s: str) -> list[Token]:
    return _Lexer(s).tokenize()
