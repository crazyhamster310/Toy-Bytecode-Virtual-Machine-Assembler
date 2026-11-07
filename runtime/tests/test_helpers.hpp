#pragma once

#include <gtest/gtest.h>

#include <concepts>
#include <source_location>
#include <vector>

#include "vm/errors.hpp"
#include "vm/opcodes.hpp"

namespace vm::test {

class BytecodeBuilder {
 private:
  std::vector<uint8_t> bytes_;

 public:
  [[nodiscard]] std::vector<uint8_t> build() const {
    std::vector<uint8_t> result;
    result.reserve(HEADER_SIZE + bytes_.size());
    std::ranges::copy(MAGIC_BYTES, std::back_inserter(result));
    result.push_back(static_cast<uint8_t>(static_cast<uint32_t>(VERSION) & 0xFFU));
    result.push_back(static_cast<uint8_t>((static_cast<uint32_t>(VERSION) >> 8U) & 0xFFU));
    std::ranges::copy(bytes_, std::back_inserter(result));
    return result;
  }

  BytecodeBuilder& write_opcode(const Opcode& opcode) {
    bytes_.push_back(static_cast<uint8_t>(opcode));
    return *this;
  }

  BytecodeBuilder& write_uint32_le(uint32_t value) {
    bytes_.push_back(static_cast<uint8_t>(value & 0xFFU));
    bytes_.push_back(static_cast<uint8_t>((value >> 8U) & 0xFFU));
    bytes_.push_back(static_cast<uint8_t>((value >> 16U) & 0xFFU));
    bytes_.push_back(static_cast<uint8_t>((value >> 24U) & 0xFFU));
    return *this;
  }

  BytecodeBuilder& write_int32_le(int32_t value) {
    return write_uint32_le(static_cast<uint32_t>(value));
  }

  BytecodeBuilder& push(int32_t value) { return write_opcode(Opcode::PUSH).write_int32_le(value); }

  BytecodeBuilder& halt() { return write_opcode(Opcode::HALT); }
  BytecodeBuilder& pop() { return write_opcode(Opcode::POP); }
  BytecodeBuilder& dup() { return write_opcode(Opcode::DUP); }
  BytecodeBuilder& swap() { return write_opcode(Opcode::SWAP); }

  BytecodeBuilder& add() { return write_opcode(Opcode::ADD); }
  BytecodeBuilder& sub() { return write_opcode(Opcode::SUB); }
  BytecodeBuilder& mul() { return write_opcode(Opcode::MUL); }
  BytecodeBuilder& div() { return write_opcode(Opcode::DIV); }
  BytecodeBuilder& mod() { return write_opcode(Opcode::MOD); }

  BytecodeBuilder& eq() { return write_opcode(Opcode::EQ); }
  BytecodeBuilder& lt() { return write_opcode(Opcode::LT); }
  BytecodeBuilder& gt() { return write_opcode(Opcode::GT); }

  BytecodeBuilder& print() { return write_opcode(Opcode::PRINT); }
  BytecodeBuilder& read() { return write_opcode(Opcode::READ); }
  BytecodeBuilder& putc() { return write_opcode(Opcode::PUTC); }

  BytecodeBuilder& load() { return write_opcode(Opcode::LOAD); }
  BytecodeBuilder& store() { return write_opcode(Opcode::STORE); }

  BytecodeBuilder& jmp(uint32_t address) {
    return write_opcode(Opcode::JMP).write_uint32_le(address);
  }

  BytecodeBuilder& jz(uint32_t address) {
    return write_opcode(Opcode::JZ).write_uint32_le(address);
  }
};

}  // namespace vm::test

void expect_vm_runtime_error(std::invocable auto&& func, vm::ErrorCode expected_code,
                             size_t expected_ip, vm::Opcode expected_opcode,
                             const std::source_location& loc = std::source_location::current()) {
  testing::ScopedTrace trace(loc.file_name(), static_cast<int>(loc.line()),
                             "expect_vm_runtime_error");

  try {
    func();
    FAIL() << "Expected VmRuntimeError with code " << static_cast<int>(expected_code);
  } catch (const vm::VmRuntimeError& err) {
    EXPECT_EQ(err.code(), expected_code);
    EXPECT_EQ(err.ip(), expected_ip);
    EXPECT_EQ(err.opcode(), expected_opcode);
  } catch (const std::exception& e) {
    FAIL() << "Expected VmRuntimeError, but caught: " << e.what();
  }
}

void expect_vm_load_error(std::invocable auto&& func, vm::ErrorCode expected_code,
                          const std::source_location& loc = std::source_location::current()) {
  testing::ScopedTrace trace(loc.file_name(), static_cast<int>(loc.line()), "expect_vm_load_error");

  try {
    func();
    FAIL() << "Expected VmLoadError with code " << static_cast<int>(expected_code);
  } catch (const vm::VmLoadError& err) {
    EXPECT_EQ(err.code(), expected_code);
  } catch (const std::exception& e) {
    FAIL() << "Expected VmLoadError, but caught: " << e.what();
  }
}
