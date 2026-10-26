#pragma once

#include <array>
#include <cstdint>
#include <string_view>

namespace vm {

enum class Opcode : uint8_t {
  HALT = 0xFF,
  PUSH = 0x01,
  POP = 0x02,
  DUP = 0x03,
  SWAP = 0x04,
  ADD = 0x10,
  SUB = 0x11,
  MUL = 0x12,
  DIV = 0x13,
  MOD = 0x14,
  EQ = 0x20,
  LT = 0x21,
  GT = 0x22,
  JMP = 0x30,
  JZ = 0x31,
  PRINT = 0x40,
  LOAD = 0x50,
  STORE = 0x51,
};

inline constexpr std::array<uint8_t, 4> MAGIC_BYTES{'B', 'V', 'M', '1'};
inline constexpr uint16_t VERSION = 1;
inline constexpr size_t HEADER_SIZE = 6;

constexpr std::string_view to_string(Opcode opcode) {
  switch (opcode) {
    case Opcode::HALT:
      return "HALT";
    case Opcode::PUSH:
      return "PUSH";
    case Opcode::POP:
      return "POP";
    case Opcode::DUP:
      return "DUP";
    case Opcode::SWAP:
      return "SWAP";
    case Opcode::ADD:
      return "ADD";
    case Opcode::SUB:
      return "SUB";
    case Opcode::MUL:
      return "MUL";
    case Opcode::DIV:
      return "DIV";
    case Opcode::MOD:
      return "MOD";
    case Opcode::EQ:
      return "EQ";
    case Opcode::LT:
      return "LT";
    case Opcode::GT:
      return "GT";
    case Opcode::JMP:
      return "JMP";
    case Opcode::JZ:
      return "JZ";
    case Opcode::PRINT:
      return "PRINT";
    case Opcode::LOAD:
      return "LOAD";
    case Opcode::STORE:
      return "STORE";
  }
  return "UNKNOWN";
}

}  // namespace vm
