import argparse
import sys
from pathlib import Path

from byte_asm.emitter import emit
from byte_asm.lexer import LexerError, tokenize
from byte_asm.parser import ParserError, parse


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        prog="byte-asm",
        description="Two-pass assembler for Toy Bytecode Virtual Machine",
    )
    parser.add_argument("source", type=Path, help="path to input .asm source file")
    parser.add_argument("-o", "--output", type=Path, help="path to output .bin file")
    return parser


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)

    if not args.source.exists():
        print(f"error: source file '{args.source}' not found", file=sys.stderr)
        return 1

    try:
        source_text = args.source.read_text(encoding="utf-8")
        tokens = tokenize(source_text)
        program = parse(tokens)
        bytecode = emit(program)

        output_path: Path = args.output if args.output else args.source.with_suffix(".bin")
        output_path.write_bytes(bytecode)
        print(f"Compiled {args.source} -> {output_path} ({len(bytecode)} bytes)")
        return 0

    except (LexerError, ParserError) as err:
        print(f"error: [{args.source}:{err.line}:{err.column}] {err.message}", file=sys.stderr)
        return 1
    except OSError as err:
        print(f"error: I/O failure: {err}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    sys.exit(main())
