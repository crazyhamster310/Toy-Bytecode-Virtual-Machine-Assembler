#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>

namespace vm {

class VmException : public std::runtime_error {
 public:
  using std::runtime_error::runtime_error;
};

class StackUnderflowException : public VmException {
 public:
  StackUnderflowException() : VmException("Stack underflow: evaluated empty stack") {}
};

class StackOverflowException : public VmException {
 public:
  StackOverflowException() : VmException("Stack overflow: maximum stack depth exceeded") {}
};

class MemoryOutOfBoundsException : public VmException {
 public:
  explicit MemoryOutOfBoundsException(size_t address)
      : VmException("Memory access out of bounds at address: " + std::to_string(address)) {}
};

class DivisionByZeroException : public VmException {
 public:
  DivisionByZeroException() : VmException("Division by zero") {}
};

class InvalidOpcodeException : public VmException {
 public:
  explicit InvalidOpcodeException(uint8_t opcode)
      : VmException("Invalid opcode: " + std::to_string(opcode)) {}
};

}  // namespace vm