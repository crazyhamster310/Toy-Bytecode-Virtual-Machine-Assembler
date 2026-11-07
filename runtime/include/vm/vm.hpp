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
  bool trace{false};
};

class VirtualMachine {
 private:
  VmConfig config_;
  EvaluationStack stack_;
  LinearMemory memory_;
  std::vector<uint8_t> bytecode_;
  size_t ip_{0};
  bool halted_{false};
  std::reference_wrapper<std::ostream> out_;
  std::reference_wrapper<std::istream> in_;
  std::reference_wrapper<std::ostream> trace_out_;

  int32_t fetch_i32(size_t instr_ip, Opcode opcode);
  uint32_t fetch_u32(size_t instr_ip, Opcode opcode);

  void require_stack(size_t count, size_t instr_ip, Opcode opcode) const;
  void ensure_stack_capacity(size_t needed, size_t instr_ip, Opcode opcode) const;
  void require_memory_bounds(uint32_t address, size_t instr_ip, Opcode opcode) const;

  size_t fetch_jump_address(size_t instr_ip, Opcode opcode);

 public:
  explicit VirtualMachine(const VmConfig& config = VmConfig(), std::ostream& out = std::cout,
                          std::istream& inp = std::cin, std::ostream& trace_out = std::cerr)
      : config_(config),
        stack_(config.stack_size),
        memory_(config.memory_size),
        out_(out),
        in_(inp),
        trace_out_(trace_out) {}

  void load_bytecode(std::span<const uint8_t> bytecode);
  void step();
  void run();
  [[nodiscard]] bool is_halted() const { return halted_; }
  [[nodiscard]] const EvaluationStack& stack() const { return stack_; }
  [[nodiscard]] const LinearMemory& memory() const { return memory_; }
  [[nodiscard]] size_t ip() const { return ip_; }
};

}  // namespace vm
