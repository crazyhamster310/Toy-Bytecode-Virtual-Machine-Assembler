#pragma once

#include <cstddef>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string_view>

#include "vm/opcodes.hpp"

namespace vm {

enum class ErrorCode : uint8_t {
  HeaderTooShort,
  InvalidMagicBytes,
  UnsupportedVersion,

  StackUnderflow,
  StackOverflow,
  MemoryOutOfBounds,
  JumpOutOfBounds,
  DivisionByZero,
  SignedOverflow,
  InvalidOpcode,
  TruncatedInstruction,
  MissingHalt,
  ExecutionHalted,
  InputFailure,
};

class VmException : public std::runtime_error {
 private:
  ErrorCode code_;

 public:
  VmException(ErrorCode code, std::string_view message)
      : std::runtime_error(std::string(message)), code_(code) {}

  [[nodiscard]] ErrorCode code() const noexcept { return code_; }
};

class VmLoadError : public VmException {
 public:
  using VmException::VmException;
};

class VmRuntimeError : public VmException {
 private:
  size_t ip_;
  Opcode opcode_;

 public:
  VmRuntimeError(size_t instr_ip, Opcode opcode, ErrorCode code, std::string_view message)
      : VmException(code,
                    std::format("[IP: 0x{:08X}] ({}): {}", instr_ip, to_string(opcode), message)),
        ip_(instr_ip),
        opcode_(opcode) {}

  [[nodiscard]] Opcode opcode() const noexcept { return opcode_; }
  [[nodiscard]] size_t ip() const noexcept { return ip_; }
};

}  // namespace vm