#pragma once

#include <functional>
#include <iostream>
#include <ostream>
#include <span>

#include "vm/memory.hpp"
#include "vm/opcodes.hpp"

namespace vm {

struct VmConfig {
  size_t stack_size{16384};
  size_t memory_size{16384};
};

class VirtualMachine {
 private:
  EvaluationStack stack_;
  LinearMemory memory_;
  std::vector<uint8_t> bytecode_;
  size_t ip_{0};
  bool halted_{false};
  std::reference_wrapper<std::ostream> out_;

  uint8_t fetch_u8();
  int32_t fetch_i32();
  uint32_t fetch_u32();

 public:
  explicit VirtualMachine(const VmConfig& config = VmConfig(), std::ostream& out = std::cout)
      : stack_(config.stack_size), memory_(config.memory_size), out_(out) {}

  void load_bytecode(std::span<const uint8_t> bytecode);
  void step();
  void run();
  [[nodiscard]] bool is_halted() const { return halted_; }
  [[nodiscard]] const EvaluationStack& stack() const { return stack_; }
  [[nodiscard]] const LinearMemory& memory() const { return memory_; }
  [[nodiscard]] size_t ip() const { return ip_; }
};

}  // namespace vm
